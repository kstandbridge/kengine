#include <Windows.h>
#include <Psapi.h>

internal void
Win32ConsoleOut_(char *Format, va_list ArgList)
{
    u8 Buffer[4096] = {0};
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    AppendFormatString_(&StringState, Format, ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    
#if KENGINE_CONSOLE
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    DWORD NumberOfBytesWritten;
    WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&NumberOfBytesWritten, 0);
    Assert(NumberOfBytesWritten == Text.Size);
#endif
    
#if KENGINE_INTERNAL
    Buffer[Text.Size] = '\0';
    OutputDebugStringA((char *)Buffer);
#endif
}

void
Win32ConsoleOut(char *Format, ...)
{
    va_list ArgList;
    va_start(ArgList, Format);
    
    Win32ConsoleOut_(Format, ArgList);
    
    va_end(ArgList);
    
}

typedef enum win32_memory_block_flag
{
    MemoryBlockFlag_AllocatedDuringLooping = 0x1,
    MemoryBlockFlag_FreedDuringLooping = 0x2,
} win32_memory_block_flag;

typedef struct win32_memory_block
{
    platform_memory_block PlatformBlock;
    struct win32_memory_block *Prev;
    struct win32_memory_block *Next;
    u64 LoopingFlags;
} win32_memory_block;

typedef struct win32_saved_memory_block
{
    u64 BasePointer;
    u64 Size;
} win32_saved_memory_block;

typedef struct win32_global_memory
{
    HANDLE ProcessHandle;

    ticket_mutex MemoryMutex;
    win32_memory_block MemorySentinel;

    HANDLE RecordingHandle;
    s32 InputRecordingIndex;

    HANDLE PlaybackHandle;
    s32 InputPlayingIndex;

    char ExeFilePathBuffer[MAX_PATH];
    string ExeFilePath;
    string ExeDirectoryPath;

} win32_global_memory;

global win32_global_memory GlobalMemory;

internal void
InitGlobalMemory()
{
    GlobalMemory.ProcessHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, GetCurrentProcessId());
    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;
    
    DWORD ExePathSize = GetModuleFileNameA(0, GlobalMemory.ExeFilePathBuffer, sizeof(GlobalMemory.ExeFilePathBuffer));
    GlobalMemory.ExeFilePath = String_(ExePathSize, GlobalMemory.ExeFilePathBuffer);
    char *OnePastLastExeFileNameSlash = GlobalMemory.ExeFilePathBuffer;
    for(char *At = GlobalMemory.ExeFilePathBuffer;
        *At;
        ++At)
    {
        if(*At == '\\')
        {
            OnePastLastExeFileNameSlash = At + 1;
        }
    }
    
    GlobalMemory.ExeDirectoryPath = String_((umm)(OnePastLastExeFileNameSlash - GlobalMemory.ExeFilePathBuffer),
                                            GlobalMemory.ExeFilePathBuffer);
}

internal b32
Win32IsInLoop()
{
    b32 Result = ((GlobalMemory.InputRecordingIndex != 0) ||
                  (GlobalMemory.InputPlayingIndex));
    return Result;
}

platform_memory_block *
Win32AllocateMemory(umm Size, u64 Flags)
{
    // NOTE(kstandbridge): Cache align
    Assert(sizeof(win32_memory_block) == 64);
    
    // NOTE(kstandbridge): Raymond Chen on page sizes: https://devblogs.microsoft.com/oldnewthing/20210510-00/?p=105200
    // TODO(kstandbridge): GetSystemInfo > dwPageSize
    umm PageSize = Kilobytes(4);
    umm TotalSize = Size + sizeof(win32_memory_block);
    umm BaseOffset = sizeof(win32_memory_block);
    umm ProtectOffset = 0;
    if(Flags & PlatformMemoryBlockFlag_UnderflowCheck)
    {
        TotalSize = Size + 2*PageSize;
        BaseOffset = 2*PageSize;
        ProtectOffset = PageSize;
    }
    else if(Flags & PlatformMemoryBlockFlag_OverflowCheck)
    {
        umm SizeRoundedUp = AlignPow2(Size, PageSize);
        TotalSize = SizeRoundedUp + 2*PageSize;
        BaseOffset = PageSize + SizeRoundedUp - Size;
        ProtectOffset = PageSize + SizeRoundedUp;
    }
    
    win32_memory_block *Win32Block = (win32_memory_block *)VirtualAlloc(0, TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(Win32Block);
    Win32Block->PlatformBlock.Base = (u8 *)Win32Block + BaseOffset;
    Assert(Win32Block->PlatformBlock.Used == 0);
    Assert(Win32Block->PlatformBlock.Prev == 0);
    
    if(Flags & (PlatformMemoryBlockFlag_UnderflowCheck|PlatformMemoryBlockFlag_OverflowCheck))
    {
        DWORD OldProtect = 0;
        b32 IsProtected = VirtualProtect((u8 *)Win32Block + ProtectOffset, PageSize, PAGE_NOACCESS, &OldProtect);
        if(!IsProtected)
        {
            InvalidCodePath;
        }
    }
    
    win32_memory_block *Sentinel = &GlobalMemory.MemorySentinel;
    Win32Block->Next = Sentinel;
    Win32Block->PlatformBlock.Size = Size;
    Win32Block->PlatformBlock.Flags = Flags;
    Win32Block->LoopingFlags = 0;
    if(Win32IsInLoop(&GlobalMemory) && !(Flags & PlatformMemoryBlockFlag_NotRestored))
    {
        Win32Block->LoopingFlags = MemoryBlockFlag_AllocatedDuringLooping;
    }
    
    BeginTicketMutex(&GlobalMemory.MemoryMutex);
    Win32Block->Prev = Sentinel->Prev;
    Win32Block->Prev->Next = Win32Block;
    Win32Block->Next->Prev = Win32Block;
    EndTicketMutex(&GlobalMemory.MemoryMutex);
    
    platform_memory_block *Result = &Win32Block->PlatformBlock;
    return Result;
}

internal void
Win32FreeMemoryBlock(win32_memory_block *Win32Block)
{
    BeginTicketMutex(&GlobalMemory.MemoryMutex);
    Win32Block->Prev->Next = Win32Block->Next;
    Win32Block->Next->Prev = Win32Block->Prev;
    EndTicketMutex(&GlobalMemory.MemoryMutex);
    
    BOOL Result = VirtualFree(Win32Block, 0, MEM_RELEASE);
    Assert(Result);
}

void
Win32DeallocateMemory(platform_memory_block *PlatformBlock)
{
    if(PlatformBlock)
    {
        win32_memory_block *Win32Block = (win32_memory_block *)PlatformBlock;
        if(Win32IsInLoop() && !(Win32Block->PlatformBlock.Flags & PlatformMemoryBlockFlag_NotRestored))
        {
            Win32Block->LoopingFlags = MemoryBlockFlag_FreedDuringLooping;
        }
        else
        {
            Win32FreeMemoryBlock(Win32Block);
        }

    }
}

internal void
Win32ClearBlocksByMask(u64 Mask)
{
    for(win32_memory_block *BlockIter = GlobalMemory.MemorySentinel.Next;
        BlockIter != &GlobalMemory.MemorySentinel;
        )
    {
        win32_memory_block *Block = BlockIter;
        BlockIter = BlockIter->Next;
        
        if((Block->LoopingFlags & Mask) == Mask)
        {
            Win32FreeMemoryBlock(Block);
        }
        else
        {
            Block->LoopingFlags = 0;
        }
    }
}

internal void
Win32BeginInputPlayBack(s32 InputPlayingIndex)
{
    Win32ClearBlocksByMask(MemoryBlockFlag_AllocatedDuringLooping);
    
    char FileName[MAX_PATH];
    FormatStringToBuffer((u8 *)FileName, sizeof(FileName), "%S\\loop_edit_%d_input.kni",
                         GlobalMemory.ExeDirectoryPath, InputPlayingIndex);

    GlobalMemory.PlaybackHandle = CreateFileA(FileName, GENERIC_READ, 0, 0, OPEN_EXISTING, 0, 0);
    if(GlobalMemory.PlaybackHandle != INVALID_HANDLE_VALUE)
    {
        GlobalMemory.InputPlayingIndex = InputPlayingIndex;
        
        for(;;)
        {
            win32_saved_memory_block Block = {0};
            DWORD BytesRead;
            ReadFile(GlobalMemory.PlaybackHandle, &Block, sizeof(Block), &BytesRead, 0);
            if(Block.BasePointer != 0)
            {
                void *BasePointer = (void *)Block.BasePointer;
                Assert(Block.Size <= U32Max);
                ReadFile(GlobalMemory.PlaybackHandle, BasePointer, (u32)Block.Size, &BytesRead, 0);
            }
            else
            {
                break;
            }
            if(BytesRead)
            {
            }
        }
        // TODO(kstandbridge): Stream memory in from the file!
    }
}

internal void
Win32EndInputPlayBack()
{
    Win32ClearBlocksByMask(MemoryBlockFlag_FreedDuringLooping);
    CloseHandle(GlobalMemory.PlaybackHandle);
    GlobalMemory.InputPlayingIndex = 0;
}

internal void
Win32BeginRecordingInput(s32 InputRecordingIndex)
{
    char FileName[MAX_PATH];
    FormatStringToBuffer((u8 *)FileName, sizeof(FileName), "%S\\loop_edit_%d_input.kni",
                         GlobalMemory.ExeDirectoryPath, InputRecordingIndex);
    
    GlobalMemory.RecordingHandle = CreateFileA(FileName, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(GlobalMemory.RecordingHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        
        GlobalMemory.InputRecordingIndex = InputRecordingIndex;
        win32_memory_block *Sentinel = &GlobalMemory.MemorySentinel;
        
        BeginTicketMutex(&GlobalMemory.MemoryMutex);
        for(win32_memory_block *SourceBlock = Sentinel->Next;
            SourceBlock != Sentinel;
            SourceBlock = SourceBlock->Next)
        {
            if(!(SourceBlock->PlatformBlock.Flags & PlatformMemoryBlockFlag_NotRestored))
            {
                win32_saved_memory_block DestBlock;
                void *BasePointer = SourceBlock->PlatformBlock.Base;
                DestBlock.BasePointer = (u64)BasePointer;
                DestBlock.Size = SourceBlock->PlatformBlock.Size;
                WriteFile(GlobalMemory.RecordingHandle, &DestBlock, sizeof(DestBlock), &BytesWritten, 0);
                Assert(DestBlock.Size <= U32Max);
                WriteFile(GlobalMemory.RecordingHandle, BasePointer, (u32)DestBlock.Size, &BytesWritten, 0);
            }
        }
        EndTicketMutex(&GlobalMemory.MemoryMutex);
        
        win32_saved_memory_block DestBlock = {0};
        WriteFile(GlobalMemory.RecordingHandle, &DestBlock, sizeof(DestBlock), &BytesWritten, 0);
    }
}

internal void
Win32RecordInput(app_input *NewInput)
{
    DWORD BytesWritten;
    WriteFile(GlobalMemory.RecordingHandle, NewInput, sizeof(*NewInput), &BytesWritten, 0);
    Assert(BytesWritten == sizeof(*NewInput));
}

internal void
Win32EndRecordingInput()
{
    CloseHandle(GlobalMemory.RecordingHandle);
    GlobalMemory.InputRecordingIndex = 0;
}


internal void
Win32PlayBackInput(app_input *NewInput)
{
    DWORD BytesRead = 0;
    if(ReadFile(GlobalMemory.PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0))
    {
        if(BytesRead == 0)
        {
            // NOTE(kstandbridge): Hit end of stream, loop
            int PlayingIndex = GlobalMemory.InputPlayingIndex;
            Win32EndInputPlayBack();
            Win32BeginInputPlayBack(PlayingIndex);
            ReadFile(GlobalMemory.PlaybackHandle, NewInput, sizeof(*NewInput), &BytesRead, 0);
        }
    }
}

b32
Win32FileExists(string Path)
{
    b32 Result;
    
    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);
    
    u32 Attributes = GetFileAttributesA(CPath);
    
    Result = ((Attributes != INVALID_FILE_ATTRIBUTES) && 
              !(Attributes & FILE_ATTRIBUTE_DIRECTORY));
    
    return Result;
}

string
Win32ReadEntireFile(memory_arena *Arena, string FilePath)
{
    string Result = {0};
    
    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);
    
    HANDLE FileHandle = CreateFileA(CFilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        b32 ReadResult = GetFileSizeEx(FileHandle, &FileSize);
        Assert(ReadResult);
        if(ReadResult)
        {    
            Result.Size = FileSize.QuadPart;
            Result.Data = PushSize(Arena, Result.Size);
            Assert(Result.Data);
            
            if(Result.Data)
            {
                u32 BytesRead;
                ReadResult = ReadFile(FileHandle, Result.Data, (u32)Result.Size, (LPDWORD)&BytesRead, 0);
                Assert(ReadResult);
                Assert(BytesRead == Result.Size);
            }
        }
        
        CloseHandle(FileHandle);
    }
    
    return Result;
}

b32
Win32WriteTextToFile(string Text, string FilePath)
{
    b32 Result = false;

    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);
    
    HANDLE FileHandle = CreateFileA(CFilePath, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {    
        DWORD BytesWritten = 0;
        WriteFile(FileHandle, Text.Data, (DWORD)Text.Size, &BytesWritten, 0);
        
        Result = (Text.Size == BytesWritten);
        Assert(Result);
        
        CloseHandle(FileHandle);
    }
    else
    {
        Assert(!"Failed to get file handle");
    }
    
    return Result;
}

platform_file
Win32OpenFile(string FilePath, platform_file_access_flags Flags)
{
    platform_file Result = {0};
    Assert(sizeof(HANDLE) <= sizeof(Result.Handle));

    DWORD DesiredAccess = 0;
    DWORD CreationDisposition = 0;
    if((Flags & FileAccess_Read) &&
       (Flags & FileAccess_Write))
    {
        DesiredAccess = GENERIC_READ | GENERIC_WRITE;
        CreationDisposition = OPEN_ALWAYS;

    }
    else if(Flags & FileAccess_Read)
    {
        DesiredAccess = GENERIC_READ;
        CreationDisposition = OPEN_EXISTING;
    }
    else if(Flags & FileAccess_Write)
    {
        DesiredAccess = GENERIC_WRITE;
        if(Win32FileExists(FilePath))
        {
            CreationDisposition = TRUNCATE_EXISTING;
        }
        else
        {
            CreationDisposition = CREATE_NEW;
        }
    }

    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);
   
    HANDLE Win32Handle = CreateFileA(CFilePath, DesiredAccess, FILE_SHARE_READ, 0, CreationDisposition, 0, 0);
    
    if(Win32Handle != INVALID_HANDLE_VALUE)
    {
        *(HANDLE *)&Result.Handle = Win32Handle;
        Result.NoErrors = true;
    }
    else
    {
        Assert(!"Open file failure");
        Result.NoErrors = false;
    }

    return Result;
}

void
Win32WriteFile(platform_file *File, string Text)
{
    if(PlatformNoErrors(File))
    {
        HANDLE Win32Handle = *(HANDLE *)&File->Handle;

        OVERLAPPED Overlapped = 
        {
            .Offset = (u32)((File->Offset >> 0) & 0xFFFFFFFF),
            .OffsetHigh = (u32)((File->Offset >> 32) & 0xFFFFFFFF)
        };
        
        DWORD BytesWritten;
        if(WriteFile(Win32Handle, Text.Data, Text.Size, &BytesWritten, &Overlapped) &&
           (Text.Size == BytesWritten))
        {
            File->Offset += BytesWritten;
        }
        else
        {
            Assert(!"Write failure");
            File->NoErrors = false;
        }
    }
}

void
Win32CloseFile(platform_file *File)
{
    HANDLE Win32Handle = *(HANDLE *)&File->Handle;
    if(Win32Handle != INVALID_HANDLE_VALUE)
    {
        CloseHandle(Win32Handle);
    }
}

u64
Win32GetOSTimerFrequency()
{
    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);

    u64 Result = Frequency.QuadPart;
    return Result;
}

u64
Win32ReadOSTimer()
{
    LARGE_INTEGER PerformanceCount;
    QueryPerformanceCounter(&PerformanceCount);

    u64 Result = PerformanceCount.QuadPart;
    return Result;
}

u64
Win32ReadOSPageFaultCount()
{
    PROCESS_MEMORY_COUNTERS_EX MemoryCounters = 
    {
        .cb = sizeof(PROCESS_MEMORY_COUNTERS_EX),
    };
    GetProcessMemoryInfo(GlobalMemory.ProcessHandle, (PROCESS_MEMORY_COUNTERS *)&MemoryCounters, sizeof(MemoryCounters));

    u64 Result = MemoryCounters.PageFaultCount;
    return Result;
}
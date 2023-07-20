#include <Windows.h>

internal void
Win32ConsoleOut_(char *Format, va_list ArgList)
{
    u8 Buffer[4096] = {0};
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    AppendFormatString_(&StringState, Format, ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    DWORD NumberOfBytesWritten;
    WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&NumberOfBytesWritten, 0);
    Assert(NumberOfBytesWritten == Text.Size);

    
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

typedef struct win32_memory_block
{
    platform_memory_block PlatformBlock;
    struct win32_memory_block *Prev;
    struct win32_memory_block *Next;
    u64 Padding;
} win32_memory_block;

typedef struct win32_state
{
    ticket_mutex MemoryMutex;
    win32_memory_block MemorySentinel;

} win32_state;

global win32_state GlobalWin32State;

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
    
    win32_memory_block *Sentinel = &GlobalWin32State.MemorySentinel;
    Win32Block->Next = Sentinel;
    Win32Block->PlatformBlock.Size = Size;
    Win32Block->PlatformBlock.Flags = Flags;
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Win32Block->Prev = Sentinel->Prev;
    Win32Block->Prev->Next = Win32Block;
    Win32Block->Next->Prev = Win32Block;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    platform_memory_block *Result = &Win32Block->PlatformBlock;
    return Result;
}

void
Win32DeallocateMemory(platform_memory_block *Block)
{
    win32_memory_block *Win32Block = (win32_memory_block *)Block;
    
    BeginTicketMutex(&GlobalWin32State.MemoryMutex);
    Win32Block->Prev->Next = Win32Block->Next;
    Win32Block->Next->Prev = Win32Block->Prev;
    EndTicketMutex(&GlobalWin32State.MemoryMutex);
    
    b32 IsFreed = VirtualFree(Win32Block, 0, MEM_RELEASE);
    if(!IsFreed)
    {
        InvalidCodePath;
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

    DWORD DesiredAccess;
    DWORD CreationDisposition;

    if(Flags & FileAccess_Read)
    {
        DesiredAccess |= GENERIC_READ;
        CreationDisposition = OPEN_EXISTING;
    }
    
    if(Flags & FileAccess_Write)
    {
        DesiredAccess |= GENERIC_WRITE;
        CreationDisposition = OPEN_ALWAYS;
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
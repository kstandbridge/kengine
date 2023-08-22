internal void
LinuxConsoleOut_(char *Format, va_list ArgList)
{
    u8 Buffer[4096] = {0};
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    AppendFormatString_(&StringState, Format, ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);

    write(1, Text.Data, Text.Size);
}

void
LinuxConsoleOut(char *Format, ...)
{
    va_list ArgList;
    va_start(ArgList, Format);
    
    LinuxConsoleOut_(Format, ArgList);
    
    va_end(ArgList);
    
}

typedef enum linux_memory_block_flag
{
    MemoryBlockFlag_AllocatedDuringLooping = 0x1,
    MemoryBlockFlag_FreedDuringLooping = 0x2,
} linux_memory_block_flag;

typedef struct linux_memory_block
{
    platform_memory_block PlatformBlock;
    struct linux_memory_block *Prev;
    struct linux_memory_block *Next;
    u64 LoopingFlags;
} linux_memory_block;

typedef struct linux_saved_memory_block
{
    u64 BasePointer;
    u64 Size;
} linux_saved_memory_block;

typedef struct global_memory
{
    ticket_mutex MemoryMutex;
    linux_memory_block MemorySentinel;
    
    s32 RecordingHandle;
    s32 InputRecordingIndex;

    s32 PlaybackHandle;
    s32 InputPlayingIndex;

    char ExeFilePathBuffer[MAX_PATH];
    string ExeFilePath;
    string ExeDirectoryPath;
} global_memory;

global global_memory GlobalMemory;

internal void
InitGlobalMemory()
{
    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;

    ssize_t ExePathSize = readlink("/proc/self/exe", GlobalMemory.ExeFilePathBuffer, ArrayCount(GlobalMemory.ExeFilePathBuffer) - 1);
    if (ExePathSize > 0)
    {
        GlobalMemory.ExeFilePath = String_(ExePathSize, (u8 *)GlobalMemory.ExeFilePathBuffer);

        char *LastSlash = GlobalMemory.ExeFilePathBuffer;
        for(char *At = GlobalMemory.ExeFilePathBuffer;
            *At;
            ++At)
        {
            if(*At == '/')
            {
                LastSlash = At;
            }
        }

        GlobalMemory.ExeDirectoryPath = String_((umm)((LastSlash + 1) - GlobalMemory.ExeFilePathBuffer), (u8 *)GlobalMemory.ExeFilePathBuffer);
    }
}

internal b32
LinuxIsInLoop()
{
    b32 Result = ((GlobalMemory.InputRecordingIndex != 0) ||
                  (GlobalMemory.InputPlayingIndex));
    return Result;
}

platform_memory_block *
LinuxAllocateMemory(umm Size, u64 Flags)
{
    // NOTE(kstandbridge): Cache align
    Assert(sizeof(linux_memory_block) == 64);
    
    umm PageSize = sysconf(_SC_PAGESIZE);
    umm TotalSize = Size + sizeof(linux_memory_block);
    umm BaseOffset = sizeof(linux_memory_block);
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
    
    linux_memory_block *LinuxBlock = (linux_memory_block *)
        mmap(0, TotalSize, PROT_READ|PROT_WRITE,
             MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    Assert(LinuxBlock);
    LinuxBlock->PlatformBlock.Base = (u8 *)LinuxBlock + BaseOffset;
    Assert(LinuxBlock->PlatformBlock.Used == 0);
    Assert(LinuxBlock->PlatformBlock.Prev == 0);
    
    if(Flags & (PlatformMemoryBlockFlag_UnderflowCheck|PlatformMemoryBlockFlag_OverflowCheck))
    {
        s32 Error = mprotect((u8 *)LinuxBlock + ProtectOffset, PageSize, PROT_NONE);
        Assert(Error == 0);
    }
    
    linux_memory_block *Sentinel = &GlobalMemory.MemorySentinel;
    LinuxBlock->Next = Sentinel;
    LinuxBlock->PlatformBlock.Size = Size;
    LinuxBlock->PlatformBlock.Flags = Flags;
    LinuxBlock->LoopingFlags = 0;
    if(LinuxIsInLoop(&GlobalMemory) && !(Flags & PlatformMemoryBlockFlag_NotRestored))
    {
        LinuxBlock->LoopingFlags = MemoryBlockFlag_AllocatedDuringLooping;
    }
        
    BeginTicketMutex(&GlobalMemory.MemoryMutex);
    LinuxBlock->Prev = Sentinel->Prev;
    LinuxBlock->Prev->Next = LinuxBlock;
    LinuxBlock->Next->Prev = LinuxBlock;
    EndTicketMutex(&GlobalMemory.MemoryMutex);
    
    platform_memory_block *Result = &LinuxBlock->PlatformBlock;
    return Result;
}

internal void
LinuxFreeMemoryBlock(linux_memory_block *LinuxBlock)
{
    u32 Size = LinuxBlock->PlatformBlock.Size;
    u64 Flags = LinuxBlock->PlatformBlock.Flags;
    umm PageSize = sysconf(_SC_PAGESIZE);
    umm TotalSize = Size + sizeof(linux_memory_block);
    if(Flags & PlatformMemoryBlockFlag_UnderflowCheck)
    {
        TotalSize = Size + 2*PageSize;
    }
    else if(Flags & PlatformMemoryBlockFlag_OverflowCheck)
    {
        umm SizeRoundedUp = AlignPow2(Size, PageSize);
        TotalSize = SizeRoundedUp + 2*PageSize;
    }
    
    BeginTicketMutex(&GlobalMemory.MemoryMutex);
    LinuxBlock->Prev->Next = LinuxBlock->Next;
    LinuxBlock->Next->Prev = LinuxBlock->Prev;
    EndTicketMutex(&GlobalMemory.MemoryMutex);
    
    munmap(LinuxBlock, TotalSize);
}

void
LinuxDeallocateMemory(platform_memory_block *PlatformBlock)
{
    if(PlatformBlock)
    {
        linux_memory_block *LinuxBlock = ((linux_memory_block *)PlatformBlock);
        if(LinuxIsInLoop() && !(LinuxBlock->PlatformBlock.Flags & PlatformMemoryBlockFlag_NotRestored))
        {
            LinuxBlock->LoopingFlags = MemoryBlockFlag_FreedDuringLooping;
        }
        else
        {
            LinuxFreeMemoryBlock(LinuxBlock);
        }
    }
}

internal void
LinuxClearBlocksByMask(u64 Mask)
{
    for(linux_memory_block *BlockIter = GlobalMemory.MemorySentinel.Next;
        BlockIter != &GlobalMemory.MemorySentinel;
        )
    {
        linux_memory_block *Block = BlockIter;
        BlockIter = BlockIter->Next;
        
        if((Block->LoopingFlags & Mask) == Mask)
        {
            LinuxFreeMemoryBlock(Block);
        }
        else
        {
            Block->LoopingFlags = 0;
        }
    }
}

internal void
LinuxBeginInputPlayBack(s32 InputPlayingIndex)
{
    LinuxClearBlocksByMask(MemoryBlockFlag_AllocatedDuringLooping);
    
    char FileName[MAX_PATH];
    FormatStringToBuffer((u8 *)FileName, sizeof(FileName), "%S\\loop_edit_%d_input.kni",
                         GlobalMemory.ExeDirectoryPath, InputPlayingIndex);
                         
    GlobalMemory.PlaybackHandle = open(FileName, O_RDONLY);
    if(GlobalMemory.PlaybackHandle >= 0)
    {
        GlobalMemory.InputPlayingIndex = InputPlayingIndex;
        
        for(;;)
        {
            linux_saved_memory_block Block = {};
            ssize_t BytesRead = read(GlobalMemory.PlaybackHandle, &Block, sizeof(Block));
            if(Block.BasePointer != 0)
            {
                void *BasePointer = (void *)Block.BasePointer;
                Assert(Block.Size <= U32Max);
                BytesRead = read(GlobalMemory.PlaybackHandle, BasePointer, (u32)Block.Size);
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
LinuxEndInputPlayBack()
{
    LinuxClearBlocksByMask(MemoryBlockFlag_FreedDuringLooping);
    close(GlobalMemory.PlaybackHandle);
    GlobalMemory.InputPlayingIndex = 0;
}

internal void
LinuxBeginRecordingInput(s32 InputRecordingIndex)
{
    char FileName[MAX_PATH];
    FormatStringToBuffer((u8 *)FileName, sizeof(FileName), "%S\\loop_edit_%d_input.kni",
                         GlobalMemory.ExeDirectoryPath, InputRecordingIndex);
    
    GlobalMemory.RecordingHandle = open(FileName, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if(GlobalMemory.RecordingHandle >= 0)
    {
        GlobalMemory.InputRecordingIndex = InputRecordingIndex;
        linux_memory_block *Sentinel = &GlobalMemory.MemorySentinel;
        
        BeginTicketMutex(&GlobalMemory.MemoryMutex);
        for(linux_memory_block *SourceBlock = Sentinel->Next;
            SourceBlock != Sentinel;
            SourceBlock = SourceBlock->Next)
        {
            if(!(SourceBlock->PlatformBlock.Flags & PlatformMemoryBlockFlag_NotRestored))
            {
                linux_saved_memory_block DestBlock;
                void *BasePointer = SourceBlock->PlatformBlock.Base;
                DestBlock.BasePointer = (u64)BasePointer;
                DestBlock.Size = SourceBlock->PlatformBlock.Size;
                write(GlobalMemory.RecordingHandle, &DestBlock, sizeof(DestBlock));
                Assert(DestBlock.Size <= U32Max);
                write(GlobalMemory.RecordingHandle, BasePointer, (u32)DestBlock.Size);
            }
        }
        EndTicketMutex(&GlobalMemory.MemoryMutex);
        
        linux_saved_memory_block DestBlock = {0};
        write(GlobalMemory.RecordingHandle, &DestBlock, sizeof(DestBlock));
    }
}

internal void
LinuxRecordInput(app_input *NewInput)
{
    ssize_t BytesWritten = write(GlobalMemory.RecordingHandle, NewInput, sizeof(*NewInput));
    Assert(BytesWritten == sizeof(*NewInput));
}

internal void
LinuxEndRecordingInput()
{
    syncfs(GlobalMemory.RecordingHandle);
    close(GlobalMemory.RecordingHandle);
    GlobalMemory.InputRecordingIndex = 0;
}

internal void
LinuxPlayBackInput(app_input *NewInput)
{
    ssize_t BytesRead = read(GlobalMemory.PlaybackHandle, NewInput, sizeof(*NewInput));
    if(BytesRead == 0)
    {
        // NOTE(kstandbridge): Hit end of stream, loop
        s32 PlayingIndex = GlobalMemory.InputPlayingIndex;
        LinuxEndInputPlayBack();
        LinuxBeginInputPlayBack(PlayingIndex);
        read(GlobalMemory.PlaybackHandle, NewInput, sizeof(*NewInput));
    }
    else
    {
        Assert(BytesRead == sizeof(*NewInput));
    }
}

// TODO(kstandbridge): PlatformDirectoryExists and Win32DirectoryExists
b32
LinuxDirectoryExists(string Path)
{
    b32 Result;

    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);

    DIR *Directory = opendir(CPath);

    Result = (Directory != 0);

    return Result;
}

b32
LinuxFileExists(string Path)
{
    b32 Result;

    char CPath[MAX_PATH];
    StringToCString(Path, MAX_PATH, CPath);

    Result = (access(CPath, F_OK) == 0);

    return Result;
}

string
LinuxReadEntireFile(memory_arena *Arena, string FilePath)
{
    string Result = {0};
    
    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);

    struct stat FileData;
    u32 StatError = stat(CFilePath, &FileData);
    if(StatError == 0)
    {
        Result.Size = (umm)FileData.st_size;
        Result.Data = (u8 *)PushSize(Arena, Result.Size);
        if(Result.Data)
        {
            s32 FileHandle = open(CFilePath, O_RDONLY, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
            
            if(FileHandle >= 0)
            {
                s64 Offset = 0;
                ssize_t BytesRead = pread(FileHandle, Result.Data, Result.Size, Offset);
            
                close(FileHandle);
                Assert(Result.Size == (umm)BytesRead);
            }
            else
            {
                Assert(!"Failed to open file");
            }

        }
        else
        {
            Assert(!"Failed to allocate data");
        }
    }
    else
    {
        Assert(!"Failed to get file data");
    }

    return Result;
}

b32
LinuxWriteTextToFile(string Text, string FilePath)
{
    b32 Result = false;
    
    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);

    s32 FileHandle = open(CFilePath, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    
    if(FileHandle >= 0)
    {
        s64 Offset = 0;
        ssize_t BytesWritten = pwrite(FileHandle, Text.Data, Text.Size, Offset);
        
        close(FileHandle);

        Result = (Text.Size == (umm)BytesWritten);
        Assert(Result);   
    }
    else
    {
        Assert(!"Failed to open file");
    }

    return Result;
}

platform_file
LinuxOpenFile(string FilePath, platform_file_access_flags Flags)
{
    platform_file Result = {0};

    s32 OFlag = 0;
    if((Flags & FileAccess_Read) &&
       (Flags & FileAccess_Write))
    {
        OFlag = O_RDWR | O_CREAT;
    }
    else if(Flags & FileAccess_Read)
    {
        OFlag = O_RDONLY;
    }
    else if(Flags & FileAccess_Write)
    {
        OFlag = O_RDWR | O_CREAT | O_TRUNC;
    }

    char CFilePath[MAX_PATH];
    StringToCString(FilePath, MAX_PATH, CFilePath);

    s32 Handle = open(CFilePath, OFlag, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    
    if(Handle >= 0)
    {
        *(s32 *)&Result.Handle = Handle;
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
LinuxWriteFile(platform_file *File, string Text)
{
    if(PlatformNoErrors(File))
    {
        s32 LinuxHandle = *(s32 *)&File->Handle;

        ssize_t BytesWritten = pwrite(LinuxHandle, Text.Data, Text.Size, File->Offset);
        if(Text.Size == (umm)BytesWritten)
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
LinuxCloseFile(platform_file *File)
{
    s32 LinuxHandle = *(s32 *)&File->Handle;
    if(LinuxHandle >= 0)
    {
        close(LinuxHandle);
    }
}

u64
LinuxGetOSTimerFrequency()
{
    return 1000000;
}

u64
LinuxReadOSTimer()
{
    struct timeval Value;
    gettimeofday(&Value, 0);

    u64 Result = LinuxGetOSTimerFrequency()*(u64)Value.tv_sec + (u64)Value.tv_usec;
    return Result;
}
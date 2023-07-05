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
    LinuxMem_AllocatedDuringLooping = 0x1,
    LinuxMem_FreedDuringLooping     = 0x2,
} linux_memory_block_flag;
typedef struct linux_memory_block
{
    platform_memory_block Block;
    struct linux_memory_block *Prev;
    struct linux_memory_block *Next;
    u64 Padding;
} linux_memory_block;

typedef struct linux_state
{
    ticket_mutex MemoryMutex;
    linux_memory_block MemorySentinel;
} linux_state;

global linux_state GlobalLinuxState;

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
    
    linux_memory_block *Block = (linux_memory_block *)
        mmap(0, TotalSize, PROT_READ|PROT_WRITE,
             MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
    Assert(Block);
    Block->Block.Base = (u8 *)Block + BaseOffset;
    Assert(Block->Block.Used == 0);
    Assert(Block->Block.Prev == 0);
    
    if(Flags & (PlatformMemoryBlockFlag_UnderflowCheck|PlatformMemoryBlockFlag_OverflowCheck))
    {
        s32 Error = mprotect((u8 *)Block + ProtectOffset, PageSize, PROT_NONE);
        Assert(Error == 0);
    }
    
    linux_memory_block *Sentinel = &GlobalLinuxState.MemorySentinel;
    Block->Next = Sentinel;
    Block->Block.Size = Size;
    Block->Block.Flags = Flags;
        
    BeginTicketMutex(&GlobalLinuxState.MemoryMutex);
    Block->Prev = Sentinel->Prev;
    Block->Prev->Next = Block;
    Block->Next->Prev = Block;
    EndTicketMutex(&GlobalLinuxState.MemoryMutex);
    
    platform_memory_block *PlatBlock = &Block->Block;
    return PlatBlock;
}

void
LinuxDeallocateMemory(platform_memory_block *Block)
{
    linux_memory_block *LinuxBlock = ((linux_memory_block *)Block);
    
    u32 Size = LinuxBlock->Block.Size;
    u64 Flags = LinuxBlock->Block.Flags;
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
    
    BeginTicketMutex(&GlobalLinuxState.MemoryMutex);
    LinuxBlock->Prev->Next = LinuxBlock->Next;
    LinuxBlock->Next->Prev = LinuxBlock->Prev;
    EndTicketMutex(&GlobalLinuxState.MemoryMutex);
    
    munmap(LinuxBlock, TotalSize);
}
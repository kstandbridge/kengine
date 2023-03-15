
internal void
ZeroSize(u64 Size, void *Ptr)
{
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}
arena_push_params
DefaultArenaPushParams()
{
    arena_push_params Result;
    Result.Flags = ArenaPushFlag_ClearToZero;
    Result.Alignment = 4;
    return Result;
}


internal umm
GetAlignmentOffset(memory_arena *Arena, umm Alignment)
{
    umm Result = 0;
    
    umm ResultPointer = (umm)Arena->CurrentBlock->Base + Arena->CurrentBlock->Used;
    umm AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        Result = Alignment - (ResultPointer & AlignmentMask);
    }
    
    return Result;
}

void *
PushSize_(memory_arena *Arena, umm SizeInit, arena_push_params Params)
{
    void *Result = 0;
    
    Assert(Params.Alignment <= 128);
    Assert(IsPow2(Params.Alignment));
    
    // NOTE(kstandbridge): Only one can be enabled at a time
#if 0
    Arena->AllocationFlags = PlatformMemoryBlockFlag_OverflowCheck;
#endif
    
#if 0
    Arena->AllocationFlags = PlatformMemoryBlockFlag_UnderflowCheck;
#endif
    
    umm Size = 0;
    if(Arena->CurrentBlock)
    {
        Size = SizeInit;
        umm AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
        Size += AlignmentOffset;
    }
    
    if(!Arena->CurrentBlock ||
       ((Arena->CurrentBlock->Used + Size) > Arena->CurrentBlock->Size))
    {
        Size = SizeInit; // NOTE(kstandbridge): The base will be aligned automatically
        if(Arena->AllocationFlags &(PlatformMemoryBlockFlag_UnderflowCheck|
                                    PlatformMemoryBlockFlag_OverflowCheck))
        {
            Arena->MinimumBlockSize = 0;
            Size = AlignPow2(Size, Params.Alignment);
        }
        else if(!Arena->MinimumBlockSize)
        {
            // NOTE(kstandbridge): Raymond Chen on page sizes https://devblogs.microsoft.com/oldnewthing/20210510-00/?p=105200
            Arena->MinimumBlockSize = Megabytes(2);
        }
        
        umm BlockSize = Maximum(Size, Arena->MinimumBlockSize);
        
        platform_memory_block *NewBlock = PlatformAllocateMemory(BlockSize, Arena->AllocationFlags);
        NewBlock->Prev = Arena->CurrentBlock;
        Arena->CurrentBlock = NewBlock;
        // TODO(kstandbridge): Debug block allocation event
    }
    
    Assert((Arena->CurrentBlock->Used + Size) <= Arena->CurrentBlock->Size);
    
    umm AlignmentOffset = GetAlignmentOffset(Arena, Params.Alignment);
    umm OffsetInBlock = Arena->CurrentBlock->Used + AlignmentOffset;
    Result = Arena->CurrentBlock->Base + OffsetInBlock;
    Arena->CurrentBlock->Used += Size;
    
    Assert(Size >= SizeInit);
    
    // NOTE(kstandbridge): Incase the first allocation was greater than the page alignment
    Assert(Arena->CurrentBlock->Used <= Arena->CurrentBlock->Size);
    
    if(Params.Flags & ArenaPushFlag_ClearToZero)
    {
        ZeroSize(SizeInit, Result);
    }
    
    // TODO(kstandbridge): Debug allocation event
    
    return Result;
}

void *
Copy(umm Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(Size--) {*Dest++ = *Source++;}
    
    return DestInit;
}

internal void
FreeLastArenaBlock(memory_arena *Arena)
{
    platform_memory_block *Free = Arena->CurrentBlock;
    // TODO(kstandbridge): Debug event block free
    Arena->CurrentBlock = Free->Prev;
    PlatformDeallocateMemory(Free);
}

temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;
    
    Result.Arena = Arena;
    Result.Block = Arena->CurrentBlock;
    Result.Used = Arena->CurrentBlock ? Arena->CurrentBlock->Used : 0;
    
    ++Arena->TempCount;
    
    return(Result);
}

void
EndTemporaryMemory(temporary_memory MemoryFlush)
{
    memory_arena *Arena = MemoryFlush.Arena;
    while(Arena->CurrentBlock != MemoryFlush.Block)
    {
        FreeLastArenaBlock(Arena);
    }
    
    if(Arena->CurrentBlock)
    {
        Assert(Arena->CurrentBlock->Used >= MemoryFlush.Used);
        Arena->CurrentBlock->Used = MemoryFlush.Used;
        // TODO(kstandbridge): Debug event block truncate?
    }
    
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

void
ClearArena(memory_arena *Arena)
{
    while(Arena->CurrentBlock)
    {
        // NOTE(kstandbridge): Can't look at ArenaBlock properies after freeing
        b32 IsFinalBlock = (Arena->CurrentBlock->Prev == 0);
        FreeLastArenaBlock(Arena);
        if(IsFinalBlock)
        {
            break;
        }
    }
}

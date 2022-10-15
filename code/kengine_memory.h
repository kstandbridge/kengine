#ifndef KENGINE_MEMORY_H

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(u64 Size, void *Ptr)
{
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}

typedef struct memory_arena
{
    platform_memory_block *CurrentBlock;
    umm MinimumBlockSize;
    u32 AllocationFlags;
    u32 TempCount;
} memory_arena;

typedef struct temporary_memory
{
    memory_arena *Arena;
    platform_memory_block *Block;
    umm Used;
} temporary_memory;

inline umm
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


inline void *
Copy(umm Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(Size--) {*Dest++ = *Source++;}
    
    return DestInit;
}

typedef enum arena_push_flags
{
    ArenaPushFlag_ClearToZero = 0x1
} arena_push_flags;

typedef struct arena_push_params
{
    u32 Flags;
    u32 Alignment;
} arena_push_params;

inline arena_push_params
DefaultArenaPushParams()
{
    arena_push_params Result;
    Result.Flags = ArenaPushFlag_ClearToZero;
    Result.Alignment = 4;
    return Result;
}

inline arena_push_params
NoClearArenaPushParams()
{
    arena_push_params Result = DefaultArenaPushParams();
    Result.Flags &= ~ArenaPushFlag_ClearToZero;
    return Result;
}

#define MEMORY_ALIGNMENT 4
#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type), DefaultArenaPushParams())
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type), DefaultArenaPushParams())
#define PushSize(Arena, Size) PushSize_(Arena, Size, DefaultArenaPushParams())
#define PushCopy(Arena, Size, Source) Copy(Size, Source, PushSize_(Arena, Size, DefaultArenaPushParams()))
inline void *
PushSize_(memory_arena *Arena, umm SizeInit, arena_push_params Params)
{
    void *Result = 0;
    
    Assert(Params.Alignment <= 128);
    Assert(IsPow2(Params.Alignment));
    
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
        
        platform_memory_block *NewBlock = Platform.AllocateMemory(BlockSize, Arena->AllocationFlags);
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


inline u8 *
BeginPushSize(memory_arena *Arena)
{
    // TODO(kstandbridge): Lots of copy/pasta from PushSize_ here, also setting alignment to 4?
    if(!Arena->CurrentBlock)
    {
        if(!Arena->MinimumBlockSize)
        {
            // NOTE(kstandbridge): Raymond Chen on page sizes https://devblogs.microsoft.com/oldnewthing/20210510-00/?p=105200
            Arena->MinimumBlockSize = Megabytes(2);
        }
        
        umm BlockSize = Arena->MinimumBlockSize;
        
        platform_memory_block *NewBlock = Platform.AllocateMemory(BlockSize, Arena->AllocationFlags);
        NewBlock->Prev = Arena->CurrentBlock;
        Arena->CurrentBlock = NewBlock;
    }
    
    umm AlignmentOffset = GetAlignmentOffset(Arena, 4);
    
    u8 *Result = Arena->CurrentBlock->Base + Arena->CurrentBlock->Used + AlignmentOffset;
    ++Arena->TempCount;
    
    return Result;
}

inline void
EndPushSize(memory_arena *Arena, umm Size)
{
    --Arena->TempCount;
    PushSize_(Arena, Size, NoClearArenaPushParams());
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;
    
    Result.Arena = Arena;
    Result.Block = Arena->CurrentBlock;
    Result.Used = Arena->CurrentBlock ? Arena->CurrentBlock->Used : 0;
    
    ++Arena->TempCount;
    
    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    while(Arena->CurrentBlock != TempMem.Block)
    {
        platform_memory_block *Free = Arena->CurrentBlock;
        // TODO(kstandbridge): Debug event block free
        Arena->CurrentBlock = Free->Prev;
        Platform.DeallocateMemory(Free);
    }
    
    if(Arena->CurrentBlock)
    {
        Assert(Arena->CurrentBlock->Used >= TempMem.Used);
        Arena->CurrentBlock->Used = TempMem.Used;
        // TODO(kstandbridge): Debug event block truncate?
    }
    
    Assert(Arena->TempCount > 0);
    --Arena->TempCount;
}

inline string
BeginPushString(memory_arena *Arena)
{
    string Result;
    
    umm AlignmentOffset = GetAlignmentOffset(Arena, 4);
    Result.Data = Arena->CurrentBlock->Base + Arena->CurrentBlock->Used + AlignmentOffset;
    Result.Size = 1;
    
    ++Arena->TempCount;
    
    return Result;
}

inline void
EndPushString(string *Text, memory_arena *Arena, umm Size)
{
    Assert((Arena->CurrentBlock->Used + Size) <= Arena->CurrentBlock->Size);
    
    Text->Size = Size;
    Arena->CurrentBlock->Used += Size;
    
    --Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

#define KENGINE_MEMORY_H
#endif //KENGINE_MEMORY_H

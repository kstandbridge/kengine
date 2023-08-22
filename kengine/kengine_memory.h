#ifndef KENGINE_MEMORY_H
#define KENGINE_MEMORY_H

typedef enum platform_memory_block_flags
{
    PlatformMemoryBlockFlag_NotRestored = 0x1,
    PlatformMemoryBlockFlag_OverflowCheck = 0x2,
    PlatformMemoryBlockFlag_UnderflowCheck = 0x4,
} platform_memory_block_flags;

typedef struct platform_memory_block
{
    u64 Flags;
    u64 Size;
    u8 *Base;
    umm Used;
    
    struct platform_memory_block *Prev;
} platform_memory_block;

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

temporary_memory
BeginTemporaryMemory(memory_arena *Arena);

void
EndTemporaryMemory(temporary_memory MemoryFlush);

inline void
CheckArena(memory_arena *Arena)
{
    if(Arena)
    {
        Assert(Arena->TempCount == 0);
    }
    else
    {
        InvalidCodePath;
    }
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

arena_push_params DefaultArenaPushParams();

arena_push_params
NoClearArenaPushParams();

#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type), DefaultArenaPushParams())
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type), DefaultArenaPushParams())
#define PushSize(Arena, Size) PushSize_(Arena, Size, DefaultArenaPushParams())
#define PushCopy(Arena, Size, Source) Copy(Size, Source, PushSize_(Arena, Size, DefaultArenaPushParams()))
void *
PushSize_(memory_arena *Arena, umm SizeInit, arena_push_params Params);

#define BootstrapPushStruct(type, Member) (type *)BootstrapPushsize_(sizeof(type), OffsetOf(type, Member), DefaultArenaPushParams())
internal void *
BootstrapPushsize_(umm StructSize, umm OffsetToArena, arena_push_params Params)
{
    memory_arena Arena = {0};
    
    void *Result = PushSize_(&Arena, StructSize, Params);
    *(memory_arena *)((u8 *)Result + OffsetToArena) = Arena;
    
    return Result;
}

void *
Copy(umm Size, void *SourceInit, void *DestInit);

void
ClearArena(memory_arena *Arena);

#endif //KENGINE_MEMORY_H

#ifndef KENGINE_MEMORY_H

typedef struct memory_arena
{
    u64 Size;
    u8 *Base;
    umm Used;
    
    s32 TempCount;
} memory_arena;

inline void
InitializeArena(memory_arena *Arena, u64 Size, void *Base)
{
    Arena->Size = Size;
    Arena->Base = (u8 *)Base;
    Arena->Used = 0;
    Arena->TempCount = 0;
}

inline umm
GetAlignmentOffset(memory_arena *Arena, umm Alignment)
{
    umm AlignmentOffset = 0;
    
    umm ResultPointer = (umm)Arena->Base + Arena->Used;
    umm AlignmentMask = Alignment - 1;
    if(ResultPointer & AlignmentMask)
    {
        AlignmentOffset = Alignment - (ResultPointer & AlignmentMask);
    }
    
    return(AlignmentOffset);
}


#define PushStruct(Arena, type) (type *)PushSize_(Arena, sizeof(type), 4)
#define PushArray(Arena, Count, type) (type *)PushSize_(Arena, (Count)*sizeof(type), 4)
#define PushSize(Arena, Size) PushSize_(Arena, Size, 4)
#define PushCopy(Arena, Size, Source) Copy(Size, Source, PushSize_(Arena, Size, 4))
inline void *
PushSize_(memory_arena *Arena, umm SizeInit, umm Alignment)
{
    umm Size = SizeInit;
    
    umm AlignmentOffset = GetAlignmentOffset(Arena, Alignment);
    Size += AlignmentOffset;
    
    Assert((Arena->Used + Size) <= Arena->Size);
    void *Result = Arena->Base + Arena->Used + AlignmentOffset;
    Arena->Used += Size;
    
    Assert(Size >= SizeInit);
    
    return(Result);
}

#define KENGINE_MEMORY_H
#endif //KENGINE_MEMORY_H

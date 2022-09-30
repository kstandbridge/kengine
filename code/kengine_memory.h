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

typedef struct
{
    u64 Size;
    u8 *Base;
    umm Used;
    
    s32 TempCount;
} memory_arena;

typedef struct temporary_memory
{
    memory_arena *Arena;
    umm Used;
} temporary_memory;

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
    
    return AlignmentOffset;
}


inline void *
Copy(umm Size, void *SourceInit, void *DestInit)
{
    u8 *Source = (u8 *)SourceInit;
    u8 *Dest = (u8 *)DestInit;
    while(Size--) {*Dest++ = *Source++;}
    
    return DestInit;
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
    
    return Result;
}


inline u8 *
BeginPushSize(memory_arena *Arena)
{
    umm AlignmentOffset = GetAlignmentOffset(Arena, 4);
    
    u8 *Result = Arena->Base + Arena->Used + AlignmentOffset;
    ++Arena->TempCount;
    
    return Result;
}

inline void
EndPushSize(memory_arena *Arena, umm Size)
{
    --Arena->TempCount;
    PushSize_(Arena, Size, 4);
}


inline void
SubArena(memory_arena *Result, memory_arena *Arena, u64 Size)
{
    Result->Size = Size;
    Result->Base = (u8 *)PushSize_(Arena, Size, 16);
    Result->Used = 0;
    Result->TempCount = 0;
}

inline temporary_memory
BeginTemporaryMemory(memory_arena *Arena)
{
    temporary_memory Result;
    
    Result.Arena = Arena;
    Result.Used = Arena->Used;
    
    ++Arena->TempCount;
    
    return(Result);
}

inline void
EndTemporaryMemory(temporary_memory TempMem)
{
    memory_arena *Arena = TempMem.Arena;
    Assert(Arena->Used >= TempMem.Used);
    ZeroSize(Arena->Used - TempMem.Used, Arena->Base + TempMem.Used);
    Arena->Used = TempMem.Used;
    Assert(Arena->TempCount > 0);
    
    
    --Arena->TempCount;
}

inline string
BeginPushString(memory_arena *Arena)
{
    string Result;
    
    umm AlignmentOffset = GetAlignmentOffset(Arena, 4);
    Result.Data = Arena->Base + Arena->Used + AlignmentOffset;
    Result.Size = 1;
    
    ++Arena->TempCount;
    
    return Result;
}

inline void
EndPushString(string *Text, memory_arena *Arena, umm Size)
{
    Assert((Arena->Used + Size) <= Arena->Size);
    
    Text->Size = Size;
    Arena->Used += Size;
    
    --Arena->TempCount;
}

inline void
CheckArena(memory_arena *Arena)
{
    Assert(Arena->TempCount == 0);
}

inline b32
ArenaHasRoomFor(memory_arena *Arena, umm Size)
{
    
    umm AlignmentOffset = GetAlignmentOffset(Arena, 4);
    Size += AlignmentOffset;
    b32 Result = ((Arena->Used + Size) <= Arena->Size);
    return Result;
}

#define KENGINE_MEMORY_H
#endif //KENGINE_MEMORY_H

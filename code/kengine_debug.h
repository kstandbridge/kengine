#ifndef KENGINE_DEBUG_H

// TODO(kstandbridge): Remove all these and replace with introspection
#define DLIST_INSERT(Sentinel, Element)         \
(Element)->Next = (Sentinel)->Next;         \
(Element)->Prev = (Sentinel);               \
(Element)->Next->Prev = (Element);          \
(Element)->Prev->Next = (Element);
#define DLIST_REMOVE(Element)         \
if((Element)->Next) \
{ \
(Element)->Next->Prev = (Element)->Prev;         \
(Element)->Prev->Next = (Element)->Next;         \
(Element)->Next = (Element)->Prev = 0; \
}
#define DLIST_INSERT_AS_LAST(Sentinel, Element)         \
(Element)->Next = (Sentinel);               \
(Element)->Prev = (Sentinel)->Prev;         \
(Element)->Next->Prev = (Element);          \
(Element)->Prev->Next = (Element);

#define DLIST_INIT(Sentinel) \
(Sentinel)->Next = (Sentinel); \
(Sentinel)->Prev = (Sentinel)

#define DLIST_IS_EMPTY(Sentinel) \
((Sentinel)->Next == (Sentinel))

#define FREELIST_ALLOCATE(Result, FreeListPointer, AllocationCode)             \
(Result) = (FreeListPointer); \
if(Result) {FreeListPointer = (Result)->NextFree;} else {Result = AllocationCode;}
#define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
if(Pointer) {(Pointer)->NextFree = (FreeListPointer); (FreeListPointer) = (Pointer);}



#define DEBUG_FRAME_COUNT 256
typedef struct debug_profile_node
{
    struct debug_element *Element;
    struct debug_stored_event *FirstChild;
    struct debug_stored_event *NextSameParent;
    u64 Duration;
    u64 DurationOfChildren;
    u64 ParentRelativeClock;
    u32 ThreadOrdinal;
} debug_profile_node;

typedef struct debug_stored_event
{
    union
    {
        struct debug_stored_event *Next;
        struct debug_stored_event *NextFree;
    };
    
    u32 FrameIndex;
    
    union
    {
        debug_event Event;
        debug_profile_node ProfileNode;
    };
} debug_stored_event;

typedef struct debug_frame
{
    u64 BeginClock;
    u64 EndClock;
    f32 SecondsElapsed;
    
    u32 FrameIndex;
    
    u32 StoredEventCount;
    u32 ProfileBlockCount;
    
    debug_stored_event *RootProfileNode;
    
} debug_frame;

typedef struct debug_element_frame
{
    debug_stored_event *OldestEvent;
    debug_stored_event *MostRecentEvent;
} debug_element_frame;

typedef struct debug_parsed_name
{
    u32 HashValue;
    string FileName;
    u32 LineNumber;
    string Name;
    
    string GUID;
} debug_parsed_name;

typedef struct debug_element
{
    debug_parsed_name ParsedName;
    
    debug_event_type Type;
    
    b32 ValueWasEdited;
    
    debug_element_frame Frames[DEBUG_FRAME_COUNT];
    
    struct debug_element *NextInHash;
} debug_element;

typedef struct debug_variable_link
{
    struct debug_variable_link *Next;
    struct debug_variable_link *Prev;
    
    struct debug_variable_link *FirstChild;
    struct debug_variable_link *LastChild;
    
    string Name;
    debug_element *Element;
    
} debug_variable_link;

typedef struct open_debug_block
{
    union
    {
        struct open_debug_block *Parent;
        struct open_debug_block *NextFree;
    };
    
    u32 StartingFrameIndex;
    debug_element *Element;
    u64 BeginClock;
    debug_stored_event *Node;
    
} open_debug_block;

typedef struct debug_thread
{
    union
    {
        struct debug_thread *Next;
        struct debug_thread *NextFree;
    };
    
    u32 Id;
    u32 LaneIndex;
    open_debug_block *FirstOpenCodeBlock;
    
} debug_thread;

typedef struct debug_clock_entry
{
    debug_element *Element;
    
    f64 Min;
    f64 Max;
    f64 Sum;
    f64 Avg;
    u32 Count;
} debug_clock_entry;


inline u32
F32ToRadixValue(f32 Value)
{
    u32 Result = *(u32 *)&Value;
    if(Result & 0x80000000)
    {
        Result = ~Result;
    }
    else
    {
        Result |= 0x80000000;
    }
    return(Result);
}

typedef enum sort_type
{
    Sort_Ascending,
    Sort_Descending
} sort_type;

internal void
DebugClockEntryRadixSort(u32 Count, debug_clock_entry *First, debug_clock_entry *Temp, sort_type SortType)
{
    debug_clock_entry *Source = First;
    debug_clock_entry *Dest = Temp;
    for(u32 ByteIndex = 0;
        ByteIndex < 32;
        ByteIndex += 8)
    {
        u32 SortKeyOffsets[256];
        ZeroArray(ArrayCount(SortKeyOffsets), SortKeyOffsets);
        
        // NOTE(kstandbridge): First pass - count how many of each key
        for(u32 Index = 0;
            Index < Count;
            ++Index)
        {
            u32 RadixValue;
            if(SortType == Sort_Descending)
            {
                RadixValue = F32ToRadixValue(-Source[Index].Sum);
            }
            else
            {
                Assert(SortType == Sort_Ascending);
                RadixValue = F32ToRadixValue(Source[Index].Sum);
            }
            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
            ++SortKeyOffsets[RadixPiece];
        }
        
        // NOTE(kstandbridge): Change counts to offsets
        u32 Total = 0;
        for(u32 SortKeyIndex = 0;
            SortKeyIndex < ArrayCount(SortKeyOffsets);
            ++SortKeyIndex)
        {
            u32 OffsetCount = SortKeyOffsets[SortKeyIndex];
            SortKeyOffsets[SortKeyIndex] = Total;
            Total += OffsetCount;
        }
        
        // NOTE(kstandbridge): Second pass - place elements into the right location
        for(u32 Index = 0;
            Index < Count;
            ++Index)
        {
            u32 RadixValue;
            if(SortType == Sort_Descending)
            {
                RadixValue = F32ToRadixValue(-Source[Index].Sum);
            }
            else
            {
                Assert(SortType == Sort_Ascending);
                RadixValue = F32ToRadixValue(Source[Index].Sum);
            }
            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;
            Dest[SortKeyOffsets[RadixPiece]++] = Source[Index];
        }
        
        debug_clock_entry *SwapTemp = Dest;
        Dest = Source;
        Source = SwapTemp;
    }
}


typedef struct debug_state
{
    memory_arena Arena;
    debug_variable_link *ProfileGroup;
    debug_variable_link *RootGroup;
    
    debug_thread *FirstThread;
    debug_thread *FirstFreeThread;
    open_debug_block *FirstFreeBlock;
    
    b32 Paused;
    
    u32 ViewingFrameOrdinal;
    
    u32 MostRecentFrameOrdinal;
    u32 CollationFrameOrdinal;
    u32 OldestFrameOrdinal;
    debug_element *ElementHash[1024];
    
    u32 TotalFrameCount;
    debug_frame Frames[DEBUG_FRAME_COUNT];
    u32 FrameBarLaneCount;
    
    debug_element *RootProfileElement;
    
    debug_stored_event *FirstFreeStoredEvent;
} debug_state;

#define KENGINE_DEBUG_H
#endif //KENGINE_DEBUG_H

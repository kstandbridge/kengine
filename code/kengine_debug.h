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



#define DEBUG_FRAME_COUNT 8
typedef struct debug_profile_node
{
    struct debug_element *Element;
    struct debug_stored_event *FirstChild;
    struct debug_stored_event *NextSameParent;
    u64 Duration;
    u64 DurationOfChildren;
    u64 ParentRelativeClock;
    u32 Reserved;
    u16 ThreadOrdinal;
    u16 CoreIndex;
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
    
    f32 FrameBarScale;
    
    u32 FrameIndex;
    
    u32 StoredEventCount;
    u32 ProfileBlockCount;
    u32 DataBlockCount;
    
    debug_stored_event *RootProfileNode;
    
} debug_frame;

typedef struct debug_element_frame
{
    debug_stored_event *OldestEvent;
    debug_stored_event *MostRecentEvent;
} debug_element_frame;

typedef struct debug_element
{
    char *OriginalGUID; // NOTE(kstandbridge): Might point to an unloaded DLL?
    string GUID;
    string Name;
    u32 FileNameCount;
    u32 LineNumber;
    u32 NameStartsAt;
    
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
    
    // TODO(kstandbridge): Check if only for data blocks.
    debug_variable_link *Group;
    
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
    //open_debug_block *FirstOpenDataBlock;
    
} debug_thread;

typedef struct debug_statistic
{
    f64 Min;
    f64 Max;
    f64 Sum;
    f64 Avg;
    u32 Count;
} debug_statistic;

typedef struct debug_clock_entry
{
    debug_element *Element;
    debug_statistic Stat;
} debug_clock_entry;

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
    
    // TODO(kstandbridge): What is this per frame stuff?
    debug_stored_event *FirstFreeStoredEvent;
} debug_state;

#define KENGINE_DEBUG_H
#endif //KENGINE_DEBUG_H

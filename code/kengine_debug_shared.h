#ifndef KENGINE_DEBUG_SHARED_H


#if KENGINE_INTERNAL
typedef enum
{
    DebugEvent_None,
    
    DebugEvent_FrameEnd,
    
} debug_event_type;

typedef struct
{
    char *GUID;
    u64 Clock;
    u16 ThreadId;
    u16 Type;
    union
    {
        b32 Value_B32;
        s32 Value_S32;
        u32 Value_U32;
        f32 Value_F32;
    };
    
} debug_event;

typedef struct debug_event_table
{
    u32 RecordIncrement;
    
    u32 volatile EventIndex;
    debug_event Events[32];
} debug_event_table;

#define SetDebugEventRecording(Enabled) (GlobalDebugEventTable->RecordIncrement = (Enabled) ? 1 : 0)

#define GenerateDebugId__(File, Line, Counter, Name) File "|" #Line "|" #Counter "|" Name
#define GenerateDebugId_(File, Line, Counter, Name) GenerateDebugId__(File, Line, Counter, Name)
#define GenerateDebugId(Name) GenerateDebugId_(__FILE__, __LINE__, __COUNTER__, Name)
#define PushDebugEvent(EventType, GUIDInit)  \
u32 EventIndex = AtomicAddU32(&GlobalDebugEventTable->EventIndex, GlobalDebugEventTable->RecordIncrement); \
Assert(EventIndex < ArrayCount(GlobalDebugEventTable->Events)); \
debug_event *Event = GlobalDebugEventTable->Events + EventIndex; \
Event->GUID = GUIDInit; \
Event->Clock = (u64)__rdtsc(); \
Event->ThreadId = (u16)GetThreadID(); \
Event->Type = (u16)EventType;

#define DEBUG_FRAME_END(SecondsElapsed) \
{ \
PushDebugEvent(DebugEvent_FrameEnd, GenerateDebugId("Frame End")); \
Event->Value_F32 = SecondsElapsed; \
}
#else

#define DEBUG_FRAME_END(...)

#endif


#define KENGINE_DEBUG_SHARED_H
#endif //KENGINE_DEBUG_SHARED_H

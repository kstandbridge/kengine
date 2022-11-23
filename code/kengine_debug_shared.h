#ifndef KENGINE_DEBUG_SHARED_H


#if KENGINE_INTERNAL

typedef enum debug_event_type
{
    DebugEvent_None,
    
    DebugEvent_FrameEnd,
    
    DebugEvent_BeginBlock,
    DebugEvent_EndBlock,
    
} debug_event_type;

typedef struct
{
    char *GUID;
    u64 Clock;
    u16 ThreadId;
    
    u8 Type;
    union
    {
        b32 Value_B32;
        s32 Value_S32;
        u32 Value_U32;
        f32 Value_F32;
    };
    
} debug_event;

typedef struct debug_settings
{
    b32 ForceSoftwareRendering;
    b32 ShowUIElementOutlines;
    b32 ShowDebugTab;
} debug_settings;

typedef struct debug_event_table
{
    debug_settings Settings;
    
    u32 RecordIncrement;
    
    u32 CurrentEventArrayIndex;
    
    u64 volatile EventArrayIndex_EventIndex;
    debug_event Events[2][65535];
} debug_event_table;

#define SetDebugEventRecording(Enabled) (GlobalDebugEventTable->RecordIncrement = (Enabled) ? 1 : 0)

#define GenerateDebugId__(File, Line, Counter, Name) File "|" #Line "|" #Counter "|" Name
#define GenerateDebugId_(File, Line, Counter, Name) GenerateDebugId__(File, Line, Counter, Name)
#define GenerateDebugId(Name) GenerateDebugId_(__FILE__, __LINE__, __COUNTER__, Name)

#define PushDebugEvent(EventType, GUIDInit)  \
u64 ArrayIndex_EventIndex = AtomicAddU64(&GlobalDebugEventTable->EventArrayIndex_EventIndex, GlobalDebugEventTable->RecordIncrement); \
u32 EventIndex = ArrayIndex_EventIndex & 0xFFFFFFFF; \
Assert(EventIndex < ArrayCount(GlobalDebugEventTable->Events[0])); \
debug_event *Event = GlobalDebugEventTable->Events[ArrayIndex_EventIndex >> 32] + EventIndex; \
Event->GUID = GUIDInit; \
Event->Clock = (u64)__rdtsc(); \
Event->ThreadId = (u16)GetThreadID(); \
Event->Type = (u16)EventType;

#define DEBUG_FRAME_END(SecondsElapsed) \
{ \
PushDebugEvent(DebugEvent_FrameEnd, GenerateDebugId("Frame End")); \
Event->Value_F32 = SecondsElapsed; \
}
#define BEGIN_BLOCK(GUID) {PushDebugEvent(DebugEvent_BeginBlock, GenerateDebugId(GUID)); }
#define END_BLOCK() {PushDebugEvent(DebugEvent_EndBlock, GenerateDebugId("END_BLOCK_")); }

#define DEBUG_IF(B32) if(GlobalDebugEventTable && GlobalDebugEventTable->Settings.B32)

#else // KENGINE_INTERNAL

#define DEBUG_FRAME_END(...)
#define BEGIN_BLOCK(...)
#define END_BLOCK()

#define DEBUG_IF(...) if(0)

#endif // KENGINE_INTERNAL


#define KENGINE_DEBUG_SHARED_H
#endif //KENGINE_DEBUG_SHARED_H

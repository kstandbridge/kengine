#ifndef KENGINE_PLATFORM_H

#include <stdarg.h>
#include <immintrin.h>

#define introspect(...)
#include "kengine_types.h"

#if KENGINE_SLOW
#define Assert(Expression) if(!(Expression)) {__debugbreak();}
#else
#define Assert(...)
#endif

#include "kengine_memory.h"

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline u32 
AtomicCompareExchangeU32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);
    
    return Result;
}

inline u32
AtomicIncrementU32(u32 volatile *Value)
{
    u32 Result = _InterlockedIncrement((long volatile *)Value);
    
    return Result;
}

inline u32 
AtomicExchange(u32 volatile *Value, u32 New)
{
    u32 Result = (u32)_InterlockedExchange((long volatile *)Value, New);
    
    return Result;
}

inline u64 
AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = (u64)_InterlockedExchange64((__int64 volatile *)Value, New);
    
    return Result;
}

inline u32 
AtomicAddU32(u32 volatile *Value, u32 Addend)
{
    u32 Result = (u32)_InterlockedExchangeAdd((long volatile *)Value, Addend);
    
    return Result;
}    

inline u64 
AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    u64 Result = (u64)_InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);
    
    return Result;
}    

inline u32 
GetThreadID()
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);
    
    return ThreadID;
}

typedef void platform_work_queue_callback(void *Data);

typedef struct platform_work_queue platform_work_queue;
typedef void platform_add_work_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);
typedef loaded_glyph get_glyph_for_code_point(memory_arena *Arena, u32 CodePoint);
typedef f32 get_horizontal_advance(u32 PrevCodePoint, u32 CodePoint);
typedef f32 get_verticle_advance();

typedef struct
{
    s32 HalfTransitionCount;
    b32 EndedDown;
} app_button_state;

typedef enum
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Extended0,
    MouseButton_Extended1,
    
    MouseButton_Count,
} mouse_button_type;

typedef struct
{
    app_button_state MouseButtons[MouseButton_Count];
    f32 MouseX;
    f32 MouseY;
    f32 MouseZ;
} app_input;

inline b32 
WasPressed(app_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
    return Result;
}

typedef struct app_state app_state;
typedef struct debug_state debug_state;
typedef struct
{
    app_state *AppState;
    
    struct platform_work_queue *PerFrameWorkQueue;
    struct platform_work_queue *BackgroundWorkQueue;
    
    platform_add_work_entry *AddWorkEntry;
    platform_complete_all_work *CompleteAllWork;
    
    get_glyph_for_code_point *GetGlyphForCodePoint;
    get_horizontal_advance *GetHorizontalAdvance;
    get_verticle_advance *GetVerticleAdvance;
    
#if KENGINE_INTERNAL
    debug_state *DebugState;
    struct debug_event_table *DebugEventTable;
    b32 DllReloaded;
#endif
    
} platform_api;

typedef struct render_commands render_commands;
typedef void app_update_frame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input);

typedef void debug_update_frame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input);

#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

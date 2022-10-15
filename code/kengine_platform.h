#ifndef KENGINE_PLATFORM_H

#include <stdarg.h>
#include <immintrin.h>

#define introspect(...)
#include "kengine_types.h"


typedef enum platform_memory_block_flags
{
    PlatformMemoryBlockFlag_OverflowCheck = 0x1,
    PlatformMemoryBlockFlag_UnderflowCheck = 0x2
} platform_memory_block_flags;

typedef struct platform_memory_block
{
    u64 Flags;
    u64 Size;
    u8 *Base;
    umm Used;
    
    struct platform_memory_block *Prev;
} platform_memory_block;

typedef void platform_work_queue_callback(void *Data);

typedef struct platform_work_queue platform_work_queue;
typedef void platform_add_work_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);
typedef loaded_glyph get_glyph_for_code_point(struct memory_arena *Arena, u32 CodePoint);
typedef f32 get_horizontal_advance(u32 PrevCodePoint, u32 CodePoint);
typedef f32 get_verticle_advance();

typedef struct app_button_state
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

typedef struct app_input
{
    f32 dtForFrame;
    
    app_button_state MouseButtons[MouseButton_Count];
    f32 MouseX;
    f32 MouseY;
    f32 MouseZ;
    
    b32 ShiftDown, AltDown, ControlDown;
    b32 FKeyPressed[13];
} app_input;

inline b32 
WasPressed(app_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
    return Result;
}

typedef platform_memory_block *platform_allocate_memory(umm Size, u64 Flags);
typedef void platform_deallocate_memory(platform_memory_block *Block);

typedef struct platform_memory_stats
{
    u32 BlockCount;
    umm TotalSize;
    umm TotalUsed;
} platform_memory_stats;

typedef platform_memory_stats platform_get_memory_stats();

typedef struct platform_api
{
    platform_add_work_entry *AddWorkEntry;
    platform_complete_all_work *CompleteAllWork;
    
    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
    platform_get_memory_stats *GetMemoryStats;
    
    get_glyph_for_code_point *GetGlyphForCodePoint;
    get_horizontal_advance *GetHorizontalAdvance;
    get_verticle_advance *GetVerticleAdvance;
    
} platform_api;
extern platform_api Platform;

typedef struct app_memory
{
    // TODO(kstandbridge): App state maybe, UI state hard no
    struct app_state *AppState;
    struct ui_state *UIState;
    
    // TODO(kstandbridge): Remove work queues?
    struct platform_work_queue *PerFrameWorkQueue;
    struct platform_work_queue *BackgroundWorkQueue;
    
#if KENGINE_INTERNAL
    struct debug_state *DebugState;
    struct debug_event_table *DebugEventTable;
#endif
    platform_api PlatformAPI;
} app_memory;

typedef struct render_commands render_commands;
typedef void app_update_frame(app_memory *Memory, render_commands *Commands, struct memory_arena *Arena, app_input *Input);
typedef void debug_update_frame(app_memory *Memory, render_commands *Commands, struct memory_arena *Arena, app_input *Input);

#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

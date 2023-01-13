#ifndef KENGINE_H

#include "kengine_platform.h"
#include "kengine_memory.h"
#if KENGINE_INTERNAL
#include "kengine_generated.h"
#endif
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#if KENGINE_INTERNAL
#include "kengine_debug_shared.h"
#include "kengine_debug.h"
#include "kengine_debug_generated.h"
#endif
#include "kengine_render_group.h"

typedef struct app_state
{
    memory_arena Arena;
    
    u32 SomeValue;
    
} app_state;

typedef struct control_element
{
    v2 Size;
    v3 Offset;
} control_element;

typedef struct ui_grid
{
    rectangle2 Bounds;
    u32 Columns;
    u32 Rows;
    
    f32 ColumnWidth;
    f32 RowHeight;
    
    struct ui_grid *Prev;
} ui_grid;

typedef struct ui_frame
{
    memory_arena *Arena;
    render_group *RenderGroup;
    app_input *Input;
    ui_grid *CurrentGrid;
} ui_frame;

typedef struct ui_state
{
    // NOTE(kstandbridge): Persistent
    // Interactions
    
    
    // NOTE(kstandbridge): Transient
    ui_frame *Frame;
} ui_state;

#define KENGINE_H
#endif //KENGINE_H
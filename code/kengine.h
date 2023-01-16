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
    
    s32 SomeValue;
    
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

typedef union ui_id_value
{
    void *Void;
    u64 U64;
    u32 U32[2];
} ui_id_value;

typedef struct ui_id
{
    ui_id_value Value[2];
} ui_id;

#define GenerateUIId__(Ptr, File, Line) GenerateUIId___(Ptr, File "|" #Line)
#define GenerateUIId_(Ptr, File, Line) GenerateUIId__(Ptr, File, Line)
#define GenerateUIId(Ptr) GenerateUIId_((Ptr), __FILE__, __LINE__)
inline ui_id
GenerateUIId___(void *Ptr, char *Text)
{
    ui_id Result =
    {
        .Value = 
        {
            Ptr,
            Text
        },
    };
    
    return Result;
}

typedef enum ui_interaction_type
{
    UI_Interaction_None,
    
    UI_Interaction_NOP,
    
    UI_Interaction_ImmediateButton,
    
} ui_interaction_type;

typedef struct ui_interaction
{
    ui_id Id;
    ui_interaction_type Type;
    
    void *Target;
    union
    {
        void *Generic;
        v2 *P;
    };
} ui_interaction;

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
    memory_arena Arena;
    
    v2 MouseP;
    v2 dMouseP;
    v2 LastMouseP;
    
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
    // NOTE(kstandbridge): Transient
    ui_frame *Frame;
} ui_state;

inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = UI_Interaction_None;
    Interaction->Generic = 0;
}

inline b32
InteractionsAreEqual(ui_interaction A, ui_interaction B)
{
    b32 Result = ((A.Id.Value[0].U64 == B.Id.Value[0].U64) &&
                  (A.Id.Value[1].U64 == B.Id.Value[1].U64) &&
                  (A.Type == B.Type) &&
                  (A.Target == B.Target) &&
                  (A.Generic == B.Generic));
    
    return Result;
}

inline b32
InteractionIsHot(ui_state *UiState, ui_interaction B)
{
    b32 Result = InteractionsAreEqual(UiState->HotInteraction, B);
    
    if(B.Type == UI_Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

#define KENGINE_H
#endif //KENGINE_H
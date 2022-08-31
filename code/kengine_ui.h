#ifndef KENGINE_UI_H

typedef enum
{
    Interaction_None,
    
    Interaction_NOP,
    
    Interaction_Select,
    
    Interaction_ImmediateButton,
    Interaction_Draggable,
    Interaction_SetU32,
    
} interaction_type;

#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B) FILE_AND_LINE__(A, B)
#define FILE_AND_LINE FILE_AND_LINE_(__FILE__, __LINE__)
#define SIZE_AUTO 0.0f

typedef union
{
    void *Ptr;
    u64 U64;
    u32 U32[2];
} ui_interaction_id_value;

typedef struct
{
    ui_interaction_id_value Value[2];
} ui_interaction_id;

#define InteractionIdFromPtr(Ptr) InteractionIdFromPtr_((Ptr), (char *)(FILE_AND_LINE))
internal ui_interaction_id
InteractionIdFromPtr_(void *Ptr, char *Text)
{
    ui_interaction_id Result;
    
    Result.Value[0].Ptr = Ptr;
    Result.Value[1].Ptr = Text;
    
    return Result;
}

#define InteractionIdFromU32s(A, B) InteractionIdFromU32s_((A), (B), (char *)(FILE_AND_LINE))
internal ui_interaction_id
InteractionIdFromU32s_(u32 A, u32 B, char *Text)
{
    ui_interaction_id Result;
    
    Result.Value[0].U32[0] = A;
    Result.Value[0].U32[1] = B;
    Result.Value[1].Ptr = Text;
    
    return Result;
}

typedef struct
{
    ui_interaction_id Id;
    interaction_type Type;
    
    void *Target;
    union
    {
        void *Generic;
        void *Ptr;
        v2 *P;
        u32 U32_Value;
    };
} ui_interaction;

typedef struct ui_state
{
    memory_arena TranArena;
    
    loaded_glyph Glyphs[255];
    
#if KENGINE_INTERNAL
    b32 ShowDebugTab;
#endif
    
    f32 LineAdvance;
    
    v2 MouseP;
    v2 dMouseP;
    v2 LastMouseP;
    b32 MouseDown;
    
    ui_interaction_id ClickedId;
    
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
} ui_state;

typedef struct ui_column
{
    f32 Width;
    
    rectangle2 Bounds;
    
    struct ui_column *Next;
} ui_column;

typedef struct ui_row
{
    f32 Height;
    
    ui_column *FirstColumn;
    struct ui_row *Next;
} ui_row;

typedef struct
{
    ui_state *UIState;
    
    rectangle2 Bounds;
    u16 Rows;
    u16 Columns;
    b32 SizeInitialized;
    
    f32 Scale;
    f32 SpacingX;
    f32 SpacingY;
    
    ui_row *FirstRow;
} ui_grid;

typedef struct
{
    ui_grid *Grid;
    ui_interaction Interaction;
    b32 IsHot;
    rectangle2 Bounds;
    
    // TODO(kstandbridge): Allow to be resized
    v2 *Size;
} ui_element;

#define KENGINE_UI_H
#endif //KENGINE_UI_H

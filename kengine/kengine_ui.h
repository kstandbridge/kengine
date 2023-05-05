#ifndef KENGINE_UI_H

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
    
    UI_Interaction_Draggable,
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

typedef struct ui_state
{
    v2 MouseP;
    v2 dMouseP;
    v2 LastMouseP;
    
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
    ui_interaction SelectedInteration;
    
} ui_state;

typedef enum ui_interaction_state
{
    UIInteractionState_None,
    UIInteractionState_HotClicked,
    UIInteractionState_Hot,
    UIInteractionState_Selected
} ui_interaction_state;

typedef struct ui_grid
{
    rectangle2 Bounds;
    u32 Columns;
    u32 Rows;
    
    b32 GridSizeCalculated;
    f32 *ColumnWidths;
    f32 *RowHeights;
    
    f32 DefaultRowHeight;
    
    struct ui_grid *Prev;
} ui_grid;

typedef struct ui_frame
{
    memory_arena *Arena;
    
    ui_grid *CurrentGrid;
} ui_frame;

inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = UI_Interaction_None;
    Interaction->Target = 0;
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
InteractionIsHot(ui_state *State, ui_interaction B)
{
    b32 Result = InteractionsAreEqual(State->HotInteraction, B);
    
    if(B.Type == UI_Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline void
GridSetRowHeight(ui_frame *Frame, u32 Row, f32 Height)
{
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    Assert(Row <= Grid->Rows);
    
    // NOTE(kstandbridge): Sizes must be set before a control is drawn
    Assert(!Grid->GridSizeCalculated);
    
    Grid->RowHeights[Row] = Height;
}

inline void
GridSetColumnWidth(ui_frame *Frame, u32 Column, f32 Width)
{
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    Assert(Column <= Grid->Columns);
    
    // NOTE(kstandbridge): Sizes must be set before a control is drawn
    Assert(!Grid->GridSizeCalculated);
    
    Grid->ColumnWidths[Column] = Width;
}

ui_interaction_state
AddUIInteraction(ui_state *State, rectangle2 Bounds, ui_interaction Interaction);

ui_frame
BeginUI(ui_state *State, app_input *Input, memory_arena *Arena);

void
EndUI(ui_state *State, app_input *Input);

void
BeginGrid(ui_frame *Frame, rectangle2 Bounds, u32 Columns, u32 Rows);

void
EndGrid(ui_frame *Frame);

rectangle2
GridGetCellBounds(ui_frame *Frame, u32 Column, u32 Row, f32 Margin);

#define KENGINE_UI_H
#endif //KENGINE_UI_H

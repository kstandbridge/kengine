

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

typedef enum ui_interaction_state
{
    UIInteractionState_None,
    UIInteractionState_HotClicked,
    UIInteractionState_Hot,
    UIInteractionState_Selected
} ui_interaction_state;

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
InteractionIsHot(ui_state *State, ui_interaction B)
{
    b32 Result = InteractionsAreEqual(State->HotInteraction, B);
    
    if(B.Type == UI_Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

internal ui_interaction_state
AddUIInteraction(ui_state *State, rectangle2 Bounds, ui_interaction Interaction)
{
    ui_interaction_state Result = UIInteractionState_None;
    
    Assert(Interaction.Type);
    
    if(Rectangle2IsIn(Bounds, State->MouseP))
    {
        State->NextHotInteraction = Interaction;
    }
    
    if(InteractionsAreEqual(Interaction, State->Interaction) &&
       InteractionsAreEqual(Interaction, State->NextHotInteraction))
    {
        Result = UIInteractionState_HotClicked;
    }
    else if(InteractionsAreEqual(Interaction, State->HotInteraction))
    {
        Result = UIInteractionState_Hot;
    }
    else if(InteractionsAreEqual(Interaction, State->SelectedInteration))
    {
        Result = UIInteractionState_Selected;
    }
    else
    {
        Result = UIInteractionState_None;
    }
    
    return Result;
}

typedef struct ui_frame
{
    memory_arena *Arena;
    
    ui_grid *CurrentGrid;
} ui_frame;

internal ui_frame
BeginUI(ui_state *State, app_input *Input, memory_arena *Arena)
{
    ui_frame Result =
    {
        .Arena = Arena,
        .CurrentGrid = 0
    };
    
    
    State->LastMouseP = State->MouseP;
    State->MouseP = Input->MouseP;
    State->dMouseP = V2Subtract(State->MouseP, State->LastMouseP);
    
    
    return Result;
}

internal void
EndUI(ui_state *State, app_input *Input)
{
    State->ToExecute = State->NextToExecute;
    ClearInteraction(&State->NextToExecute);
    
    u32 TransistionCount = Input->MouseButtons[MouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[MouseButton_Left].EndedDown;
    if(TransistionCount % 2)
    {
        MouseButton = !MouseButton;
    }
    
    for(u32 TransitionIndex = 0;
        TransitionIndex <= TransistionCount;
        ++TransitionIndex)
    {
        b32 MouseMove = false;
        b32 MouseDown = false;
        b32 MouseUp = false;
        if(TransitionIndex == 0)
        {
            MouseMove = true;
        }
        else
        {
            MouseDown = MouseButton;
            MouseUp = !MouseButton;
        }
        
        b32 EndInteraction = false;
        
        switch(State->Interaction.Type)
        {
            case UI_Interaction_ImmediateButton:
            {
                if(MouseUp)
                {
                    State->NextToExecute = State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            case UI_Interaction_None:
            {
                State->HotInteraction = State->NextHotInteraction;
                if(MouseDown)
                {
                    State->Interaction = State->HotInteraction;
                }
            } break;
            
            default:
            {
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            } break;
        }
        
        if(MouseDown)
        {
            ClearInteraction(&State->SelectedInteration);
        }
        
        if(MouseUp)
        {
            State->SelectedInteration = State->Interaction;
        }
        
        if(EndInteraction)
        {
            ClearInteraction(&State->Interaction);
        }
        
        MouseButton = !MouseButton;
    }
    
    ClearInteraction(&State->NextHotInteraction);
}

internal void
BeginGrid(ui_frame *Frame, rectangle2 Bounds, u32 Columns, u32 Rows)
{
    memory_arena *Arena = Frame->Arena;
    
    ui_grid *Grid = PushStruct(Arena, ui_grid);
    Grid->Prev = Frame->CurrentGrid;
    Frame->CurrentGrid = Grid;
    
    Grid->Bounds = Bounds;
    Grid->Columns = Columns;
    Grid->Rows = Rows;
    
    Grid->GridSizeCalculated = false;
    Grid->ColumnWidths = PushArray(Arena, Columns, f32);
    for(u32 ColumnIndex = 0;
        ColumnIndex < Columns;
        ++ColumnIndex)
    {
        Grid->ColumnWidths[ColumnIndex] = 0.0f;
    }
    
    Grid->RowHeights = PushArray(Arena, Rows, f32);
    for(u32 RowIndex = 0;
        RowIndex < Rows;
        ++RowIndex)
    {
        Grid->RowHeights[RowIndex] = 0.0f;
    }
}

internal void
EndGrid(ui_frame *Frame)
{
    Assert(Frame->CurrentGrid);
    Frame->CurrentGrid = Frame->CurrentGrid->Prev;
}

internal void
GridSetRowHeight(ui_frame *Frame, u32 Row, f32 Height)
{
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    Assert(Row <= Grid->Rows);
    
    // NOTE(kstandbridge): Sizes must be set before a control is drawn
    Assert(!Grid->GridSizeCalculated);
    
    Grid->RowHeights[Row] = Height;
}

internal rectangle2
GridGetCellBounds(ui_frame *Frame, u32 Column, u32 Row)
{
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    if(!Grid->GridSizeCalculated)
    {
        v2 Dim = Rectangle2GetDim(Grid->Bounds);
        u32 ColumnsFilled = 0;
        f32 WidthUsed = 0.0f;
        for(u32 ColumnIndex = 0;
            ColumnIndex < Grid->Columns;
            ++ColumnIndex)
        {
            if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
            {
                ++ColumnsFilled;
            }
            else
            {
                WidthUsed += Grid->ColumnWidths[ColumnIndex];
            }
        }
        if(ColumnsFilled > 0)
        {
            f32 WidthRemaining = Dim.X - WidthUsed;
            f32 ColumnWidth = WidthRemaining / ColumnsFilled;
            
            for(u32 ColumnIndex = 0;
                ColumnIndex < Grid->Columns;
                ++ColumnIndex)
            {
                if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
                {
                    Grid->ColumnWidths[ColumnIndex] = ColumnWidth;
                }
            }
        }
        
        u32 RowsFilled = 0;
        f32 HeightUsed = 0.0f;
        for(u32 RowIndex = 0;
            RowIndex < Grid->Rows;
            ++RowIndex)
        {
            if(Grid->RowHeights[RowIndex] == 0.0f)
            {
                ++RowsFilled;
            }
            else
            {
                HeightUsed += Grid->RowHeights[RowIndex];
            }
        }
        if(RowsFilled > 0)
        {
            f32 HeightRemaining = Dim.Y - HeightUsed;
            f32 RowHeight = HeightRemaining / RowsFilled;
            
            for(u32 RowIndex = 0;
                RowIndex < Grid->Rows;
                ++RowIndex)
            {
                if(Grid->RowHeights[RowIndex] == 0.0f)
                {
                    Grid->RowHeights[RowIndex] = RowHeight;
                }
            }
        }
        
        Grid->GridSizeCalculated = true;
    }
    
    v2 Min = Grid->Bounds.Min;
    for(u32 ColumnIndex = 0;
        ColumnIndex < Column;
        ++ColumnIndex)
    {
        Min.X += Grid->ColumnWidths[ColumnIndex];
    }
    
    for(u32 RowIndex = 0;
        RowIndex < Row;
        ++RowIndex)
    {
        Min.Y += Grid->RowHeights[RowIndex];
    }
    
    v2 Max = V2Add(Min, V2(Grid->ColumnWidths[Column], Grid->RowHeights[Row]));
    
    rectangle2 Result = Rectangle2(Min, Max);
    return Result;
}

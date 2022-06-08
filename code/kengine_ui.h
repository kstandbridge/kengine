#ifndef KENGINE_UI_H

typedef enum ui_interaction_type
{
    UiInteraction_None,
    
    UiInteraction_NOP,
    
    UiInteraction_ImmediateButton,
    UiInteraction_Draggable,
    UiInteraction_TextInput,
    UiInteraction_EditableBool,
    UiInteraction_MultipleChoice,
    UiInteraction_MultipleChoiceOption,
    
} ui_interaction_type;

typedef struct ui_interaction
{
    u32 ID;
    ui_interaction_type Type;
    
    union
    {
        void *Generic;
        v2 *P;
        editable_string *Str;
        b32 *Bool;
    };
} ui_interaction;

typedef struct ui_state
{
    assets *Assets;
    
    v2 LastMouseP;
    
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
    ui_interaction SelectedInteraction;
    ui_interaction ClickedInteraction;
    
} ui_state;

typedef enum ui_element_type
{
    ElementType_Static,
    ElementType_Checkbox,
    ElementType_Slider,
    ElementType_Spacer,
    ElementType_TextBox,
    ElementType_DropDown,
    ElementType_Button,
} ui_element_type;

typedef enum ui_text_layout_type
{
    TextLayout_None,
    
    TextLayout_TopLeft,
    TextLayout_MiddleLeft,
    TextLayout_BottomLeft,
    
    TextLayout_TopMiddle,
    TextLayout_MiddleMiddle,
    TextLayout_BottomMiddle,
    
    TextLayout_Scrollable,
    
} ui_text_layout_type;

typedef struct ui_element
{
    struct ui_element *Prev;
    struct ui_element *Next;
    
    ui_element_type Type;
    ui_interaction Interaction;
    
    string Label;
    ui_text_layout_type LabelLayout;
    v2 Dim;
    v2 TextOffset;
    v2 MinDim;
    v2 MaxDim;
    b32 IsFloating;
} ui_element; 

typedef enum ui_layout_type
{
    LayoutType_Auto,
    LayoutType_Fill,
} ui_layout_type;

typedef struct ui_layout_column
{
    
    ui_layout_type Type;
    
    f32 ColumnWidth;
    
    f32 UsedWidth;
    s32 SpacerCount;
    
    v2 Dim;
    b32 HasRows;
    union
    {
        s32 ElementCount;
        s32 RowCount;
    };
    union
    {
        struct ui_layout_row *LastRow;
        ui_element *LastElement;
    };
    
    struct ui_layout_column *Parent;
    struct ui_layout_column *Next;
} ui_layout_column;

typedef struct ui_layout_row
{
    ui_layout_type Type;
    
    f32 MaxHeight;
    
    v2 Dim;
    s32 ColumnCount;
    ui_layout_column *LastColumn;
    
    struct ui_layout_row *Parent;
    struct ui_layout_row *Next;
} ui_layout_row;

typedef enum ui_div_type
{
    UI_Div_Horizontal,
    UI_Div_Vertical,
} ui_div_type;
typedef struct ui_div
{
    ui_div_type Type;
    
    v2 Dim;
    s32 ChildCount;
    struct ui_div *LastChild;
    struct ui_div *Parent;
    
    struct ui_div *Next;
} ui_div;

typedef struct ui_layout
{
    ui_state *State;
    memory_arena *Arena;
    
    f32 Scale;
    f32 MinRowHeight;
    
    v2 MouseP;
    v2 dMouseP;
    f32 Padding;
    
    loaded_bitmap *DrawBuffer;
    
    s32 FillRows;
    f32 UsedHeight;
    s32 FloatingElements;
    
    s32 RowCount;
    ui_layout_row *LastRow;
    
    ui_layout_row *CurrentRow;
    ui_layout_column *CurrentColumn;
    ui_element *CurrentElement;
    
    ui_div MainDiv;
    ui_div *CurrentDiv;
    
    
} ui_layout;


inline b32
InteractionsAreEqual(ui_interaction A, ui_interaction B)
{
    b32 Result = ((A.ID == B.ID) &&
                  (A.Type == B.Type) &&
                  (A.Generic == B.Generic));
    
    return Result;
}

inline b32
InteractionIsToExecute(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->ToExecute, A);
    
    if(A.Type == UiInteraction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsNextToExecute(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->NextToExecute, A);
    
    if(A.Type == UiInteraction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsHot(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->HotInteraction, A);
    
    if(A.Type == UiInteraction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsSelected(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->SelectedInteraction, A);
    
    if(A.Type == UiInteraction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsClicked(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->ClickedInteraction, A);
    
    if(A.Type == UiInteraction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32 
InteractionIsValid(ui_interaction *Interaction)
{
    b32 Result = (Interaction->Type != UiInteraction_None);
    
    return Result;
}

inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = UiInteraction_None;
    Interaction->Generic = 0;
}

#define KENGINE_UI_H
#endif //KENGINE_UI_H

#ifndef KENGINE_UI_H


typedef enum ui_interaction_type
{
    UiInteraction_None,
    
    UiInteraction_NOP,
    
    UiInteraction_ImmediateButton,
    UiInteraction_Draggable,
    UiInteraction_TextInput,
    
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
    ElementType_Button,
} ui_element_type;

typedef struct ui_element
{
    ui_element_type Type;
    ui_interaction Interaction;
    string Label;
    v2 Dim;
    v2 TextOffset;
    
    struct ui_element *Next;
} ui_element; 

typedef enum ui_layout_type
{
    LayoutType_Auto,
    LayoutType_Fill,
} ui_layout_type;

typedef struct ui_layout_row
{
    ui_layout_type Type;
    ui_element *LastElement;
    
    f32 UsedWidth;
    f32 MaxHeight;
    s32 SpacerCount;
    s32 ElementCount;
    
    struct ui_layout_row *Next;
} ui_layout_row;

typedef struct ui_layout
{
    ui_state *State;
    memory_arena *Arena;
    
    render_group *RenderGroup;
    
    b32 IsCreatingRow;
    
    f32 Scale;
    v2 MouseP;
    v2 dMouseP;
    f32 Padding;
    
    s32 FillRows;
    f32 UsedHeight;
    
    ui_layout_row *LastRow;
    
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

#ifndef KENGINE_UI_H

typedef enum ui_interaction_type
{
    Interaction_None,
    
    Interaction_NOP,
    
    Interaction_EditableBool,
    
} ui_interaction_type;

typedef struct ui_interaction
{
    u32 ID;
    ui_interaction_type Type;
    
    union
    {
        void *Generic;
        b32 *Bool;
    };
} ui_interaction;

typedef enum ui_element_type
{
    Element_Row,
    Element_Spacer,
    Element_Label,
    Element_Checkbox,
    
} ui_element_type;

typedef struct ui_element
{
    u32 Id;
    ui_element_type Type;
    ui_interaction Interaction;
    
    string Text;
    rectangle2 TextBounds;
    v2 TextOffset;
    
    v2 Dim;
    v2 UsedDim;
    
    s32 ChildCount;
    s32 SetWidthChildCount;
    
    struct ui_element *FirstChild;
    struct ui_element *Parent;
    
    struct ui_element *Next;
} ui_element;

typedef struct ui_layout
{
    memory_arena *Arena;
    assets *Assets;
    f32 Scale;
    f32 Padding;
    
    loaded_bitmap *DrawBuffer;
    
    s32 ChildCount;
    ui_element SentinalElement;
    ui_element *CurrentElement;
    
    u32 CurrentId;
    v2 MouseP;
    v2 dMouseP;
    
} ui_layout;

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
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsNextToExecute(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->NextToExecute, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsHot(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->HotInteraction, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsSelected(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->SelectedInteraction, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline b32
InteractionIsClicked(ui_state *State, ui_interaction A)
{
    b32 Result = InteractionsAreEqual(State->ClickedInteraction, A);
    
    if(A.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = Interaction_None;
    Interaction->Generic = 0;
}

#define KENGINE_UI_H
#endif //KENGINE_UI_H

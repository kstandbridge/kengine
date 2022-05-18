#ifndef KENGINE_UI_H


typedef enum ui_interaction_type
{
    UiInteraction_None,
    
    UiInteraction_NOP,
    
    UiInteraction_ImmediateButton,
    UiInteraction_Draggable,
    
} ui_interaction_type;

typedef struct ui_interaction
{
    u32 ID;
    ui_interaction_type Type;
    
    union
    {
        void *Generic;
        v2 *P;
    };
} ui_interaction;

typedef struct ui_state
{
    v2 LastMouseP;
    ui_interaction Interaction;
    
    ui_interaction HotInteraction;
    ui_interaction NextHotInteraction;
    
    ui_interaction ToExecute;
    ui_interaction NextToExecute;
    
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

#ifndef KENGINE_UI_H

typedef enum
{
    Interaction_None,
    
    Interaction_NOP,
    
    Interaction_Select,
    
    Interaction_ImmediateButton,
    Interaction_Draggable,
    
} interaction_type;

#define FILE_AND_LINE__(A, B) A "|" #B
#define FILE_AND_LINE_(A, B) FILE_AND_LINE__(A, B)
#define FILE_AND_LINE FILE_AND_LINE_(__FILE__, __LINE__)

typedef union
{
    void *Ptr;
    u64 U64;
    u32 U32[2];
} ui_interaction_id_value;

typedef union
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
    };
} ui_interaction;

typedef struct
{
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

typedef struct
{
    ui_state *UIState;
    
    v2 UpperLeftCorner;
    v2 At;
    f32 Scale;
    f32 SpacingX;
    f32 SpacingY;
    
    f32 NextYDelta;
    
} ui_frame;

typedef struct
{
    ui_frame *UIFrame;
    ui_interaction Interaction;
    b32 IsHot;
    v2 Dim;
    
    // TODO(kstandbridge): Allow to be resized
    v2 *Size;
    
    rectangle2 Bounds;
} ui_element;

#define KENGINE_UI_H
#endif //KENGINE_UI_H

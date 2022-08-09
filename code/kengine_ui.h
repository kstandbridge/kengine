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
} interaction_id_value;

typedef union
{
    interaction_id_value Value[2];
} interaction_id;

#define InteractionIdFromPtr(Ptr) InteractionIdFromPtr_((Ptr), (char *)(FILE_AND_LINE))
internal interaction_id
InteractionIdFromPtr_(void *Ptr, char *Text)
{
    interaction_id Result;
    
    Result.Value[0].Ptr = Ptr;
    Result.Value[1].Ptr = Text;
    
    return Result;
}

typedef struct
{
    interaction_id Id;
    interaction_type Type;
    
    void *Target;
    union
    {
        void *Generic;
        void *Ptr;
        v2 *P;
    };
} interaction;

typedef struct
{
    v2 MouseP;
    v2 dMouseP;
    v2 LastMouseP;
    
    
    interaction Interaction;
    
    interaction HotInteraction;
    interaction NextHotInteraction;
    
    interaction ToExecute;
    interaction NextToExecute;
    
} u_i_state;

#define KENGINE_UI_H
#endif //KENGINE_UI_H

#ifndef KENGINE_INPUT_H

typedef struct app_button_state
{
    s32 HalfTransitionCount;
    b32 EndedDown;
} app_button_state;

typedef enum mouse_button_type
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Extended0,
    MouseButton_Extended1,
    
    MouseButton_Count,
} mouse_button_type;

typedef struct app_input
{
    app_button_state MouseButtons[MouseButton_Count];
    v2 MouseP;
    f32 MouseWheel;
    
    b32 ShiftDown;
    b32 AltDown;
    b32 ControlDown;
    b32 FKeyPressed[13];
} app_input;

inline b32 
WasPressed(app_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
    return Result;
}


#define KENGINE_INPUT_H
#endif //KENGINE_INPUT_H

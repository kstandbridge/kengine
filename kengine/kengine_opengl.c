typedef struct window_dimension
{
    s32 Width;
    s32 Height;
} window_dimension;

typedef struct app_memory
{
    struct app_state *AppState;

    platform_api PlatformAPI;
} app_memory;

typedef struct offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
    s32 BytesPerPixel;
} offscreen_buffer;

typedef struct sound_output_buffer
{
    s32 SamplesPerSecond;
    s32 SampleCount;
    s16 *Samples;
} sound_output_buffer;

typedef struct button_state
{
    s32 HalfTransitionCount;
    b32 EndedDown;
} button_state;

typedef struct controller_input
{
    b32 IsConnected;
    b32 IsAnalog;
    f32 StickAverageX;
    f32 StickAverageY;

    union
    {
        button_state Buttons[12];

        struct
        {
            button_state MoveUp;
            button_state MoveDown;
            button_state MoveLeft;
            button_state MoveRight;

            button_state ActionUp;
            button_state ActionDown;
            button_state ActionLeft;
            button_state ActionRight;

            button_state LeftShoulder;
            button_state RightShoulder;

            button_state Back;
            button_state Start;

            button_state Terminator;
        };
    };
} controller_input;

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
    button_state MouseButtons[MouseButton_Count];
    s32 MouseX;
    s32 MouseY;

    controller_input Controllers[5];
} app_input;

inline controller_input *
GetController(app_input *Input, u32 ControllerIndex)
{
    Assert(ControllerIndex < ArrayCount(Input->Controllers));
    
    controller_input *Result = &Input->Controllers[ControllerIndex];


    return Result;
}

internal void
DebugDrawVertical(offscreen_buffer *Backbuffer,
                  s32 X, s32 Top, s32 Bottom, u32 Color)
{
    if(Top <= 0)
    {
        Top = 0;
    }

    if(Bottom > Backbuffer->Height)
    {
        Bottom = Backbuffer->Height;
    }
    
    if((X >= 0) && (X < Backbuffer->Width))
    {
        u8 *Pixel = ((u8 *)Backbuffer->Memory +
                        X*Backbuffer->BytesPerPixel +
                        Top*Backbuffer->Pitch);
        for(s32 Y = Top;
            Y < Bottom;
            ++Y)
        {
            *(u32 *)Pixel = Color;
            Pixel += Backbuffer->Pitch;
        }
    }
}

typedef void app_update_and_render(app_memory *AppMemory, app_input *Input, offscreen_buffer *Buffer);

typedef void app_get_sound_samples(app_memory *AppMemory, sound_output_buffer *SoundBuffer);

#define VK_W           25
#define VK_A           38
#define VK_S           39
#define VK_D           40
#define VK_Q           24
#define VK_E           26
#define VK_UP          111
#define VK_DOWN        116
#define VK_LEFT        113
#define VK_RIGHT       114
#define VK_ESCAPE      9
#define VK_ENTER       36
#define VK_SPACE       65
#define VK_P           33
#define VK_L           46

#define VK_SHIFT_MASK  0x01
#define VK_CTRL_MASK   0x04
#define VK_ALT_MASK    0x08

#define VK_F1          67
#define VK_F2          68
#define VK_F3          69
#define VK_F4          70
#define VK_F5          71
#define VK_F6          72
#define VK_F7          73
#define VK_F8          74
#define VK_F9          75
#define VK_F10         76
#define VK_F11         95
#define VK_F12         96

#define VK_LEFT_MOUSE   1
#define VK_RIGHT_MOUSE  3
#define VK_MIDDLE_MOUSE 2
#define VK_EXT1_MOUSE   8
#define VK_EXT2_MOUSE   9

#define MAX_JOYSTICKS                   8

#define XBOX_CONTROLLER_DEADZONE        5000

#define JOYSTICK_AXIS_LEFT_THUMB_X      0
#define JOYSTICK_AXIS_LEFT_THUMB_Y      1
#define JOYSTICK_AXIS_RIGHT_THUMB_X     2
#define JOYSTICK_AXIS_RIGHT_THUMB_Y     3
#define JOYSTICK_AXIS_RIGHT_TRIGGER     4
#define JOYSTICK_AXIS_LEFT_TRIGGER      5
#define JOYSTICK_AXIS_DPAD_X            6
#define JOYSTICK_AXIS_DPAD_Y            7

#define JOYSTICK_BUTTON_A               0
#define JOYSTICK_BUTTON_B               1
#define JOYSTICK_BUTTON_X               2
#define JOYSTICK_BUTTON_Y               3
#define JOYSTICK_BUTTON_LEFT_SHOULDER   4
#define JOYSTICK_BUTTON_RIGHT_SHOULDER  5
#define JOYSTICK_BUTTON_BACK            6
#define JOYSTICK_BUTTON_START           7
#define JOYSTICK_BUTTON_XBOX            8
#define JOYSTICK_BUTTON_LEFT_THUMB      9
#define JOYSTICK_BUTTON_RIGHT_THUMB     10

#define JOYSTICK_A                      0x0001
#define JOYSTICK_B                      0x0002
#define JOYSTICK_X                      0x0004
#define JOYSTICK_Y                      0x0008
#define JOYSTICK_LEFT_SHOULDER          0x0010
#define JOYSTICK_RIGHT_SHOULDER         0x0020
#define JOYSTICK_BACK                   0x0040
#define JOYSTICK_START                  0x0080
#define JOYSTICK_XBOX                   0x0100
#define JOYSTICK_LEFT_THUMB             0x0200
#define JOYSTICK_RIGHT_THUMB            0x0400

typedef struct linux_audio_buffer
{
    u32 Size;
    volatile u32 ReadIndex;
    volatile u32 WriteIndex;
    u8 *Data;

} linux_audio_buffer;

typedef struct linux_sound_output
{
    u32 SamplesPerSecond;
    u32 RunningSampleIndex;
    u32 LastReadIndex;
    u32 LatencySampleCount;
    u32 BytesPerSample;
    u32 SafetyBytes;
    
    linux_audio_buffer Buffer;

    s32 ToneHz;
    s16 ToneVolume;
    s32 WavePeriod;
    s32 SecondaryBufferSize;
    f32 tSine;
} linux_sound_output;

typedef struct linux_sound_player
{
    snd_pcm_t *PcmHandle;
    u32 SamplesPerSecond;
    volatile b32 IsPlaying;
    pthread_t Thread;
    snd_pcm_uframes_t PeriodSize;
    u32 BytesPerFrame;
    

    linux_sound_output Output;

} linux_sound_player;

typedef struct linux_debug_time_marker
{
    u32 OutputPlayCursor;
    u32 OutputWriteCursor;
    u32 OutputLocation;
    u32 OutputByteCount;
    u32 ExpectedFlipPlayCursor;
    
    u32 FlipPlayCursor;
    u32 FlipWriteCursor;
} linux_debug_time_marker;

typedef struct linux_joystick
{
    u8 IDs[MAX_JOYSTICKS];
    s32 FileDescriptors[MAX_JOYSTICKS + 1];
    s32 EventFileDescriptors[MAX_JOYSTICKS + 1];
    struct ff_effect Effect;
} linux_joystick;

typedef struct linux_app_code
{
    void *Library;
    ino_t LibraryFileId;

    app_update_and_render *UpdateAndRender;
    app_get_sound_samples *GetSoundSamples;

    b32 IsValid;
    
} linux_app_code;

typedef struct linux_state
{
    memory_arena Arena;
    b32 Running;
    b32 Pause;
    offscreen_buffer Backbuffer;

    Pixmap Pixmap;
    XImage *Image;

    linux_sound_player SoundPlayer;

    linux_joystick Joystick;
    
} linux_state;

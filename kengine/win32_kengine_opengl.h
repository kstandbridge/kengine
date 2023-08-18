
typedef struct win32_sound_output
{
    s32 SamplesPerSecond;
    u32 RunningSampleIndex;
    s32 BytesPerSample;
    s32 SecondaryBufferSize;
    s32 LatencySampleCount;
} win32_sound_output;

typedef struct win32_state
{
    memory_arena Arena;
    b32 Running;
    offscreen_buffer Backbuffer;

    BITMAPINFO Info;
    LPDIRECTSOUNDBUFFER SecondaryBuffer;
} win32_state;
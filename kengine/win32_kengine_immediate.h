
typedef struct win32_sound_output
{
    s32 SamplesPerSecond;
    u32 RunningSampleIndex;
    s32 BytesPerSample;
    DWORD SecondaryBufferSize;
    DWORD SafetyBytes;
    f32 tSine;
    s32 LatencySampleCount;
} win32_sound_output;

typedef struct win32_debug_time_marker
{
    DWORD OutputPlayCursor;
    DWORD OutputWriteCursor;
    DWORD OutputLocation;
    DWORD OutputByteCount;
    DWORD ExpectedFlipPlayCursor;
    
    DWORD FlipPlayCursor;
    DWORD FlipWriteCursor;
} win32_debug_time_marker;

typedef struct win32_app_code
{
    HMODULE Library;
    FILETIME LibraryLastWriteTime;
    
    app_update_and_render *UpdateAndRender;
    app_get_sound_samples *GetSoundSamples;

    b32 IsValid;
} win32_app_code;

typedef struct win32_state
{
    memory_arena Arena;
    b32 Running;
    b32 Pause;
    offscreen_buffer Backbuffer;

    BITMAPINFO Info;
    LPDIRECTSOUNDBUFFER SecondaryBuffer;

} win32_state;
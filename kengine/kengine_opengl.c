
typedef struct app_memory
{
    struct app_state *AppState;

    platform_api PlatformAPI;
} app_memory;

typedef void app_update_and_render(app_memory *AppMemory, app_input *Input, offscreen_buffer *Buffer);

typedef void app_get_sound_samples(app_memory *AppMemory, sound_output_buffer *SoundBuffer);

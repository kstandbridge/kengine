
typedef struct app_memory
{
    struct app_state *AppState;

    platform_api PlatformAPI;
} app_memory;

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

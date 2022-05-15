#ifndef KENGINE_PLATFORM_H

int _fltused = 0x9875;

#pragma function(memset)
void *memset(void *_Dst, int _Val, size_t _Size)
{
    unsigned char Val = *(unsigned char *)&_Val;
    unsigned char *At = (unsigned char *)_Dst;
    while(_Size--)
    {
        *At++ = Val;
    }
    return(_Dst);
}

#define introspect(...)
#include "kengine_types.h"

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}


#define BITMAP_BYTES_PER_PIXEL 4
typedef struct app_offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
} app_offscreen_buffer;

//
// NOTE(kstandbridge): Platform api
//
typedef string debug_read_entire_file(memory_arena *Arena, char *FilePath);

typedef loaded_bitmap debug_get_glyph_for_codepoint(memory_arena *Arena, u32 Codepoint);

typedef struct platform_api
{
    debug_read_entire_file *DEBUGReadEntireFile;
    debug_get_glyph_for_codepoint *DEBUGGetGlyphForCodepoint;
} platform_api;

//
// NOTE(kstandbridge): App api
//

typedef struct app_memory
{
    u64 StorageSize;
    void *Storage;
    
    platform_api PlatformAPI;
} app_memory;

typedef void app_update_and_render(app_memory *Memory, app_offscreen_buffer *Buffer, f32 DeltaTime);


#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

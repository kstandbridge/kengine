#ifndef WIN32_KENGINE_H

#include "kengine_platform.h"
#include "win32_kengine_types.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "kengine_renderer.h"

typedef struct win32_offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    
    s32 Width;
    s32 Height;
    s32 Pitch;
} win32_offscreen_buffer;

typedef struct win32_state
{
    b32 IsRunning;
    HWND Window;
    
    win32_offscreen_buffer Backbuffer;
} win32_state;


#define WIN32_KENGINE_H
#endif //WIN32_KENGINE_H

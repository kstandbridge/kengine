#ifndef WIN32_KENGINE_H

#include "kengine_platform.h"
#include "kengine_shared.h"
#include "kengine_generated.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "win32_kengine_shared.h"
#include "kengine_render_group.h"


typedef struct win32_offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    BITMAPINFO Info;
    void *Memory;
    
    s32 Width;
    s32 Height;
    s32 Pitch;
} win32_offscreen_buffer;

typedef struct app_code
{
    char EXEFileName[MAX_PATH];
    char *OnePastLastEXEFileNameSlash;
    
    HMODULE Library;
    FILETIME LastDLLWriteTime;
    
    app_update_frame *AppUpdateFrame;
} app_code;


#define WIN32_KENGINE_H
#endif //WIN32_KENGINE_H

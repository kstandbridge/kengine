#ifndef KENGINE_H

#ifndef VERSION
#define VERSION 0
#endif //VERSION

#ifdef KENGINE_IMPLEMENTATION
    #undef KENGINE_IMPLEMENTATION
    #define KENGINE_IMPLEMENTATION 1
#endif

#ifdef KENGINE_CONSOLE
    #undef KENGINE_CONSOLE
    #define KENGINE_CONSOLE 1
#endif

#if !defined(KEGNINE_WIN32)
    #define KEGNINE_WIN32 0
#else
    #undef KENGINE_WIN32
    #define KENGINE_WIN32 1
#endif

#if !defined(KENGINE_LINUX)
    #define KENGINE_LINUX 0
#else
    #undef KENGINE_LINUX
    #define KENGINE_LINUX 1
#endif

#include <stdarg.h>

#define introspect(...)
#include "kengine/kengine_types.h"
#include "kengine/kengine_memory.h"
#include "kengine/kengine_generated.h"
#include "kengine/kengine_math.h"
#include "kengine/kengine_string.h"

#if KENGINE_WIN32
    #include "kengine/kengine_win32.h"
#elif KENGINE_LINUX
    #include "kengine/kengine_linux.h"
#endif

#if KENGINE_IMPLEMENTATION
    #include "kengine/kengine_memory.c"
    #include "kengine/kengine_string.c"
    #if KENGINE_WIN32
        #include "kengine/kengine_win32.c"
    #elif KENGINE_LINUX
        #include "kengine/kengine_linux.c"
    #endif

#endif

#if KENGINE_CONSOLE

typedef struct app_memory
{
    struct app_state *AppState;

#if KENGINE_CONSOLE
    u32 ArgCount;
    char **Args;
#endif

} app_memory;
global app_memory GlobalAppMemory;

s32
MainLoop(app_memory *AppMemory);

s32
main(u32 ArgCount, char *Args[])
{
    s32 Result = 0;

    GlobalAppMemory.ArgCount = ArgCount - 1;
    GlobalAppMemory.Args = Args + 1;

#if KENGINE_WIN32
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
#elif KENGINE_LINUX
    GlobalLinuxState.MemorySentinel.Prev = &GlobalLinuxState.MemorySentinel;
    GlobalLinuxState.MemorySentinel.Next = &GlobalLinuxState.MemorySentinel;
#endif

    Result = MainLoop(&GlobalAppMemory);

    return Result;
}

#endif

#define KENGINE_H
#endif // KENGINE_H
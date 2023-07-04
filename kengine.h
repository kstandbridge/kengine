#ifndef KENGINE_LINUX_H

#ifdef KENGINE_IMPLEMENTATION
    #undef KENGINE_IMPLEMENTATION
    #define KENGINE_IMPLEMENTATION 1
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
#include "kengine/kengine_log.h"
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

#define KENGINE_LINUX_H
#endif // KENGINE_LINUX_H
#ifndef KENGINE_LINUX_H

#ifdef KENGINE_IMPLEMENTATION
    #undef KENGINE_IMPLEMENTATION
    #define KENGINE_IMPLEMENTATION 1
#endif

#if !defined(KEGNINE_WIN32)
    #define KEGINE_WIN32 0
#endif

#if !defined(KEGNINE_LINUX)
    #define KEGINE_LINUX 0
#endif

#if !KEGNINE_WIN32 && !KEGNINE_LINUX
    #if _MSC_VER
        #undef KENGINE_WIN32
        #define KENGINE_WIN32 1
    #else
        #undef KEGNINE_LINUX
        #define KEGNINE_LINUX 1
    #endif
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
    #include "kengine_linux.h"
#endif

#if KENGINE_IMPLEMENTATION
    #include "kengine/kengine_memory.c"
    #include "kengine/kengine_string.c"

    #if KENGINE_WIN32
        #include "kengine/kengine_win32.c"
    #elif KENGINE_LINUX
        #include "kengine_linux.c"
    #endif

#endif

#define KENGINE_LINUX_H
#endif // KENGINE_LINUX_H
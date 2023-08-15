#ifndef KENGINE_H

#if !defined(VERSION)
    #define VERSION 0
#endif

#ifdef KENGINE_IMPLEMENTATION
    #undef KENGINE_IMPLEMENTATION
    #define KENGINE_IMPLEMENTATION 1
#endif

#ifdef KENGINE_CONSOLE
    #undef KENGINE_CONSOLE
    #define KENGINE_CONSOLE 1
#endif

#ifdef KENGINE_TEST
    #undef KENGINE_TEST
    #define KENGINE_TEST 1
#endif


#ifdef KENGINE_PREPROCESSOR
    #undef KENGINE_PREPROCESSOR
    #define KENGINE_PREPROCESSOR 1
#endif

#ifdef KENGINE_OPENGL
    #undef KENGINE_OPENGL
    #define KENGINE_OPENGL 1
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

#if KENGINE_WIN32
    #include "kengine/win32_kengine.h"
#elif KENGINE_LINUX
    #include "kengine/linux_kengine.h"
#endif

#include "kengine/kengine_profiler.h"
#include "kengine/kengine_math.h"
#include "kengine/kengine_intrinsics.h"
#include "kengine/kengine_string.h"
#include "kengine/kengine_random.h"
#include "kengine/kengine_sha512.h"
#include "kengine/kengine_eddsa.h"
#include "kengine/kengine_tokenizer.h"
#include "kengine/kengine_xml_parser.h"
#include "kengine/kengine_c_parser.h"
#include "kengine/kengine_json_parser.h"
#include "kengine/kengine_image.h"


#if KENGINE_IMPLEMENTATION
    #include "kengine/kengine_memory.c"
    #include "kengine/kengine_string.c"
    #include "kengine/kengine_sha512.c"
    #include "kengine/kengine_eddsa.c"
    #include "kengine/kengine_tokenizer.c"
    #include "kengine/kengine_c_parser.c"

    #if KENGINE_WIN32
        #include "kengine/win32_kengine.c"
    #elif KENGINE_LINUX
        #include "kengine/linux_kengine.c"
    #endif


#endif

#if KENGINE_CONSOLE

#include "kengine/kengine_console.c"

#elif KENGINE_TEST

#include "kengine/kengine_test.c"

#elif KENGINE_PREPROCESSOR

#include "kengine/kengine_preprocessor.c"

#elif KENGINE_OPENGL

    #if KENGINE_WIN32

    #include <windows.h>
    #include "kengine/win32_kengine_opengl.c"

    #elif KENGINE_LINUX
    
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include "kengine/linux_kengine_opengl.c"
       
    #endif

#endif

#define KENGINE_H
#endif // KENGINE_H
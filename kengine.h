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
#include "kengine/kengine_random.h"
#include "kengine/kengine_sha512.h"
#include "kengine/kengine_eddsa.h"
#include "kengine/kengine_tokenizer.h"
#include "kengine/kengine_xml_parser.h"
#include "kengine/kengine_c_parser.h"

#if KENGINE_WIN32
    #include "kengine/kengine_win32.h"
#elif KENGINE_LINUX
    #include "kengine/kengine_linux.h"
#endif

#if KENGINE_IMPLEMENTATION
    #include "kengine/kengine_memory.c"
    #include "kengine/kengine_string.c"
    #include "kengine/kengine_sha512.c"
    #include "kengine/kengine_eddsa.c"
    #include "kengine/kengine_tokenizer.c"
    #include "kengine/kengine_c_parser.c"

    #if KENGINE_WIN32
        #include "kengine/kengine_win32.c"
    #elif KENGINE_LINUX
        #include "kengine/kengine_linux.c"
    #endif

#endif

#if KENGINE_CONSOLE

#include "kengine/kengine_console.c"

#elif KENGINE_TEST

#include "kengine/kengine_test.c"

#elif KENGINE_PREPROCESSOR

#include "kengine/kengine_preprocessor.c"

#endif

#define KENGINE_H
#endif // KENGINE_H
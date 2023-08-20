#ifndef KENGINE_TYPES_H

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SEE/NEON optimizations are not available for this compiler yet!!!!
#endif

#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <float.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;
typedef s32 b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t umm;
typedef intptr_t smm;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#define U8Max 255
#define U16Max 65535
#define S32Min ((s32)0x80000000)
#define S32Max ((s32)0x7fffffff)
#define U32Min 0
#define U32Max ((u32)-1)
#define U64Max ((u64)-1)
#define F32Max FLT_MAX
#define F32Min -FLT_MAX

#define internal static
#define local_persist static
#define global static

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#define MAX_URL 2048
#define MAX_PATH 260

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#define AlignPow2(Value, Alignment) (((Value) + ((Alignment) - 1)) & ~(((Value) - (Value)) + (Alignment) - 1))
#define IsPow2(Value) ((Value & ~(Value - 1)) == Value)

#if KENGINE_SLOW
    #if COMPILER_MSVC
        #define Assert(Expression) if(!(Expression)) {__debugbreak();}
    #elif COMPILER_LLVM
        #define Assert(Expression) if(!(Expression)) {__builtin_trap();}
    #endif
#else
    #define Assert(...)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define OffsetOf(type, Member) (umm)&(((type *)0)->Member)

#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
#elif COMPILER_LLVM
#define CompletePreviousReadsBeforeFutureReads asm volatile("" ::: "memory")
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")
#else
#error Other compilers/platforms?
#endif

#include "kengine_intrinsics.h"

typedef struct ticket_mutex
{
    u64 volatile Ticket;
    u64 volatile Serving;
} ticket_mutex;

internal void
BeginTicketMutex(ticket_mutex *Mutex)
{
    u64 Ticket = AtomicAddU64(&Mutex->Ticket, 1);
    while(Ticket != Mutex->Serving)
    {
        _mm_pause();
    }
}

inline void
EndTicketMutex(ticket_mutex *Mutex)
{
    AtomicAddU64(&Mutex->Serving, 1);
}

typedef enum sort_type
{
    Sort_Ascending,
    Sort_Descending
} sort_type;

typedef struct date_time
{
    u16 Year;
    u8 Month;
    u8 Day;
    
    u8 Hour;
    u8 Minute;
    u8 Second;
    u16 Milliseconds;
} date_time;

typedef struct string
{
    umm Size;
    u8 *Data;
} string;
#define NullString() String_(0, 0)
#define IsNullString(Text) ((Text.Data == 0) || (Text.Size == 0))

introspect(linked_list)
typedef struct string_list
{
    string Entry;
    
    struct string_list *Next;
} string_list;

introspect(ctor, set1, math)
typedef struct v2
{
    f32 X;
    f32 Y;
} v2;

introspect(ctor, set1, math)
typedef struct v2i
{
    s32 X;
    s32 Y;
} v2i;

introspect(ctor, set1, math)
typedef struct v3
{
    f32 X;
    f32 Y;
    f32 Z;
} v3;

introspect(ctor, set1, math)
typedef struct v4
{
    f32 R;
    f32 G;
    f32 B;
    f32 A;
} v4;

introspect(ctor)
typedef struct rectangle2
{
    v2 Min;
    v2 Max;
} rectangle2;

introspect(ctor)
typedef struct rectangle2i
{
    s32 MinX;
    s32 MaxX;
    s32 MinY;
    s32 MaxY;
} rectangle2i;


typedef struct m4x4
{
    f32 E[4][4];
} m4x4;

typedef struct platform_file
{
    void *Handle;
    b32 NoErrors;
    s64 Offset;
} platform_file;

typedef enum platform_file_access_flags
{
    FileAccess_Read = 0x1,
    FileAccess_Write = 0x2

} platform_file_access_flags;

#define PlatformNoErrors(Handle) ((Handle)->NoErrors)

#define KENGINE_TYPES_H
#endif //KENGINE_TYPES_H

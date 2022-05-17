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
#error Other Compiler not supported
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_MSVC
#include <intrin.h>
//#include <immintrin.h>
#elif COMPILER_LLVM
#include <x86intrin.h>
#else
#error SEE/NEON optimizations are not available for this compiler
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

typedef intptr_t smm;
typedef uintptr_t umm;

typedef size_t memory_index;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

#define U8Max 255
#define U16Max 65535
#define S32Min ((s32)0x80000000)
#define S32Max ((s32)0x7fffffff)
#define S64Max ((s64)0x7fffffffffffffff)
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

#if KENGINE_INTERNAL
#define Assert(Expression) if(!(Expression)) {__debugbreak();}
#else
#define Assert(Expression)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

typedef struct memory_arena
{
    memory_index Size;
    u8 *Base;
    memory_index Used;
    
    s32 TempCount;
} memory_arena;

typedef struct temporary_memory
{
    memory_arena *Arena;
    memory_index Used;
} temporary_memory;

typedef struct string
{
    umm Size;
    u8 *Data;
} string;

introspect(ctor, set1, math);
typedef struct v2
{
    f32 X;
    f32 Y;
} v2;

introspect(ctor, set1, math);
typedef struct v3
{
    f32 X;
    f32 Y;
    f32 Z;
} v3;

introspect(ctor, set1, math);
typedef struct v4
{
    f32 R;
    f32 G;
    f32 B;
    f32 A;
} v4;

introspect(ctor);
typedef struct rectangle2i
{
    s32 MinX;
    s32 MaxX;
    s32 MinY;
    s32 MaxY;
} rectangle2i;

introspect(ctor);
typedef struct rectangle2
{
    v2 Min;
    v2 Max;
} rectangle2;


typedef struct loaded_bitmap
{
    void *Memory;
    v2 AlignPercentage;
    s32 Width;
    s32 Height;
    f32 WidthOverHeight;
    
    s32 Pitch;
} loaded_bitmap;


#define KENGINE_TYPES_H
#endif //KENGINE_TYPES_H

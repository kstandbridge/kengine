#ifndef KENGINE_TYPES_H

typedef signed char s8;
typedef short int s16;
typedef int s32;
typedef long int s64;
typedef int b32;

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long int u64;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

typedef __int64 smm;
typedef unsigned __int64 umm;

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

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

typedef struct
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


#define KENGINE_TYPES_H
#endif //KENGINE_TYPES_H

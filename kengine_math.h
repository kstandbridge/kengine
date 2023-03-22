#ifndef KENGINE_MATH_H

introspect(ctor, set1, math)
typedef struct v2
{
    f32 X;
    f32 Y;
} v2;

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

typedef struct m4x4
{
    f32 E[4][4];
} m4x4;

typedef struct rectangle2
{
    v2 Min;
    v2 Max;
} rectangle2;

#define Rectangle2(Min, Max) (rectangle2){Min, Max}

inline rectangle2
Rectangle2Union(rectangle2 A, rectangle2 B)
{
    rectangle2 Result;
    
    Result.Min.X = (A.Min.X < B.Min.X) ? A.Min.X : B.Min.X;
    Result.Min.Y = (A.Min.Y < B.Min.Y) ? A.Min.Y : B.Min.Y;
    Result.Max.X = (A.Max.X > B.Max.X) ? A.Max.X : B.Max.X;
    Result.Max.Y = (A.Max.Y > B.Max.Y) ? A.Max.Y : B.Max.Y;
    
    return Result;
}

#define KENGINE_MATH_H
#endif //KENGINE_MATH_H

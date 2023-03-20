#ifndef KENGINE_MATH_H

typedef struct v2
{
    f32 X;
    f32 Y;
} v2;
#define V2(X, Y) (v2){(f32)(X), (f32)(Y)}

typedef struct
{
    f32 X;
    f32 Y;
    f32 Z;
} v3;

#define V3(X, Y, Z) (v3){X, Y, Z}

typedef struct v4
{
    f32 R;
    f32 G;
    f32 B;
    f32 A;
} v4;
#define V4(R, G, B, A) (v4){R, G, B, A}

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

inline v2
V2Add(v2 A, v2 B)
{
    v2 Result;
    
    Result.X = A.X + B.X;
    Result.Y = A.Y + B.Y;
    
    return Result;
}

inline v2
V2Subtract(v2 A, v2 B)
{
    v2 Result;
    
    Result.X = A.X - B.X;
    Result.Y = A.Y - B.Y;
    
    return Result;
}

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

#ifndef KENGINE_MATH_H

//
// NOTE(kstandbridge): Scalar operations
//

inline u8
CountSetBits(u32 Value)
{
    u8 Result = 0;
    while(Value)
    {
        Result += (Value & 1);
        Value >>= 1;
    }
    return Result;
}

inline u8
GetBits(u8 Input, u8 Position, u8 Count)
{
    // NOTE(kstandbridge): Position 7 is far left, 0 is far right
    u8 Result = (Input >> (Position + 1 - Count) & ~(~0 << Count));
    return Result;
}

inline u16
PackU16(u8 HighPart, u8 LowPart)
{
    u16 Result = ((HighPart & 0xFF) << 8) | (LowPart & 0xFF);
    return Result;
}

inline u8
UnpackU16High(u16 Value)
{
    u8 *LowByte = (u8 *)&Value;
    u8 *HighByte = LowByte + 1;
    
    u8 Result = *HighByte;
    return Result;
}

inline u8
UnpackU16Low(u16 Value)
{
    u8 *LowByte = (u8 *)&Value;
    //u8 *HighByte = LowByte + 1;
    
    u8 Result = *LowByte;
    return Result;
}

inline u32
F32ToRadixValue(f32 Value)
{
    u32 Result = *(u32 *)&Value;
    if(Result & 0x80000000)
    {
        Result = ~Result;
    }
    else
    {
        Result |= 0x80000000;
    }
    return(Result);
}

inline f32
SafeRatioN(f32 Numerator, f32 Divisor, f32 N)
{
    f32 Result = N;
    
    if(Divisor != 0.0f)
    {
        Result = Numerator / Divisor;
    }
    
    return Result;
}

inline f32
SafeRatio0(f32 Numerator, f32 Divisor)
{
    f32 Result = SafeRatioN(Numerator, Divisor, 0.0f);
    return Result;
}

inline f32
SafeRatio1(f32 Numerator, f32 Divisor)
{
    f32 Result = SafeRatioN(Numerator, Divisor, 1.0f);
    return Result;
}
inline u32
SafeTruncateU64ToU32(u64 Value)
{
    Assert(Value <= U32Max);
    u32 Result = (u32)Value;
    return(Result);
}

inline u16
SafeTruncateU32ToU16(u32 Value)
{
    Assert(Value <= U16Max);
    u16 Result = (u16)Value;
    return(Result);
}

inline f32
Square(f32 A)
{
    f32 Result = A*A;
    return Result;
}

//
// NOTE(kstandbridge): v2 operations
//

inline f32
Inner(v2 A, v2 B)
{
    f32 Result = A.X*B.X + A.Y*B.Y;
    return Result;
}

inline f32
LengthSq(v2 A)
{
    f32 Result = Inner(A, A);
    return Result;
}

inline f32
LengthV2(v2 A)
{
    f32 Result = SquareRoot(LengthSq(A));
    return Result;
}

inline v2
Hadamard(v2 A, v2 B)
{
    v2 Result = V2(A.X*B.X, A.Y*B.Y);
    return Result;
}

inline f32
V2DistanceBetween(v2 A, v2 B)
{
    f32 Result = SquareRoot((A.X - B.X)*(A.X - B.X) + (A.Y - B.Y)*(A.Y - B.Y));
    return Result;
}

//
// NOTE(kstandbridge): v2i operations
//

inline f32
V2iDistanceBetween(v2i A, v2i B)
{
    f32 Result = V2DistanceBetween(V2((f32)A.X, (f32)A.Y), V2((f32)B.X, (f32)B.Y));
    return Result;
}

//
// NOTE(kstandbridge): v4 operations
//

inline v4
RGBColor(f32 R, f32 G, f32 B, f32 A)
{
    v4 Result;
    
    Result.R = R/255.0f;
    Result.G = G/255.0f;
    Result.B = B/255.0f;
    Result.A = A/255.0f;
    
    return Result;
}

inline v4
SRGB255ToLinear1(v4 C)
{
    v4 Result;
    
    f32 Inv255 = 1.0f / 255.0f;
    
    Result.R = Square(Inv255*C.R);
    Result.G = Square(Inv255*C.G);
    Result.B = Square(Inv255*C.B);
    Result.A = Inv255*C.A;
    
    return Result;
}

inline v4
Linear1ToSRGB255(v4 C)
{
    v4 Result;
    
    f32 One255 = 255.0f;
    
    Result.R = One255*SquareRoot(C.R);
    Result.G = One255*SquareRoot(C.G);
    Result.B = One255*SquareRoot(C.B);
    Result.A = One255*C.A;
    
    return Result;
}

//
// NOTE(kstandbridge): rectangle2 operations
//

inline rectangle2
Rectangle2AddRadiusTo(rectangle2 Rectangle, f32 Radius)
{
    rectangle2 Result;
    
    Result.Min = V2Subtract(Rectangle.Min, V2Set1(Radius));
    Result.Max = V2Add(Rectangle.Max, V2Set1(Radius));
    
    return Result;
}

inline rectangle2
Rectangle2InvertedInfinity()
{
    rectangle2 Result;
    
    Result.Min.X = Result.Min.Y = F32Max;
    Result.Max.X = Result.Max.Y = -F32Max;
    
    return Result;
}

inline rectangle2
Rectangle2MinDim(v2 Min, v2 Dim)
{
    rectangle2 Result;
    
    Result.Min = Min;
    Result.Max = V2Add(Min, Dim);
    
    return(Result);
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

inline b32
Rectangle2IsIn(rectangle2 Rectangle, v2 Test)
{
    b32 Result = ((Test.X >= Rectangle.Min.X) &&
                  (Test.Y >= Rectangle.Min.Y) &&
                  (Test.X < Rectangle.Max.X) &&
                  (Test.Y < Rectangle.Max.Y));
    
    return Result;
}

inline v2
Rectangle2GetDim(rectangle2 Rectangle)
{
    v2 Result = V2Subtract(Rectangle.Max, Rectangle.Min);
    return Result;
}

//
// NOTE(kstandbridge): rectangle2i operations
//

inline rectangle2i
InvertedInfinityRectangle2i()
{
    rectangle2i Result;
    
    Result.MinX = Result.MinY = S32Max;
    Result.MaxX = Result.MaxY = S32Min;
    
    return Result;
}

inline rectangle2i
Intersect(rectangle2i A, rectangle2i B)
{
    rectangle2i Result;
    
    Result.MinX = (A.MinX < B.MinX) ? B.MinX : A.MinX;
    Result.MinY = (A.MinY < B.MinY) ? B.MinY : A.MinY;
    Result.MaxX = (A.MaxX > B.MaxX) ? B.MaxX : A.MaxX;
    Result.MaxY = (A.MaxY > B.MaxY) ? B.MaxY : A.MaxY;    
    
    return(Result);
}


inline b32
HasArea(rectangle2i A)
{
    b32 Result = ((A.MinX < A.MaxX) && (A.MinY < A.MaxY));
    
    return Result;
}

//
// NOTE(kstandbridge): m4x4 operations
//


inline m4x4
M4x4RotationZ(f32 Angle)
{
    f32 CosAngle = Cos(Angle);
    f32 SinAngle = Sin(Angle);
    
    m4x4 Result;
    
    Result.E[0][0] = CosAngle;
    Result.E[0][1] = SinAngle;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = -SinAngle;
    Result.E[1][1] = CosAngle;
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = 1.0f;
    Result.E[2][3] = 0.0f;
    
    Result.E[3][0] = 0.0f;
    Result.E[3][1] = 0.0f;
    Result.E[3][2] = 0.0f;
    Result.E[3][3] = 1.0f;
    
    return Result;
}

inline m4x4
M4x4RotationX(f32 Angle)
{
    f32 SinAngle = Sin(Angle);
    f32 CosAngle = Cos(Angle);
    
    m4x4 Result;
    
    Result.E[0][0] = 1.0f;
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = CosAngle;
    Result.E[1][2] = SinAngle;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = -SinAngle;
    Result.E[2][2] = CosAngle;
    Result.E[2][3] = 0.0f;
    
    Result.E[3][0] = 0.0f;
    Result.E[3][1] = 0.0f;
    Result.E[3][2] = 0.0f;
    Result.E[3][3] = 1.0f;
    
    return Result;
}

inline m4x4
M4x4Multiply(m4x4 M1, m4x4 M2)
{
#if 0
    m4x4 Result;
    ZeroStruct(Result);
    
    for(s32 Row = 0; Row <= 3; ++Row) // NOTE(kstandbridge): Rows of A
    {
        for(s32 Column = 0; Column <= 3; ++Column) // NOTE(kstandbridge): Column of B
        {
            for(s32 Index = 0; Index <= 3; ++Index) // NOTE(kstandbridge): Columns of A, Rows of B
            {
                Result.E[Column][Row] += A.E[Row][Index]*B.E[Index][Column];
            }
        }
    }
    
    return Result;
#else
    m4x4 Result;
    // Cache the invariants in registers
    f32 x = M1.E[0][0];
    f32 y = M1.E[0][1];
    f32 z = M1.E[0][2];
    f32 w = M1.E[0][3];
    // Perform the operation on the first row
    Result.E[0][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[0][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[0][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[0][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    // Repeat for all the other rows
    x = M1.E[1][0];
    y = M1.E[1][1];
    z = M1.E[1][2];
    w = M1.E[1][3];
    Result.E[1][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[1][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[1][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[1][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    x = M1.E[2][0];
    y = M1.E[2][1];
    z = M1.E[2][2];
    w = M1.E[2][3];
    Result.E[2][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[2][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[2][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[2][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    x = M1.E[3][0];
    y = M1.E[3][1];
    z = M1.E[3][2];
    w = M1.E[3][3];
    Result.E[3][0] = (M2.E[0][0] * x) + (M2.E[1][0] * y) + (M2.E[2][0] * z) + (M2.E[3][0] * w);
    Result.E[3][1] = (M2.E[0][1] * x) + (M2.E[1][1] * y) + (M2.E[2][1] * z) + (M2.E[3][1] * w);
    Result.E[3][2] = (M2.E[0][2] * x) + (M2.E[1][2] * y) + (M2.E[2][2] * z) + (M2.E[3][2] * w);
    Result.E[3][3] = (M2.E[0][3] * x) + (M2.E[1][3] * y) + (M2.E[2][3] * z) + (M2.E[3][3] * w);
    return Result;
#endif
    
}



inline m4x4
M4x4Scale(v3 A)
{
    m4x4 Result = 
    { 
        A.X,  0,   0,   0,
        0,  A.Y,   0,   0,
        0,    0, A.Z,   0,
        0,    0,   0,   1
    };
    
    return Result;
}

inline m4x4
M4x4Translation(v3 A)
{
#if 0
    m4x4 Result = 
    { 
        1, 0, 0, A.X,
        0, 1, 0, A.Y,
        0, 0, 1, A.Z,
        0, 0, 0,   1
    };
#else
    m4x4 Result ;
    
    Result.E[0][0] = 1.0f;
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = 1.0f;
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = 1.0f;
    Result.E[2][3] = 0.0f;
    
    Result.E[3][0] = A.X;
    Result.E[3][1] = A.Y;
    Result.E[3][2] = A.Z;
    Result.E[3][3] = 1.0f;
#endif
    return Result;
}

inline m4x4
M4x4Identity()
{
    m4x4 Result = 
    { 
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    };
    
    return Result;
}

inline m4x4
M4x4Transpose(m4x4 A)
{
#if 0
    m4x4 Result;
    
    for(s32 Row = 0; Row <= 3; ++Row)
    {
        for(s32 Column = 0; Column <= 3; ++Column)
        {
            Result.E[Row][Column] = A.E[Column][Row];
        }
    }
#else
    m4x4 Result = 
    {
        A.E[0][0], A.E[1][0], A.E[2][0], A.E[3][0],
        A.E[0][1], A.E[1][1], A.E[2][1], A.E[3][1],
        A.E[0][2], A.E[1][2], A.E[2][2], A.E[3][2],
        A.E[0][3], A.E[1][3], A.E[2][3], A.E[3][3],
    };
#endif
    
    return Result;
}

inline m4x4
M4x4PerspectiveLH(f32 ViewWidth, f32 ViewHeight, f32 NearZ, f32 FarZ)
{
    f32 TwoNearZ = NearZ + NearZ;
    f32 Range = FarZ / (FarZ - NearZ);
    
    m4x4 Result;
    
    Result.E[0][0] = TwoNearZ / ViewWidth;
    Result.E[0][1] = 0.0f;
    Result.E[0][2] = 0.0f;
    Result.E[0][3] = 0.0f;
    
    Result.E[1][0] = 0.0f;
    Result.E[1][1] = TwoNearZ / ViewHeight;
    Result.E[1][2] = 0.0f;
    Result.E[1][3] = 0.0f;
    
    Result.E[2][0] = 0.0f;
    Result.E[2][1] = 0.0f;
    Result.E[2][2] = Range;
    Result.E[2][3] = 1.0f;
    
    Result.E[3][0] = 0.0f;
    Result.E[3][1] = 0.0f;
    Result.E[3][2] = -Range * NearZ;
    Result.E[3][3] = 0.0f;
    
    return Result;
}

#define KENGINE_MATH_H
#endif //KENGINE_MATH_H

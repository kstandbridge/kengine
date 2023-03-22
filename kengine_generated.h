
// kengine_math.h

#define V2(X, Y) (v2){(f32)(X), (f32)(Y)}
#define V2Set1(Value) (v2){(f32)(Value), (f32)(Value)}
#define V2Add(A, B) (v2){(A.X + B.X), (A.Y + B.Y)}
#define V2Subtract(A, B) (v2){(A.X - B.X), (A.Y - B.Y)}
#define V2Multiply(A, B) (v2){(A.X * B.X), (A.Y * B.Y)}

#define V3(X, Y, Z) (v3){(f32)(X), (f32)(Y), (f32)(Z)}
#define V3Set1(Value) (v3){(f32)(Value), (f32)(Value), (f32)(Value)}
#define V3Add(A, B) (v3){(A.X + B.X), (A.Y + B.Y), (A.Z + B.Z)}
#define V3Subtract(A, B) (v3){(A.X - B.X), (A.Y - B.Y), (A.Z - B.Z)}
#define V3Multiply(A, B) (v3){(A.X * B.X), (A.Y * B.Y), (A.Z * B.Z)}

#define V4(R, G, B, A) (v4){(f32)(R), (f32)(G), (f32)(B), (f32)(A)}
#define V4Set1(Value) (v4){(f32)(Value), (f32)(Value), (f32)(Value), (f32)(Value)}
#define V4Add(A, B) (v4){(A.R + B.R), (A.G + B.G), (A.B + B.B), (A.A + B.A)}
#define V4Subtract(A, B) (v4){(A.R - B.R), (A.G - B.G), (A.B - B.B), (A.A - B.A)}
#define V4Multiply(A, B) (v4){(A.R * B.R), (A.G * B.G), (A.B * B.B), (A.A * B.A)}

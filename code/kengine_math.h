#ifndef KENGINE_MATH_H

//
// NOTE(kstandbridge): Scalar operations
//
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


//
// NOTE(kstandbridge): rectangle2i operations
//

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


#define KENGINE_MATH_H
#endif //KENGINE_MATH_H

#ifndef KENGINE_INTRINSICS_H

inline f32
SquareRoot(f32 Real32)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Real32)));
    return Result;
}


inline s32
RoundF32ToS32(f32 Value)
{
    s32 Result = _mm_cvtss_si32(_mm_set_ss(Value));
    return Result;
}


inline u32
RoundF32ToU32(f32 Value)
{
    u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Value));
    return Result;
}

inline s32
FloorF32ToS32(f32 Real32)
{
    s32 Result = _mm_cvtss_si32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(Real32)));
    return Result;
}

inline s32
CeilF32ToS32(f32 Real32)
{
    s32 Result = _mm_cvtss_si32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(Real32)));
    return Result;
}


#define KENGINE_INTRINSICS_H
#endif //KENGINE_INTRINSICS_H

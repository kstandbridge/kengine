#ifndef KENGINE_INTRINSICS_H


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


#define KENGINE_INTRINSICS_H
#endif //KENGINE_INTRINSICS_H

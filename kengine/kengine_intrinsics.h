#ifndef KENGINE_INTRINSICS_H

inline u32 
AtomicCompareExchangeU32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);
    
    return Result;
}

inline u32
AtomicIncrementU32(u32 volatile *Value)
{
    u32 Result = _InterlockedIncrement((long volatile *)Value);
    
    return Result;
}

inline u32 
AtomicExchange(u32 volatile *Value, u32 New)
{
    u32 Result = (u32)_InterlockedExchange((long volatile *)Value, New);
    
    return Result;
}

inline u64 
AtomicExchangeU64(u64 volatile *Value, u64 New)
{
    u64 Result = (u64)_InterlockedExchange64((__int64 volatile *)Value, New);
    
    return Result;
}

inline u32 
AtomicAddU32(u32 volatile *Value, u32 Addend)
{
    u32 Result = (u32)_InterlockedExchangeAdd((long volatile *)Value, Addend);
    
    return Result;
}    

inline u64 
AtomicAddU64(u64 volatile *Value, u64 Addend)
{
    u64 Result = _InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);
    
    return Result;
}    

inline u32 
GetThreadID()
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);
    
    return ThreadID;
}

inline f32
SquareRoot(f32 Value)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Value)));
    return Result;
}

inline f32
Cos(f32 Value)
{
    f32 Result = _mm_cvtss_f32(_mm_cos_ps(_mm_set_ss(Value)));
    return Result;
}

inline f32
Sin(f32 Value)
{
    f32 Result = _mm_cvtss_f32(_mm_sin_ps(_mm_set_ss(Value)));
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
FloorF32ToS32(f32 Value)
{
    s32 Result = _mm_cvtss_si32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(Value)));
    return Result;
}

inline s32
CeilF32ToS32(f32 Value)
{
    s32 Result = _mm_cvtss_si32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(Value)));
    return Result;
}


#define KENGINE_INTRINSICS_H
#endif //KENGINE_INTRINSICS_H

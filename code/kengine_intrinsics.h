#ifndef KENGINE_INTRINSICS_H

#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline u32 AtomicCompareExchangeUInt32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = _InterlockedCompareExchange((long *)Value, New, Expected);
    
    return(Result);
}
#elif COMPILER_LLVM
// TODO(kstandbridge): Does LLVM have real read-specific barriers yet?
#define CompletePreviousReadsBeforeFutureReads asm volatile("" ::: "memory")
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")
inline u32 AtomicCompareExchangeUInt32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = __sync_val_compare_and_swap(Value, Expected, New);
    
    return(Result);
}
#else
// TODO(kstandbridge): Other compilers/platforms??
#endif

inline f32
SquareRoot(f32 Real32)
{
    f32 Result = _mm_cvtss_f32(_mm_sqrt_ss(_mm_set_ss(Real32)));
    return Result;
}

inline s32
RoundReal32ToInt32(f32 Real32)
{
    s32 Result = _mm_cvtss_si32(_mm_set_ss(Real32));
    return Result;
}

inline u32
RoundReal32ToUInt32(f32 Real32)
{
    u32 Result = (u32)_mm_cvtss_si32(_mm_set_ss(Real32));
    return Result;
}

inline s32
FloorReal32ToInt32(f32 Real32)
{
    s32 Result = _mm_cvtss_si32(_mm_floor_ss(_mm_setzero_ps(), _mm_set_ss(Real32)));
    return Result;
}

inline s32
CeilReal32ToInt32(f32 Real32)
{
    s32 Result = _mm_cvtss_si32(_mm_ceil_ss(_mm_setzero_ps(), _mm_set_ss(Real32)));
    return Result;
}

inline f32
Sin(f32 Angle)
{
    f32 Result = sinf(Angle);
    return Result;
}

inline f32
Cos(f32 Angle)
{
    f32 Result = cosf(Angle);
    return Result;
}

typedef struct bit_scan_result
{
    b32 Found;
    u32 Index;
} bit_scan_result;
inline bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result;
    
#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(u32 Test = 0;
        Test < 32;
        ++Test)
    {
        if(Value & (1 << Test))
        {
            Result.Index = Test;
            Result.Found = true;
            break;
        }
    }
#endif
    
    return(Result);
}


#define KENGINE_INTRINSICS_H
#endif //KENGINE_INTRINSICS_H

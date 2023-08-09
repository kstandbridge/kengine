#ifndef KENGINE_INTRINSICS_H

inline u32
CountSetBits(u32 Value)
{
#if COMPILER_MSVC
    u32 Result = __popcnt(Value);
#elif COMPILER_LLVM
    // TODO(kstandbridge): Linux support
    u32 Result = __builtin_popcount(Value);
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    return Result;

}

inline u32 
AtomicCompareExchangeU32(u32 volatile *Value, u32 New, u32 Expected)
{
#if COMPILER_MSVC
    u32 Result = _InterlockedCompareExchange((long volatile *)Value, New, Expected);
#elif COMPILER_LLVM
    u32 Result = __sync_val_compare_and_swap(Value, Expected, New);
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    
    return Result;
}

inline u32 
AtomicExchange(u32 volatile *Value, u32 New)
{
#if COMPILER_MSVC
    u32 Result = (u32)_InterlockedExchange((long volatile *)Value, New);
#elif COMPILER_LLVM
    u32 Result = __sync_lock_test_and_set(Value, New);
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    
    return Result;
}

inline u64 
AtomicExchangeU64(u64 volatile *Value, u64 New)
{
#if COMPILER_MSVC
    u64 Result = (u64)_InterlockedExchange64((__int64 volatile *)Value, New);
#elif COMPILER_LLVM
    u64 Result = __sync_lock_test_and_set(Value, New);
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    
    return Result;
}

inline u32 
AtomicAddU32(u32 volatile *Value, u32 Addend)
{
#if COMPILER_MSVC
    u32 Result = (u32)_InterlockedExchangeAdd((long volatile *)Value, Addend);
#elif COMPILER_LLVM
    u32 Result = __sync_fetch_and_add(Value, Addend);
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    
    return Result;
}    

inline u64 
AtomicAddU64(u64 volatile *Value, u64 Addend)
{
#if COMPILER_MSVC
    u64 Result = _InterlockedExchangeAdd64((__int64 volatile *)Value, Addend);
#elif COMPILER_LLVM
    u64 Result = __sync_fetch_and_add(Value, Addend);
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    
    return Result;
}    

inline u32
AtomicIncrementU32(u32 volatile *Value)
{
#if COMPILER_MSVC
    u32 Result = _InterlockedIncrement((long volatile *)Value);
#elif COMPILER_LLVM
    u32 Result = AtomicAddU32(Value, 1);
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    
    return Result;
}

inline u32 
GetThreadID()
{
    u32 ThreadID;
#if COMPILER_MSVC
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);
#elif defined(__i386__)
    asm("mov %%gs:0x08,%0" : "=r"(ThreadID));
#elif defined(__x86_64__)
    asm("mov %%fs:0x10,%0" : "=r"(ThreadID));
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif
    
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
#if COMPILER_MSVC
    f32 Result = _mm_cvtss_f32(_mm_cos_ps(_mm_set_ss(Value)));
#elif COMPILER_LLVM
    // TODO(kstandbridge): Linux support
    f32 Result = 0.0f;
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif

    return Result;
}

inline f32
Sin(f32 Value)
{
#if COMPILER_MSVC
    f32 Result = _mm_cvtss_f32(_mm_sin_ps(_mm_set_ss(Value)));
#elif COMPILER_LLVM
    // TODO(kstandbridge): Linux support
    f32 Result = 0.0f;
#else
    #error Platform not supported
    // TODO(kstandbridge): Other platforms
#endif

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

typedef struct bit_scan_result
{
    b32 Found;
    u32 Index;
} bit_scan_result;

internal bit_scan_result
FindLeastSignificantSetBit(u32 Value)
{
    bit_scan_result Result;

#if COMPILER_MSVC
    Result.Found = _BitScanForward((unsigned long *)&Result.Index, Value);
#else
    for(s32 Test = 0;
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

internal bit_scan_result
FindMostSignificantSetBit(u32 Value)
{
    bit_scan_result Result;

#if COMPILER_MSVC
    Result.Found = _BitScanReverse((unsigned long *)&Result.Index, Value);
#else
    for(s32 Test = 32;
        Test > 0;
        --Test)
    {
        if(Value & (1 << (Test - 1)))
        {
            Result.Index = Test - 1;
            Result.Found = true;
            break;
        }
    }
#endif

    return(Result);
}


#define KENGINE_INTRINSICS_H
#endif //KENGINE_INTRINSICS_H

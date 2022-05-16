#ifndef KENGINE_PLATFORM_H

int _fltused = 0x9875;

#pragma function(memset)
void *memset(void *_Dst, int _Val, size_t _Size)
{
    unsigned char Val = *(unsigned char *)&_Val;
    unsigned char *At = (unsigned char *)_Dst;
    while(_Size--)
    {
        *At++ = Val;
    }
    return(_Dst);
}

#define introspect(...)
#include "kengine_types.h"

#if COMPILER_MSVC
#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline u32 AtomicCompareExchangeUInt32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = _InterlockedCompareExchange((long *)Value, New, Expected);
    
    return Result;
}
inline u32 GetThreadID()
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 ThreadID = *(u32 *)(ThreadLocalStorage + 0x48);
    
    return(ThreadID);
}
#elif COMPILER_LLVM
// TODO(kstandbridge): Does LLVM have real read-specific barriers yet?
#define CompletePreviousReadsBeforeFutureReads asm volatile("" ::: "memory")
#define CompletePreviousWritesBeforeFutureWrites asm volatile("" ::: "memory")
inline u32 AtomicCompareExchangeUInt32(u32 volatile *Value, u32 New, u32 Expected)
{
    u32 Result = __sync_val_compare_and_swap(Value, Expected, New);
    
    return Result;
}
inline u32 GetThreadID(void)
{
    u32 ThreadID;
#if defined(__APPLE__) && defined(__x86_64__)
    asm("mov %%gs:0x00,%0" : "=r"(ThreadID));
#elif defined(__i386__)
    asm("mov %%gs:0x08,%0" : "=r"(ThreadID));
#elif defined(__x86_64__)
    asm("mov %%fs:0x10,%0" : "=r"(ThreadID));
#else
#error Unsupported architecture
#endif
    
    return ThreadID;
}
#else
// TODO(kstandbridge): Other compilers/platforms??
#endif

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}


#define BITMAP_BYTES_PER_PIXEL 4
typedef struct app_offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    void *Memory;
    s32 Width;
    s32 Height;
    s32 Pitch;
} app_offscreen_buffer;

//
// NOTE(kstandbridge): Platform api
//
typedef string debug_read_entire_file(memory_arena *Arena, char *FilePath);
typedef loaded_bitmap debug_get_glyph_for_codepoint(memory_arena *Arena, u32 Codepoint);
typedef f32 debug_get_horiziontal_advance_for_pair(u32 PrevCodePoint, u32 CodePoint);

typedef struct platform_work_queue platform_work_queue;
typedef void platform_work_queue_callback(void *Data);

typedef void platform_add_work_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

typedef struct platform_api
{
    platform_work_queue *PerFrameWorkQueue;
    platform_work_queue *BackgroundWorkQueue;
    
    platform_add_work_entry *AddWorkEntry;
    platform_complete_all_work *CompleteAllWork;
    
    debug_read_entire_file *DEBUGReadEntireFile;
    debug_get_glyph_for_codepoint *DEBUGGetGlyphForCodePoint;
    debug_get_horiziontal_advance_for_pair *DEBUGGetHorizontalAdvanceForPair;
} platform_api;

//
// NOTE(kstandbridge): App api
//

typedef struct app_memory
{
    u64 StorageSize;
    void *Storage;
    
    platform_api PlatformAPI;
} app_memory;

typedef void app_update_and_render(app_memory *Memory, app_offscreen_buffer *Buffer, f32 DeltaTime);


#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

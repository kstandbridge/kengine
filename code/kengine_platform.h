#ifndef KENGINE_PLATFORM_H

#include <stdarg.h>
#include <immintrin.h>

#define introspect(...)
#include "kengine_types.h"

#if KENGINE_SLOW
#define Assert(Expression) if(!(Expression)) {__debugbreak();}
#else
#define Assert(...)
#endif

#include "kengine_memory.h"

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
inline u32
GetThreadId()
{
    u8 *ThreadLocalStorage = (u8 *)__readgsqword(0x30);
    u32 Result = *(u32 *)(ThreadLocalStorage + 0x48);
    
    return Result;
}
typedef void platform_work_queue_callback(void *Data);

typedef struct platform_work_queue platform_work_queue;
typedef void platform_add_work_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

typedef struct
{
    struct platform_work_queue *PerFrameWorkQueue;
    struct platform_work_queue *BackgroundWorkQueue;
    platform_add_work_entry *AddWorkEntry;
    platform_complete_all_work *CompleteAllWork;
    
} platform_api;

typedef struct
{
    u64 StorageSize;
    void *Storage;
    
    platform_api PlatformAPI;
} app_memory;

typedef void app_update_frame(platform_api PlatformAPI, render_commands *Commands);

#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

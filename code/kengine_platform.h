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

// TODO(kstandbridge): Double-linked list meta programming
#define DLIST_INSERT(Sentinel, Element)         \
(Element)->Next = (Sentinel)->Next;         \
(Element)->Prev = (Sentinel);               \
(Element)->Next->Prev = (Element);          \
(Element)->Prev->Next = (Element);
#define DLIST_REMOVE(Element)         \
if((Element)->Next) \
{ \
(Element)->Next->Prev = (Element)->Prev;         \
(Element)->Prev->Next = (Element)->Next;         \
(Element)->Next = (Element)->Prev = 0; \
}
#define DLIST_INSERT_AS_LAST(Sentinel, Element)         \
(Element)->Next = (Sentinel);               \
(Element)->Prev = (Sentinel)->Prev;         \
(Element)->Next->Prev = (Element);          \
(Element)->Prev->Next = (Element);

#define DLIST_INIT(Sentinel) \
(Sentinel)->Next = (Sentinel); \
(Sentinel)->Prev = (Sentinel)

#define DLIST_IS_EMPTY(Sentinel) \
((Sentinel)->Next == (Sentinel))

// TODO(kstandbridge): Free list meta programming
#define FREELIST_ALLOCATE(Result, FreeListPointer, AllocationCode)             \
(Result) = (FreeListPointer); \
if(Result) {FreeListPointer = (Result)->NextFree;} else {Result = AllocationCode;}
#define FREELIST_DEALLOCATE(Pointer, FreeListPointer) \
if(Pointer) {(Pointer)->NextFree = (FreeListPointer); (FreeListPointer) = (Pointer);}


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

typedef struct app_button_state
{
    s32 HalfTransitionCount;
    b32 EndedDown;
} app_button_state;

typedef enum mouse_button_type
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Extended0,
    MouseButton_Extended1,
    
    MouseButton_Count,
} mouse_button_type;

typedef enum keyboard_button_type
{
    KeyboardButton_Delete,
    KeyboardButton_Backspace,
    KeyboardButton_Return,
    KeyboardButton_Up,
    KeyboardButton_Left,
    KeyboardButton_Down,
    KeyboardButton_Right,
    KeyboardButton_Escape,
    
    KeyboardButton_Count,
} keyboard_button_type;

typedef struct app_input
{
    app_button_state MouseButtons[MouseButton_Count];
    f32 MouseX;
    f32 MouseY;
    f32 MouseZ;
    
    app_button_state KeyboardButtons[KeyboardButton_Count];
    b32 ShiftDown;
    b32 AltDown;
    b32 ControlDown;
    
    char Text[5];
    
    f32 dtForFrame;
} app_input;

inline b32
WasPressed(app_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
    return Result;
}

//
// NOTE(kstandbridge): Platform api
//
typedef struct platform_work_queue platform_work_queue;
typedef void platform_work_queue_callback(void *Data);

typedef void platform_add_work_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

typedef string get_command_line_args(memory_arena *Arena);

typedef string debug_read_entire_file(memory_arena *Arena, char *FilePath);
typedef loaded_bitmap debug_get_glyph_for_codepoint(memory_arena *Arena, u32 Codepoint);
typedef f32 debug_get_horiziontal_advance_for_pair(u32 PrevCodePoint, u32 CodePoint);
typedef f32 debug_get_line_advance();

typedef struct platform_api
{
    platform_work_queue *PerFrameWorkQueue;
    platform_work_queue *BackgroundWorkQueue;
    
    platform_add_work_entry *AddWorkEntry;
    platform_complete_all_work *CompleteAllWork;
    
    get_command_line_args *GetCommandLineArgs;
    
    debug_read_entire_file *DEBUGReadEntireFile;
    debug_get_glyph_for_codepoint *DEBUGGetGlyphForCodePoint;
    debug_get_horiziontal_advance_for_pair *DEBUGGetHorizontalAdvanceForPair;
    debug_get_line_advance *DEBUGGetLineAdvance;
} platform_api;

//
// NOTE(kstandbridge): App api
//

typedef struct app_memory
{
    u64 StorageSize;
    void *Storage;
    
    b32 ExecutableReloaded;
    
    platform_api PlatformAPI;
} app_memory;

typedef void app_update_and_render(app_memory *Memory, app_input *Input, app_offscreen_buffer *Buffer);


#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

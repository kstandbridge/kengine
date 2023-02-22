#ifndef KENGINE_TYPES_H

typedef signed char s8;
typedef short int s16;
typedef int s32;
typedef long long s64;
typedef int b32;

typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef float f32;
typedef double f64;

#define true 1
#define false 0

typedef __int64 smm;
typedef unsigned __int64 umm;

#define U8Max 255
#define U16Max 65535
#define S32Min (-2147483647i32 - 1)
#define S32Max (2147483647i32)
#define U32Min 0
#define U32Max ((u32)-1)
#define U64Max ((u64)-1)
#define F32Max 3.402823466e+38F
#define F32Min -1.175494351e-38F

#define internal static
#define local_persist static
#define global static

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#define MAX_URL 2048
#define MAX_PATH 260

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

#define AlignPow2(Value, Alignment) (((Value) + ((Alignment) - 1)) & ~(((Value) - (Value)) + (Alignment) - 1))
#define IsPow2(Value) ((Value & ~(Value - 1)) == Value)

#if KENGINE_SLOW
#define Assert(Expression) if(!(Expression)) {__debugbreak();}
#else
#define Assert(...)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

#define OffsetOf(type, Member) (umm)&(((type *)0)->Member)

#define CompletePreviousReadsBeforeFutureReads _ReadBarrier()
#define CompletePreviousWritesBeforeFutureWrites _WriteBarrier()
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

typedef struct ticket_mutex
{
    u64 volatile Ticket;
    u64 volatile Serving;
} ticket_mutex;

inline void
BeginTicketMutex(ticket_mutex *Mutex)
{
    u64 Ticket = AtomicAddU64(&Mutex->Ticket, 1);
    while(Ticket != Mutex->Serving)
    {
        _mm_pause();
    }
}

inline void
EndTicketMutex(ticket_mutex *Mutex)
{
    AtomicAddU64(&Mutex->Serving, 1);
}

typedef struct string
{
    umm Size;
    u8 *Data;
} string;

introspect(linked_list) typedef struct string_list
{
    string Entry;
    
    struct string_list *Next;
} string_list;

#define NullString() String_(0, 0)
#define IsNullString(Text) ((Text.Data == 0) || (Text.Size == 0))

typedef struct editable_string
{
    umm Size;
    umm Length;
    u8 *Data;
    
    u32 SelectionStart;
    u32 SelectionEnd;
    
} editable_string;

introspect(ctor, set1, math) typedef struct
{
    f32 X;
    f32 Y;
} v2;

introspect(ctor, set1, math) typedef struct
{
    s32 X;
    s32 Y;
} v2i;

introspect(ctor, set1, math) typedef struct
{
    f32 X;
    f32 Y;
    f32 Z;
} v3;

introspect(ctor, set1, math) typedef struct
{
    f32 R;
    f32 G;
    f32 B;
    f32 A;
} v4;

introspect(ctor) typedef struct
{
    v2 Min;
    v2 Max;
} rectangle2;

introspect(ctor) typedef struct
{
    s32 MinX;
    s32 MaxX;
    s32 MinY;
    s32 MaxY;
} rectangle2i;

typedef struct m4x4
{
    f32 E[4][4];
} m4x4;

introspect(ctor) typedef struct
{
    v4 Text;
    v4 HotText;
    v4 ClickedText;
    v4 SelectedText;
    
    v4 Background;
    v4 HotBackground;
    v4 ClickedBackground;
    v4 SelectedBackground;
    
    v4 Border;
    v4 HotBorder;
    v4 ClickedBorder;
    v4 SelectedBorder;
    
} element_colors;

typedef struct date_time
{
    u16 Year;
    u8 Month;
    u8 Day;
    
    u8 Hour;
    u8 Minute;
    u8 Second;
    u16 Milliseconds;
} date_time;

// TODO(kstandbridge): Where should loaded_bitmap live?
#define BITMAP_BYTES_PER_PIXEL 4
typedef struct
{
    void *Memory;
    v2 AlignPercentage;
    s32 Width;
    s32 Height;
    f32 WidthOverHeight;
    
    s32 Pitch;
    void *TextureHandle;
} loaded_bitmap;

typedef struct
{
    loaded_bitmap Bitmap;
    f32 KerningChange;
} loaded_glyph;

typedef enum sort_type
{
    Sort_Ascending,
    Sort_Descending
} sort_type;


// TODO(kstandbridge): Replace this with something we actually use
introspect(linked_list) typedef struct node
{
    u32 Value;
    
    struct node *Next;
} node;


// TODO(kstandbridge): move this to types.h
typedef struct path_node_link
{
    struct path_node *Node;
    
    struct path_node_link *Prev;
    struct path_node_link *Next;
} path_node_link;

#define KENGINE_TYPES_H
#endif //KENGINE_TYPES_H



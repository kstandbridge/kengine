#ifndef KENGINE_TYPES_H

typedef signed char s8;
typedef short int s16;
typedef int s32;
typedef long int s64;
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
#define F32Max 3.402823466e+38F
#define F32Min -1.175494351e-38F

#define internal static
#define local_persist static
#define global static

#define Pi32 3.14159265359f
#define Tau32 6.28318530717958647692f

#define Kilobytes(Value) ((Value)*1024LL)
#define Megabytes(Value) (Kilobytes(Value)*1024LL)
#define Gigabytes(Value) (Megabytes(Value)*1024LL)
#define Terabytes(Value) (Gigabytes(Value)*1024LL)

#define Minimum(A, B) ((A < B) ? (A) : (B))
#define Maximum(A, B) ((A > B) ? (A) : (B))

typedef struct
{
    umm Size;
    u8 *Data;
} string;

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


// TODO(kstandbridge): Replace this with a double linked list we use in production
introspect(dlist) typedef struct double_linked_list
{
    u32 SortKey;
    
    struct double_linked_list *Prev;
    struct double_linked_list *Next;
    
} double_linked_list;

// TODO(kstandbridge): move this to types.h
introspect(dlist) typedef struct node_link
{
    struct node *Node;
    
    struct node_link *Prev;
    struct node_link *Next;
} node_link;

#define KENGINE_TYPES_H
#endif //KENGINE_TYPES_H



#ifndef KENGINE_RENDERER_SHARED_H

typedef struct
{
    u16 Type;
    u16 ClipRectIndex;
} render_group_command_header;

typedef enum
{
    RenderGroupCommandType_Clear,
    RenderGroupCommandType_Rectangle,
} render_group_command_type;

typedef struct render_command_cliprect
{
    struct render_command_cliprect *Next;
    rectangle2i Bounds;
} render_command_cliprect;

typedef struct
{
    v4 Color;
} render_group_command_clear;

typedef struct
{
    v4 Color;
    rectangle2 Bounds;
} render_group_command_rectangle;

typedef struct render_commands
{
    s32 Width;
    s32 Height;
    
    u32 MaxPushBufferSize;
    u32 PushBufferSize;
    u8 *PushBufferBase;
    
    u32 PushBufferElementCount;
    u32 SortEntryAt;
    
    u16 ClipRectCount;
    render_command_cliprect *ClipRects;
    render_command_cliprect *FirstRect;
    render_command_cliprect *LastRect;
    
} render_commands;


#define KENGINE_RENDERER_SHARED_H
#endif //KENGINE_RENDERER_SHARED_H

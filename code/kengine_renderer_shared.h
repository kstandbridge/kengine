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

typedef struct
{
    v4 Color;
} render_group_command_clear;

typedef struct
{
    v4 Color;
    rectangle2 Bounds;
} render_group_command_rectangle;

#define KENGINE_RENDERER_SHARED_H
#endif //KENGINE_RENDERER_SHARED_H

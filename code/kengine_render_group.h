#ifndef KENGINE_RENDER_GROUP_H

typedef enum render_group_entry_type
{
    RenderGroupEntry_render_entry_clear,
    RenderGroupEntry_render_entry_bitmap,
    RenderGroupEntry_render_entry_rectangle,
} render_group_entry_type;

typedef struct render_group_entry_header
{
    render_group_entry_type Type;
} render_group_entry_header;

typedef struct render_entry_clear
{
    v4 Color;
} render_entry_clear;

typedef struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;
    
    v4 Color;
    v2 P;
    v2 Dim;
    f32 Angle;
} render_entry_bitmap;

typedef struct render_entry_rectangle
{
    v4 Color;
    v2 P;
    v2 Dim;
} render_entry_rectangle;

typedef struct render_group
{
    loaded_bitmap *OutputTarget;
    
    memory_index MaxPushBufferSize;
    memory_index PushBufferSize;
    u8 *PushBufferBase;
    
    v2 ScreenCenter;
    
} render_group;

#define KENGINE_RENDER_GROUP_H
#endif //KENGINE_RENDER_GROUP_H

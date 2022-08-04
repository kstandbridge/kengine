#ifndef KENGINE_RENDERER_H

typedef struct render_group
{
    render_commands *Commands;
    u16 CurrentClipRectIndex;
} render_group;

typedef enum render_group_command_type
{
    RenderGroupCommandType_Clear,
} render_group_command_type;

typedef struct render_group_command_header
{
    u16 Type;
    u16 ClipRectIndex;
} render_group_command_header;

typedef struct render_group_command_clear
{
    v4 Color;
} render_group_command_clear;

typedef struct object_transform
{
    v3 OffsetP;
    f32 Scale;
    f32 SortBias;
} object_transform;

typedef struct entity_basis_p_result
{
    v2 P;
    f32 Scale;
    b32 Valid;
    f32 SortKey;
} entity_basis_p_result;

#define KENGINE_RENDERER_H
#endif //KENGINE_RENDERER_H

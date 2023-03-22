#ifndef KENGINE_RENDERER_H

typedef enum render_command_type
{
    RenderCommand_Rect,
    RenderCommand_Glyph,
    RenderCommand_Sprite,
} render_command_type;

typedef struct render_command
{
    render_command_type Type;
    v2 Offset;
    f32 Depth;
    v2 Size;
    v4 Color;
    v4 UV;
    
} render_command;

typedef struct render_group
{
    render_command *Commands;
    u32 CurrentCommand;
    u32 MaxCommands;
    u32 Width;
    u32 Height;
    v4 ClearColor;
} render_group;


#define KENGINE_RENDERER_H
#endif //KENGINE_RENDERER_H

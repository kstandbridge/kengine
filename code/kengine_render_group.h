#ifndef KENGINE_RENDER_GROUP_H

typedef enum render_command_type
{
    RenderCommand_Text,
    RenderCommand_Rect
} render_command_type;

typedef struct render_command_text
{
    v4 Color;
    
    string Text;
} render_command_text;

typedef struct render_command_rect
{
    v4 Color;
    
} render_command_rect;

typedef struct render_command
{
    render_command_type Type;
    v2 Offset;
    f32 Depth;
    v2 Size;
    union
    {
        render_command_text Text;
        render_command_rect Rect;
    };
} render_command;

typedef struct render_group
{
    render_command *Commands;
    u32 CurrentCommand;
    u32 MaxCommands;
    u32 Width;
    u32 Height;
} render_group;

#define KENGINE_RENDER_GROUP_H
#endif //KENGINE_RENDER_GROUP_H

#ifndef KENGINE_RENDER_GROUP_H

typedef enum render_command_type
{
    RenderCommand_Text,
    RenderCommand_Rect
} render_command_type;

typedef struct render_command_text
{
    v3 Offset;
    v2 Size;
    v4 Color;
    
    string Text;
} render_command_text;

typedef struct render_command_rect
{
    v3 Offset;
    v2 Size;
    v4 Color;
} render_command_rect;

typedef struct render_command
{
    render_command_type Type;
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
} render_group;

#define KENGINE_RENDER_GROUP_H
#endif //KENGINE_RENDER_GROUP_H

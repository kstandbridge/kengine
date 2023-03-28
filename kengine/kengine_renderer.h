#ifndef KENGINE_RENDERER_H


typedef struct vertex_instance
{
    v3 Offset;
    v2 Size;
    v4 Color;
    v4 UV;
} vertex_instance;

#define MaxVertexCount 32768
#define MaxRenderCommands 512

#define VertexInstanceSize sizeof(vertex_instance)
#define VertexBufferSize MaxVertexCount*VertexInstanceSize

typedef enum render_command_type
{
    RenderCommand_Rect,
    
    RenderCommand_Sprite,
    RenderCommand_Glyph,
} render_command_type;

typedef struct render_command
{
    render_command_type Type;
    
    u32 VertexCount;
    umm VertexBufferOffset;
    
} render_command;

typedef struct render_group
{
    v4 ClearColor;
    
    render_command Commands[MaxRenderCommands];
    u32 CurrentCommand;
    
    u32 Width;
    u32 Height;
    
    u8 VertexBuffer[VertexBufferSize];
    umm VertexBufferAt; 
    
} render_group;

inline render_command *
GetRenderCommand(render_group *Group, render_command_type Type)
{
    render_command *Result = Group->Commands + Group->CurrentCommand;
    
    if(Result->Type != Type)
    {
        if(Group->CurrentCommand >= MaxRenderCommands)
        {
            LogWarning("Render command wrapped");
            Group->CurrentCommand = 0;
            Result = Group->Commands + Group->CurrentCommand;
        }
        
        ++Group->CurrentCommand;
        Result = Group->Commands + Group->CurrentCommand;
        Result->Type = Type;
        Result->VertexCount = 0;
        Result->VertexBufferOffset = Group->VertexBufferAt;
        
    }
    
    return Result;
}

inline void
AddVertexInstance(render_group *Group, render_command *Command, v3 Offset, v2 Size, v4 Color, v4 UV)
{
    if((Group->VertexBufferAt + VertexInstanceSize) < VertexBufferSize)
    {
        vertex_instance *Instance = (vertex_instance *)(Group->VertexBuffer + Group->VertexBufferAt);
        Group->VertexBufferAt += VertexInstanceSize;
        ++Command->VertexCount;
        Instance->Offset = Offset;
        Instance->Size = Size;
        Instance->Color = Color;
        Instance->UV = UV;
    }
    else
    {
        LogWarning("Vertex buffer full");
    }
}


#define KENGINE_RENDERER_H
#endif //KENGINE_RENDERER_H

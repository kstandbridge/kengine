
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

render_command *
PushRenderCommandRect(render_group *Group, rectangle2 Bounds, f32 Depth, v4 Color)
{
    render_command *Result = GetRenderCommand(Group, RenderCommand_Rect);
    
    Bounds; Depth; Color;
    AddVertexInstance(Group, Result, V3(Bounds.Min.X, Bounds.Min.Y, Depth), V2Subtract(Bounds.Max, Bounds.Min), Color, V4(0, 0, 1, 1));
    
    return Result;
}

void
PushRenderCommandRectOutline(render_group *Group, rectangle2 Bounds, f32 Depth, f32 Thickness, v4 Color)
{
    PushRenderCommandRect(Group, Rectangle2(Bounds.Min, V2(Bounds.Min.X + Thickness, Bounds.Max.Y)), Depth, Color);
    PushRenderCommandRect(Group, Rectangle2(V2(Bounds.Max.X - Thickness, Bounds.Min.Y), Bounds.Max), Depth, Color);
    PushRenderCommandRect(Group, Rectangle2(Bounds.Min, V2(Bounds.Max.X, Bounds.Min.Y + Thickness)), Depth, Color);
    PushRenderCommandRect(Group, Rectangle2(V2(Bounds.Min.X, Bounds.Max.Y - Thickness), Bounds.Max), Depth, Color);
}

void
PushRenderCommandAlternateRectOutline(render_group *Group, rectangle2 Bounds, f32 Depth, f32 Thickness, v4 Color1, v4 Color2)
{
    PushRenderCommandRect(Group, Rectangle2(Bounds.Min, V2(Bounds.Min.X + Thickness, Bounds.Max.Y)), Depth, Color1);
    PushRenderCommandRect(Group, Rectangle2(V2(Bounds.Max.X - Thickness, Bounds.Min.Y), Bounds.Max), Depth, Color2);
    PushRenderCommandRect(Group, Rectangle2(Bounds.Min, V2(Bounds.Max.X, Bounds.Min.Y + Thickness)), Depth, Color1);
    PushRenderCommandRect(Group, Rectangle2(V2(Bounds.Min.X, Bounds.Max.Y - Thickness), Bounds.Max), Depth, Color2);
}

render_command *
PushRenderCommandSprite(render_group *Group, v2 Offset, f32 Depth, v2 Size, v4 Color, v4 UV, void *Texture)
{
    render_command *Result = GetRenderCommand(Group, RenderCommand_Sprite);
    // TODO(kstandbridge): Multiple sprite sheets?
    Result->Context = Texture;
    
    AddVertexInstance(Group, Result, V3(Offset.X, Offset.Y, Depth), Size, Color, UV);
    
    return Result;
}

render_command *
PushRenderCommandGlyph(render_group *Group, v2 Offset, f32 Depth, v2 Size, v4 Color, v4 UV, void *Texture)
{
    render_command *Result = GetRenderCommand(Group, RenderCommand_Glyph);
    // TODO(kstandbridge): Multiple sprite sheets?
    Result->Context = Texture;
    
    AddVertexInstance(Group, Result, V3(Offset.X, Offset.Y, Depth), 
                      Size, Color, UV);
    
    return Result;
}

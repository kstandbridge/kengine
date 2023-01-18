
inline render_command *
PushRenderCommandRect(render_group *Group, rectangle2 Bounds, f32 Depth, v4 Color)
{
    render_command *Result = 0;
    
    if(Group->CurrentCommand >= Group->MaxCommands)
    {
        Group->CurrentCommand = 0;
        LogWarning("Render commands buffer wrapped around");
    }
    
    Result = Group->Commands + Group->CurrentCommand;
    
    ++Group->CurrentCommand;
    Result->Type = RenderCommand_Rect;
    Result->Offset = Bounds.Min;
    Result->Depth = Depth;
    Result->Size = V2Subtract(Bounds.Max, Bounds.Min);
    Result->Color = Color;
    
    return Result;
}

inline render_command *
PushRenderCommandGlyph(render_group *Group, v2 Offset, f32 Depth, v2 Size, v4 Color, v4 UV)
{
    render_command *Result;
    
    if(Group->CurrentCommand >= Group->MaxCommands)
    {
        Group->CurrentCommand = 0;
        LogWarning("Render commands buffer wrapped around");
    }
    
    Result = Group->Commands + Group->CurrentCommand;
    
    ++Group->CurrentCommand;
    Result->Type = RenderCommand_Glyph;
    Result->Offset = Offset;
    Result->Depth = Depth;
    Result->Size = Size;
    Result->Color = Color;
    Result->UV = UV;
    
    return Result;
}

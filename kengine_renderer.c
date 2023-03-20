
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

inline void
PushRenderCommandRectOutline(render_group *Group, rectangle2 Bounds, f32 Depth, v4 Color, f32 Thickness)
{
    PushRenderCommandRect(Group, Rectangle2(Bounds.Min, V2(Bounds.Min.X + Thickness, Bounds.Max.Y)), Depth, Color);
    PushRenderCommandRect(Group, Rectangle2(V2(Bounds.Max.X - Thickness, Bounds.Min.Y), Bounds.Max), Depth, Color);
    
    PushRenderCommandRect(Group, Rectangle2(Bounds.Min, V2(Bounds.Max.X, Bounds.Min.Y + Thickness)), Depth, Color);
    
    PushRenderCommandRect(Group, Rectangle2(V2(Bounds.Min.X, Bounds.Max.Y - Thickness), Bounds.Max), Depth, Color);
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

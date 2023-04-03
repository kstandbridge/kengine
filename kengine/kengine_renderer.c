
inline render_command *
PushRenderCommandRect(render_group *Group, rectangle2 Bounds, f32 Depth, v4 Color)
{
    render_command *Result = GetRenderCommand(Group, RenderCommand_Rect);
    
    Bounds; Depth; Color;
    AddVertexInstance(Group, Result, V3(Bounds.Min.X, Bounds.Min.Y, Depth), V2Subtract(Bounds.Max, Bounds.Min), Color, V4(0, 0, 1, 1));
    
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
PushRenderCommandSprite(render_group *Group, v2 Offset, f32 Depth, v2 Size, v4 Color, v4 UV, void *Texture)
{
    render_command *Result = GetRenderCommand(Group, RenderCommand_Sprite);
    // TODO(kstandbridge): Multiple sprite sheets?
    Result->Context = Texture;
    
    AddVertexInstance(Group, Result, V3(Offset.X, Offset.Y, Depth), Size, Color, UV);
    
    return Result;
}

inline render_command *
PushRenderCommandGlyph(render_group *Group, v2 Offset, f32 Depth, v2 Size, v4 Color, v4 UV, void *Texture)
{
    render_command *Result = GetRenderCommand(Group, RenderCommand_Glyph);
    // TODO(kstandbridge): Multiple sprite sheets?
    Result->Context = Texture;
    
    AddVertexInstance(Group, Result, V3(Offset.X, Offset.Y, Depth), 
                      Size, Color, UV);
    
    return Result;
}

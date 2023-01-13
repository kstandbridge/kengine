
inline render_command *
PushRenderCommandRect(render_group *Group, rectangle2 Bounds, f32 Depth, v4 Color)
{
    render_command *Result;
    
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
    Result->Rect.Color = Color;
    
    return Result;
}

inline render_command *
PushRenderCommandText(render_group *Group, f32 Scale, v2 Offset, f32 Depth, v4 Color, string Text)
{
    render_command *Result;
    
    if(Group->CurrentCommand >= Group->MaxCommands)
    {
        Group->CurrentCommand = 0;
        LogWarning("Render commands buffer wrapped around");
    }
    
    Result = Group->Commands + Group->CurrentCommand;
    
    ++Group->CurrentCommand;
    Result->Type = RenderCommand_Text;
    Result->Offset = Offset;
    Result->Depth = Depth;
    Result->Size = V2(Scale, Scale);
    Result->Text.Color = Color;
    Result->Text.Text = Text;
    
    return Result;
}


inline render_command *
PushRenderCommand(render_group *Group, render_command_type Type)
{
    render_command *Result;
    
    if(Group->CurrentCommand >= Group->MaxCommands)
    {
        Group->CurrentCommand = 0;
        LogWarning("Render commands buffer wrapped around");
    }
    
    Result = Group->Commands + Group->CurrentCommand;
    
    ++Group->CurrentCommand;
    Result->Type = Type;
    
    return Result;
}

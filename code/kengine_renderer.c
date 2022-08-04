
inline void
PushRenderEntryClear(render_group *Group, v4 Color, f32 SortKey)
{
    render_commands *Commands = Group->Commands;
    
    u32 Size = sizeof(render_group_command_clear) + sizeof(render_group_command_header);
    if((Commands->PushBufferSize + Size) < (Commands->SortEntryAt - sizeof(sort_entry)))
    {
        render_group_command_header *Header = (render_group_command_header *)(Commands->PushBufferBase + Commands->PushBufferSize);
        Header->Type = SafeTruncateU32ToU16(RenderGroupCommandType_Clear);
        Header->ClipRectIndex = SafeTruncateU32ToU16(Group->CurrentClipRectIndex);
        render_group_command_clear *Command = (render_group_command_clear *)((u8 *)Header + sizeof(*Header));
        Command->Color = Color;
        
        Commands->SortEntryAt -= sizeof(sort_entry);
        sort_entry *Entry = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
        Entry->SortKey = SortKey;
        Entry->Index = Commands->PushBufferSize;
        
        Commands->PushBufferSize += Size;
        ++Commands->PushBufferElementCount;
    }
    else
    {
        // TODO(kstandbridge): Error ran out of buffer space
        InvalidCodePath;
    }
}

#if 0
internal void
PushRectangle(render_group *RenderGroup, object_transform Transform, rectangle2 Rectangle, v4 Color)
{
    
}
#endif

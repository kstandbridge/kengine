
internal void *
PushRenderCommand(render_group *Group, render_group_command_type Type, u32 Size, f32 SortKey)
{
    void *Result = 0;
    
    render_commands *Commands = Group->Commands;
    
    Size += sizeof(render_group_command_header);
    if((Commands->PushBufferSize + Size) < (Commands->SortEntryAt - sizeof(sort_entry)))
    {
        render_group_command_header *Header = (render_group_command_header *)(Commands->PushBufferBase + Commands->PushBufferSize);
        Header->Type = SafeTruncateU32ToU16(Type);
        Header->ClipRectIndex = SafeTruncateU32ToU16(Group->CurrentClipRectIndex);
        
        Commands->SortEntryAt -= sizeof(sort_entry);
        sort_entry *Entry = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
        Entry->SortKey = SortKey;
        Entry->Index = Commands->PushBufferSize;
        
        Result = (void *)((u8 *)Header + sizeof(*Header));
        
        Commands->PushBufferSize += Size;
        ++Commands->PushBufferElementCount;
    }
    else
    {
        // TODO(kstandbridge): Error ran out of buffer space
        InvalidCodePath;
    }
    
    return Result;
}

internal u16
PushRenderCommandClipRectangle(render_group *Group, rectangle2i Bounds)
{
    u16 Result = 0;
    
    render_commands *Commands = Group->Commands;
    
    u32 Size = sizeof(render_command_cliprect);
    if((Commands->PushBufferSize + Size) < (Commands->SortEntryAt - sizeof(sort_entry)))
    {
        render_command_cliprect *Command = (render_command_cliprect *)
        (Commands->PushBufferBase + Commands->PushBufferSize);
        Commands->PushBufferSize += Size;
        
        Result = Commands->ClipRectCount++;
        
        if(Group->Commands->LastRect)
        {
            Group->Commands->LastRect = Group->Commands->LastRect->Next = Command;
        }
        else
        {
            Group->Commands->LastRect = Group->Commands->FirstRect = Command;
        }
        Command->Next = 0;
        Command->Bounds = Bounds;
    }
    
    return Result;
}

internal void
PushRenderCommandClear(render_group *Group, f32 SortKey, v4 Color)
{
    render_group_command_clear *Command = PushRenderCommand(Group, RenderGroupCommandType_Clear, sizeof(render_group_command_clear), SortKey);
    if(Command)
    {
        Command->Color = Color;
    }
    else
    {
        // TODO(kstandbridge): Error pushing clear command
        InvalidCodePath;
    }
}

internal void
PushRenderCommandRectangle(render_group *Group, v4 Color, rectangle2 Bounds, f32 SortKey)
{
    render_group_command_rectangle *Command = PushRenderCommand(Group, RenderGroupCommandType_Rectangle, sizeof(render_group_command_rectangle), SortKey);
    if(Command)
    {
        Command->Color = Color;
        Command->Bounds = Bounds;
    }
    else
    {
        // TODO(kstandbridge): Error pushing rectangle command
        InvalidCodePath;
    }
}

internal void
PushRenderCommandRectangleOutline(render_group *Group, f32 Thickness, v4 Color, rectangle2 Bounds, f32 SortKey)
{
    v2 Min = Bounds.Min;
    v2 Max = Bounds.Max;
    
    PushRenderCommandRectangle(Group, Color, Rectangle2(V2(Min.X, Max.Y - Thickness), V2(Max.X, Max.Y)), SortKey);
    PushRenderCommandRectangle(Group, Color, Rectangle2(V2(Min.X, Min.Y), V2(Max.X, Min.Y + Thickness)), SortKey);
    
    PushRenderCommandRectangle(Group, Color, Rectangle2(V2(Min.X, Min.Y), V2(Min.X + Thickness, Max.Y)), SortKey);
    PushRenderCommandRectangle(Group, Color, Rectangle2(V2(Max.X - Thickness, Min.Y), V2(Max.X, Max.Y)), SortKey);
}


inline bitmap_dim
GetBitmapDim(loaded_bitmap *Bitmap, f32 Height, v2 Offset)
{
    bitmap_dim Result;
    
    Result.Size = V2(Height*Bitmap->WidthOverHeight, Height);
    Result.Align = Hadamard(Bitmap->AlignPercentage, Result.Size);
    Result.P = V2Subtract(Offset, Result.Align);
    
    return Result;
}

internal void
PushRenderCommandBitmap(render_group *Group, loaded_bitmap *Bitmap, f32 Height, v2 Offset, v4 Color, f32 SortKey)
{
    bitmap_dim Dim = GetBitmapDim(Bitmap, Height, Offset);
    render_group_command_bitmap *Command = PushRenderCommand(Group, RenderGroupCommandType_Bitmap, sizeof(render_group_command_bitmap), SortKey);
    if(Command)
    {
        Command->Bitmap = Bitmap;
        Command->Color = Color;
        Command->P = Dim.P;
        Command->Dim = Dim.Size;
    }
    else
    {
        // TODO(kstandbridge): Error pushing bitmap command
        InvalidCodePath;
    }
}

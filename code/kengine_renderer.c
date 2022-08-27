internal void
SortRenderCommands(render_commands *Commands, void *SortMemory)
{
    sort_entry *Entries = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
    //BubbleSort(Commands->PushBufferElementCount, Entries);
    //MergeSort(Commands->PushBufferElementCount, Entries, (sort_entry *)SortMemory);
    RadixSort(Commands->PushBufferElementCount, Entries, (sort_entry *)SortMemory);
    
#if KENGINE_SLOW
    if(Commands->PushBufferElementCount)
    {
        for(u32 Index = 0;
            Index < (Commands->PushBufferElementCount - 1);
            ++Index)
        {
            sort_entry *EntryA = Entries + Index;
            sort_entry *EntryB = EntryA + 1;
            
            Assert(EntryA->SortKey <= EntryB->SortKey);
        }
    }
#endif
}


internal void
LinearizeClipRects(render_commands *Commands, void *ClipMemory)
{
    render_command_cliprect *Out = (render_command_cliprect *)ClipMemory;
    for(render_command_cliprect *Rect = Commands->FirstRect;
        Rect;
        Rect = Rect->Next)
    {
        *Out++ = *Rect;
    }
    Commands->ClipRects = (render_command_cliprect *)ClipMemory;
}


inline render_commands
BeginRenderCommands(u32 PushBufferSize, void *PushBufferBase, s32 Width, s32 Height)
{
    render_commands Result;
    ZeroStruct(Result);
    
    Result.Width = Width;
    Result.Height = Height;
    Result.MaxPushBufferSize = PushBufferSize;
    Result.PushBufferBase = PushBufferBase;
    Result.SortEntryAt = PushBufferSize;
    
    return Result;
}

inline void
EndRenderCommands(render_commands *RenderCommands)
{
    // TODO(kstandbridge): Do we need to do any clean up?
    RenderCommands;
}

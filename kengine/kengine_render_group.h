typedef struct app_render_commands
{
    u32 Width;
    u32 Height;

    u32 MaxPushBufferSize;
    u32 PushBufferSize;
    u8 *PushBufferBase;

    u32 PushBufferElementCount;
    u32 SortEntryAt;
} app_render_commands;

typedef struct tile_sort_entry
{
    f32 SortKey;
    u32 PushBufferOffet;
} tile_sort_entry;

typedef enum render_group_entry_type
{
    RenderGroupEntryType_render_entry_clear,
    RenderGroupEntryType_render_entry_rectangle,
    RenderGroupEntryType_render_entry_bitmap,
} render_group_entry_type;

typedef struct render_group_entry_header
{
    render_group_entry_type Type;
} render_group_entry_header;

typedef struct render_entry_clear
{
    v4 Color;
} render_entry_clear;

typedef struct render_entry_rectangle
{
    v4 Color;
    v2 P;
    v2 Dim;
} render_entry_rectangle;

typedef struct render_entry_bitmap
{
    loaded_bitmap *Bitmap;

    v4 Color;
    v2 P;
    v2 Size;
} render_entry_bitmap;

typedef struct render_group
{
    app_render_commands *Commands;
} render_group;

#define PushRenderElement(Group, type, SortKey) (type *)PushRenderElement_(Group, sizeof(type), RenderGroupEntryType_##type, SortKey)
internal void *
PushRenderElement_(render_group *Group, u32 Size, render_group_entry_type Type, f32 SortKey)
{
    app_render_commands *Commands = Group->Commands;

    void *Result = 0;

    Size += sizeof(render_group_entry_header);
    if((Commands->PushBufferSize + Size) < (Commands->SortEntryAt - sizeof(tile_sort_entry)))
    {
        render_group_entry_header *Header = (render_group_entry_header *)(Commands->PushBufferBase + Commands->PushBufferSize);
        Header->Type = Type;
        Result = (u8 *)Header + sizeof(*Header);

        Commands->SortEntryAt -= sizeof(tile_sort_entry);
        tile_sort_entry *Entry = (tile_sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
        Entry->SortKey = SortKey;
        Entry->PushBufferOffet = Commands->PushBufferSize;

        Commands->PushBufferSize += Size;
        ++Commands->PushBufferElementCount;

    }
    else
    {
        Assert(!"Push buffer full!");
    }

    return Result;
}

internal void
PushClear(render_group *Group, v4 Color)
{
    render_entry_clear *Entry = PushRenderElement(Group, render_entry_clear, F32Min);
    if(Entry)
    {
        Entry->Color = Color;
    }
}

internal void
PushRect(render_group *Group, v2 Offset, v2 Dim, v4 Color, f32 SortKey)
{
    render_entry_rectangle *Entry = PushRenderElement(Group, render_entry_rectangle, SortKey);
    if(Entry)
    {
        Entry->P = Offset;
        Entry->Color = Color;
        Entry->Dim = Dim;
    }
}

internal void
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, v2 Offset, v2 Dim, v4 Color, f32 SortKey)
{
    render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap, SortKey);
    if(Entry)
    {
        Entry->Bitmap = Bitmap;
        Entry->P = Offset;
        Entry->Size = Dim;
        Entry->Color = Color;
    }
}

inline void
Swap(tile_sort_entry *A, tile_sort_entry *B)
{
    tile_sort_entry Temp = *B;
    *B = *A;
    *A = Temp;
}

internal void
BubbleSort(u32 Count, tile_sort_entry *First)
{
    for(u32 Outer = 0;
        Outer < Count;
        ++Outer)
    {
        b32 ListIsSorted = true;
        for(u32 Inner = 0;
            Inner < (Count - 1);
            ++Inner)
        {
            tile_sort_entry *EntryA = First + Inner;
            tile_sort_entry *EntryB = EntryA + 1;

            if(EntryA->SortKey > EntryB->SortKey)
            {
                Swap(EntryA, EntryB);
                ListIsSorted = false;
            }
        }

        if(ListIsSorted)
        {
            break;
        }
    }
}

internal void
SortEntries(app_render_commands *Commands)
{
    u32 Count = Commands->PushBufferElementCount;
    tile_sort_entry *Entries = (tile_sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);

    BubbleSort(Count, Entries);

#if KENGINE_SLOW
    if(Count)
    {
        for(u32 Index = 0;
            Index < (Count - 1);
            ++Index)
        {
            tile_sort_entry *EntryA = Entries + Index;
            tile_sort_entry *EntryB = EntryA + 1;

            Assert(EntryA->SortKey <= EntryB->SortKey);
        }
    }
#endif
}
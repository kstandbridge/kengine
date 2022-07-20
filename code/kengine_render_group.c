
#define PushRenderElement(Group, Type, SortKey) (Type *)PushRenderElement_(Group, sizeof(Type), RenderGroupEntry_##Type, SortKey)
inline void *
PushRenderElement_(render_group *Group, umm Size, render_group_entry_type Type, f32 SortKey)
{
    app_render_commands *Commands = Group->Commands;
    
    void *Result = 0;
    
    Size += sizeof(render_group_entry_header);
    
    if((Commands->PushBufferSize + Size) < Commands->MaxPushBufferSize)
    {
        render_group_entry_header *Header = (render_group_entry_header *)(Commands->PushBufferBase + Commands->PushBufferSize);
        Header->Type = Type;
        Result = (u8 *)Header + sizeof(*Header);
        
        Commands->SortEntryAt -= sizeof(tile_sort_entry);
        tile_sort_entry *Entry = (tile_sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
        Entry->SortKey = SortKey;
        Entry->PushBufferOffset = Commands->PushBufferSize;
        
        Commands->PushBufferSize += Size;
        ++Commands->PushBufferElementCount;
    }
    else
    {
        InvalidCodePath;
    }
    
    return Result;
}

inline void
PushClear(render_group *Group, v4 Color)
{
    render_entry_clear *Clear = PushRenderElement(Group, render_entry_clear, F32Min);
    if(Clear)
    {
        Clear->Color = Color;
    }
}

inline void
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, f32 Height, v2 Offset, v4 Color, f32 Angle, f32 SortKey)
{
    v2 Dim = V2(Height*Bitmap->WidthOverHeight, Height);
    v2 Align = Hadamard(Bitmap->AlignPercentage, Dim);
    v2 P = V2Subtract(Offset, Align);
    render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap, SortKey);
    if(Entry)
    {
        Entry->Bitmap = Bitmap;
        Entry->P = P;
        Entry->Color = Color;
        Entry->Dim = Dim;
        Entry->Angle = Angle;
    }
}

inline void
PushRect(render_group *Group, v2 P, v2 Dim, v4 Color, v4 AltColor, f32 SortKey)
{
    render_entry_rectangle *Entry = PushRenderElement(Group, render_entry_rectangle, SortKey);
    if(Entry)
    {
        Entry->P = P;
        Entry->Color = Color;
        Entry->AltColor = AltColor;
        Entry->Dim = Dim;
    }
}

inline void
PushRectOutline(render_group *Group, v2 P, v2 Dim, v4 Color, v4 AltColor, f32 Scale, f32 SortKey)
{
    f32 Thickness = Scale*3.0f;
    if(Thickness < 1.0f)
    {
        Thickness = 1.0f;
    }
    
    
    PushRect(Group, P, V2(Dim.X, Thickness), Color, AltColor, SortKey);
    PushRect(Group, V2Add(P, V2(0.0f, Dim.Y - Thickness)), V2(Dim.X, Thickness), Color, AltColor, SortKey);
    
    PushRect(Group, P, V2(Thickness, Dim.Y), Color, AltColor, SortKey);
    PushRect(Group, V2Add(P, V2(Dim.X - Thickness, 0.0f)), V2(Thickness, Dim.Y), Color, AltColor, SortKey);
}

inline void
PushText(render_group *Group, v2 P, v4 Color, string Text, f32 SortKey)
{
    render_entry_text *Entry = PushRenderElement(Group, render_entry_text, SortKey);
    if(Entry)
    {
        Entry->P = P;
        Entry->Color = Color;
        Entry->Text = Text;
    }
}

internal render_group
BeginRenderGroup(assets *Assets, app_render_commands *RenderCommands)
{
    render_group Result;
    
    Result.Assets = Assets;
    Result.Commands = RenderCommands;
    
    return Result;
    
}

internal void
EndRenderGroup(render_group *RenderGroup)
{
    // TODO(kstandbridge): Missing resource count?
    RenderGroup;
}

typedef enum text_op_type
{
    TextOp_DrawText,
    TextOp_DrawSelectedText,
    TextOp_SizeText,
} text_op_type;
internal rectangle2
TextOpInternal(text_op_type Op, render_group *RenderGroup, assets *Assets, v2 P, f32 Scale, string Str, v4 TextColor, v4 SelectedTextColor, v4 SelectedBackgroundColor, u32 SelectedStartIndex, u32 SelectedEndIndex, f32 SelectedHeight, f32 Angle, rectangle2 ClipRect,
               f32 SortKey)
{
    
    
    // TODO(kstandbridge): RenderGroup can be null, bad design
    
    rectangle2 Result = InvertedInfinityRectangle2();
    
    f32 AtX = P.X;
    f32 AtY = P.Y;
    u32 PrevCodePoint = 0;
    b32 CaretDrawn = false;
    for(u32 Index = 0;
        Index < Str.Size;
        ++Index)
    {
        u32 CodePoint = Str.Data[Index];
        
        if(Str.Data[Index] == '\r')
        {
            // NOTE(kstandbridge): Ignore
        }
        else if(Str.Data[Index] == '\n')
        {
            AtY -= Platform.DEBUGGetLineAdvance()*Scale;
            AtX = P.X;
        }
        else
        {
            if((Str.Data[0] == '\\') &&
               (IsHex(Str.Data[1])) &&
               (IsHex(Str.Data[2])) &&
               (IsHex(Str.Data[3])) &&
               (IsHex(Str.Data[4])))
            {
                CodePoint = ((GetHex(Str.Data[1]) << 12) |
                             (GetHex(Str.Data[2]) << 8) |
                             (GetHex(Str.Data[3]) << 4) |
                             (GetHex(Str.Data[4]) << 0));
                Index += 4;
            }
            
            if(CodePoint && CodePoint != ' ')
            {        
                if(Assets->Glyphs[CodePoint].Memory == 0)
                {
                    // TODO(kstandbridge): This should be threaded
                    Assets->Glyphs[CodePoint] = Platform.DEBUGGetGlyphForCodePoint(&Assets->Arena, CodePoint);
                }
                
                loaded_bitmap *Bitmap = Assets->Glyphs + CodePoint;
                Assert(Bitmap->Memory);
                f32 Height = (f32)Bitmap->Height;
                v2 Offset = V2(AtX, AtY);
                if(Op == TextOp_DrawText)
                {
                    Assert(RenderGroup);
                    
                    if(IsInRectangle(ClipRect, Offset))
                    {
                        PushBitmap(RenderGroup, Bitmap, Height, Offset, TextColor, Angle, SortKey);
                    }
                }
                else if(Op == TextOp_DrawSelectedText)
                {
                    Assert(RenderGroup);
                    
                    if(IsInRectangle(ClipRect, Offset))
                    {                        
                        if((Index >= SelectedStartIndex) &&
                           (Index < SelectedEndIndex))
                        {
                            f32 AdvanceX = Scale*Platform.DEBUGGetHorizontalAdvanceForPair(CodePoint, PrevCodePoint);
                            CaretDrawn = true;
                            
                            PushRect(RenderGroup, V2Subtract(Offset, V2(Scale, SelectedHeight*0.2f)), V2(AdvanceX + Scale*2.0f, SelectedHeight), SelectedBackgroundColor, SelectedBackgroundColor, SortKey);
                            
                            PushBitmap(RenderGroup, Bitmap, Height, Offset, SelectedTextColor, 0.0f, SortKey + 0.5f);
                        }
                        else
                        {
                            PushBitmap(RenderGroup, Bitmap, Height, Offset, TextColor, 0.0f, SortKey);
                        }
                        if(Index == SelectedStartIndex)
                        {
                            if(SelectedStartIndex == SelectedEndIndex)
                            {
                                CaretDrawn = true;
                                PushRect(RenderGroup, V2(AtX, AtY-SelectedHeight*0.2f), V2(3.0f, SelectedHeight), SelectedBackgroundColor, SelectedBackgroundColor, SortKey);
                            }
                        }
                    }
                    
                }
                else
                {
                    Assert(Op == TextOp_SizeText);
                    v2 Dim = V2(Height*Bitmap->WidthOverHeight, Height);
                    v2 Align = Hadamard(Bitmap->AlignPercentage, Dim);
                    v2 GlyphP = V2Subtract(Offset, Align);
                    rectangle2 GlyphDim = RectMinDim(GlyphP, Dim);
                    Result = Union(Result, GlyphDim);
                }
                
                PrevCodePoint = CodePoint;
            }
            
            f32 AdvanceX = Scale*Platform.DEBUGGetHorizontalAdvanceForPair(PrevCodePoint, CodePoint);
            
            if(CodePoint == ' ')
            {
                Result.Max.X += AdvanceX;
                if(Op == TextOp_DrawSelectedText)
                {
                    // TODO(kstandbridge): Duplicated code
                    Assert(RenderGroup);
                    v2 Offset = V2(AtX, AtY);
                    if(IsInRectangle(ClipRect, Offset))
                    {                        
                        if((Index >= SelectedStartIndex) &&
                           (Index < SelectedEndIndex))
                        {
                            CaretDrawn = true;
                            
                            PushRect(RenderGroup, V2Subtract(V2(AtX, AtY), V2(Scale, SelectedHeight*0.2f)), V2(AdvanceX + Scale*2.0f, SelectedHeight), SelectedBackgroundColor, SelectedBackgroundColor, SortKey);
                        }
                        
                        if(Index == SelectedStartIndex)
                        {
                            if(SelectedStartIndex == SelectedEndIndex)
                            {
                                CaretDrawn = true;;
                                v2 CaretP = V2Subtract(Offset, V2(Scale, SelectedHeight*0.2f));
                                PushRect(RenderGroup, CaretP, V2(3.0f, SelectedHeight), SelectedBackgroundColor, SelectedBackgroundColor, SortKey);
                            }
                        }
                    }
                }
            }
            
            AtX += AdvanceX;
        }
    }
    
    if(Op == TextOp_DrawSelectedText)
    {
        if(!CaretDrawn)
        {
            v2 Offset = V2(AtX, AtY);
            if(IsInRectangle(ClipRect, Offset))
            {
                v2 CaretP = V2Subtract(Offset, V2(Scale, SelectedHeight*0.2f));
                PushRect(RenderGroup, CaretP, V2(3.0f, SelectedHeight), SelectedBackgroundColor, SelectedBackgroundColor, SortKey);
            }
        }
    }
    
    return Result;
}

inline void
WriteSelectedLine(render_group *RenderGroup, assets *Assets, v2 P, f32 Scale, string Text, v4 TextColor, v4 SelectedTextColor, v4 SelectedBackgroundColor, u32 SelectedStartIndex, u32 SelectedEndIndex, f32 SelectedHeight, rectangle2 ClipRect, f32 SortKey)
{
    TextOpInternal(TextOp_DrawSelectedText, RenderGroup, Assets, P, Scale, Text, TextColor, SelectedTextColor, SelectedBackgroundColor, SelectedStartIndex, SelectedEndIndex, SelectedHeight, 0.0f, ClipRect, SortKey);
}

inline void
WriteLine(render_group *RenderGroup, assets *Assets, v2 P, f32 Scale, string Text, v4 TextColor, rectangle2 ClipRect, f32 SortKey)
{
    TextOpInternal(TextOp_DrawText, RenderGroup, Assets, P, Scale, Text, TextColor, V4Set1(0.0f), V4Set1(0.0f), 0, 0, 0, 0.0f, ClipRect, SortKey);
}

inline void
WriteRotatedLine(render_group *RenderGroup, assets *Assets, v2 P, f32 Scale, string Text, v4 TextColor, f32 Angle, rectangle2 ClipRect, f32 SortKey)
{
    TextOpInternal(TextOp_DrawText, RenderGroup, Assets, P, Scale, Text, TextColor, V4Set1(0.0f), V4Set1(0.0f), 0, 0, 0, Angle, ClipRect, SortKey);
}

inline rectangle2
GetTextSize(assets *Assets, f32 Scale, string Text)
{
    rectangle2 Result = TextOpInternal(TextOp_SizeText, 0, Assets, V2Set1(0.0f), Scale, Text, V4Set1(0.0f),  V4Set1(0.0f), V4Set1(0.0f), 0, 0, 0, 0.0f, InfinityRectangle2(), 0.0f);
    
    return Result;
}

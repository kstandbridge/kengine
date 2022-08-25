
inline void
ClearInteraction(ui_interaction *Interaction)
{
    Interaction->Type = Interaction_None;
    Interaction->Generic = 0;
}

typedef enum
{
    TextOpText_Draw,
    TextOpText_Size,
} text_op_type;
internal rectangle2
TextOp_(render_group *RenderGroup, text_op_type Op, f32 Scale, v2 P, v4 Color, string Text)
{
    rectangle2 Result = Rectangle2InvertedInfinity();
    
    memory_arena *Arena = RenderGroup->Arena;
    loaded_glyph *Glyphs = RenderGroup->Glyphs;
    
    f32 AtX = P.X;
    f32 AtY = P.Y;
    u32 PrevCodePoint = 0;
    
    for(u32 Index = 0;
        Index < Text.Size;
        ++Index)
    {
        u32 CodePoint = Text.Data[Index];
        if(Text.Data[Index] == '\r')
        {
            // NOTE(kstandbridge): Ignore
        }
        else if(Text.Data[Index] == '\n')
        {
            AtY -= Platform.GetVerticleAdvance()*Scale;
            AtX = P.X;
        }
        else
        {
            if((Text.Data[0] == '\\') &&
               (IsHex(Text.Data[1])) &&
               (IsHex(Text.Data[2])) &&
               (IsHex(Text.Data[3])) &&
               (IsHex(Text.Data[4])))
            {
                CodePoint = ((GetHex(Text.Data[1]) << 12) |
                             (GetHex(Text.Data[2]) << 8) |
                             (GetHex(Text.Data[3]) << 4) |
                             (GetHex(Text.Data[4]) << 0));
                Index += 4;
            }
            
            if(CodePoint && CodePoint != ' ')
            {        
                if(Glyphs[CodePoint].Bitmap.Memory == 0)
                {
                    // TODO(kstandbridge): This should be threaded
                    Glyphs[CodePoint] = Platform.GetGlyphForCodePoint(Arena, CodePoint);
                }
                
                loaded_glyph *Glyph = Glyphs + CodePoint;
                Assert(Glyph->Bitmap.Memory);
                
                v2 Offset = V2(AtX, AtY);
                f32 BitmapScale = Scale*(f32)Glyph->Bitmap.Height;
                
                if(Op == TextOpText_Draw)
                {
                    PushRenderCommandBitmap(RenderGroup, &Glyph->Bitmap, BitmapScale, Offset, Color, 2.0f);
#if 0                    
                    PushRenderCommandBitmap(RenderGroup, &Glyph->Bitmap, BitmapScale, V2Add(Offset, V2(2.0f, -2.0f)), V4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
#endif
                    
                }
                else
                {
                    Assert(Op == TextOpText_Size);
                    
                    bitmap_dim Dim = GetBitmapDim(&Glyph->Bitmap, BitmapScale, Offset);
                    // NOTE(kstandbridge): Glyphs have a 1 pixel border
                    Dim.Size = V2Add(Dim.Size, V2Set1(2.0f));
                    rectangle2 GlyphDim = Rectangle2MinDim(Dim.P, Dim.Size);
                    Result = Rectangle2Union(Result, GlyphDim);
                }
                
                PrevCodePoint = CodePoint;
                
                f32 AdvanceX = (Scale*Platform.GetHorizontalAdvance(PrevCodePoint, CodePoint)) + Glyph->KerningChange;
                loaded_glyph *PreviousGlyph = Glyphs + PrevCodePoint;
                AdvanceX += PreviousGlyph->KerningChange;
                
                AtX += AdvanceX;
            }
            else
            {
                f32 AdvanceX = (Scale*Platform.GetHorizontalAdvance(PrevCodePoint, CodePoint));
                
                AtX += AdvanceX;
            }
            
            
        }
    }
    
    return Result;
}

internal void
Interact(ui_state *UIState, app_input *Input)
{
    u32 TransitionCount = Input->MouseButtons[MouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[MouseButton_Left].EndedDown;
    if(TransitionCount % 2)
    {
        MouseButton = !MouseButton;
    }
    
    for(u32 TransitionIndex = 0;
        TransitionIndex <= TransitionCount;
        ++TransitionIndex)
    {
        b32 MouseMove = false;
        b32 MouseDown = false;
        b32 MouseUp = false;
        if(TransitionIndex == 0)
        {
            MouseMove = true;
        }
        else
        {
            MouseDown = MouseButton;
            MouseUp = !MouseButton;
        }
        
        b32 EndInteraction = false;
        
        switch(UIState->Interaction.Type)
        {
            case Interaction_ImmediateButton:
            case Interaction_Draggable:
            {
                UIState->ClickedId = UIState->Interaction.Id;
                if(MouseUp)
                {
                    UIState->NextToExecute = UIState->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_Select:
            {
                UIState->ClickedId = UIState->Interaction.Id;
                if(MouseUp)
                {
                    // TODO(kstandbridge): Select something?
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_None:
            {
                UIState->HotInteraction = UIState->NextHotInteraction;
                if(MouseDown)
                {
                    UIState->Interaction = UIState->HotInteraction;
                }
            } break;
            
            default:
            {
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            } break;
        }
        
        
        if(EndInteraction)
        {
            ClearInteraction(&UIState->Interaction);
        }
        
        MouseButton = !MouseButton;
    }
    
}

inline b32
InteractionIdsAreEqual(ui_interaction_id A, ui_interaction_id B)
{
    b32 Result = ((A.Value[0].U64 == B.Value[0].U64) &&
                  (A.Value[1].U64 == B.Value[1].U64));
    
    return Result;
}

inline b32
InteractionsAreEqual(ui_interaction A, ui_interaction B)
{
    b32 Result = (InteractionIdsAreEqual(A.Id, B.Id) &&
                  (A.Type == B.Type) &&
                  (A.Target == B.Target) &&
                  (A.Generic == A.Generic));
    
    return Result;
}

inline b32
InteractionIsHot(ui_state *UIState, ui_interaction Interaction)
{
    b32 Result = InteractionsAreEqual(UIState->HotInteraction, Interaction);
    
    if(Interaction.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline void
PushRenderCommandText(render_group *RenderGroup, f32 Scale, v2 P, v4 Color, string Text)
{
    // TODO(kstandbridge): NAMING Internally this is calling PushRenderCommandBitmap for each of the glyphs,
    // nothing is being drawn now, its a push call so shouldn't be DrawText
    
    TextOp_(RenderGroup, TextOpText_Draw, Scale, P, Color, Text);
}

inline rectangle2
GetTextSize(render_group *RenderGroup, f32 Scale, v2 P, string Text)
{
    rectangle2 Result = TextOp_(RenderGroup, TextOpText_Size, Scale, P, V4(1.0f, 1.0f, 1.0f, 1.0f), Text);
    return Result;
}

inline void
InitializeGridSize(ui_grid *Grid)
{
    Assert(!Grid->SizeInitialized);
    
    f32 StartX = Grid->Bounds.Min.X;
    f32 EndX = Grid->Bounds.Max.X;
    
    f32 StartY = Grid->Bounds.Min.Y;
    f32 EndY = Grid->Bounds.Max.Y;
    
    f32 TotalWidth = EndX - StartX;
    f32 TotalHeight = EndY - StartY;
    
    f32 UsedHeight = 0;
    
    u32 AutoHeightRows = Grid->Rows;
    for(ui_row *Row = Grid->FirstRow;
        Row;
        Row = Row->Next)
    {
        if(Row->Height)
        {
            UsedHeight += Row->Height;
            --AutoHeightRows;
        }
    }
    
    f32 HeightPerRow = AutoHeightRows ? (TotalHeight - UsedHeight) / AutoHeightRows : (TotalHeight - UsedHeight);
    
    f32 AtX = StartX;
    f32 AtY = EndY;
    for(ui_row *Row = Grid->FirstRow;
        Row;
        Row = Row->Next)
    {
        f32 UsedWidth = 0;
        u32 AutoWidthColumns = Grid->Columns;
        for(ui_column *Column = Row->FirstColumn;
            Column;
            Column = Column->Next)
        {
            if(Column->Width)
            {
                UsedWidth += Column->Width;
                --AutoWidthColumns;
            }
        }
        
        f32 WidthPerColumn = (AutoWidthColumns) ? (TotalWidth - UsedWidth) / AutoWidthColumns : (TotalWidth - UsedWidth);
        
        f32 CellHeight = (Row->Height) ? Row->Height : HeightPerRow;
        for(ui_column *Column = Row->FirstColumn;
            Column;
            Column = Column->Next)
        {
            f32 CellWidth = (Column->Width) ? Column->Width : WidthPerColumn;
            Column->Bounds = Rectangle2(V2(AtX, AtY - CellHeight),
                                        V2(AtX + CellWidth, AtY));
            AtX += CellWidth;
        }
        AtY -= CellHeight;
        AtX = StartX;
    }
    
    Grid->SizeInitialized = true;
}

inline rectangle2
GetCellBounds(ui_grid *Grid, u16 ColumnIndex, u16 RowIndex)
{
    BEGIN_BLOCK("GetCellBounds");
    
    if(!Grid->SizeInitialized)
    {
        InitializeGridSize(Grid);
    }
    Assert(RowIndex <= Grid->Rows);
    Assert(ColumnIndex <= Grid->Columns);
    
    ui_row *Row = Grid->FirstRow;
    Assert(Row);
    for(;
        RowIndex;
        --RowIndex)
    {
        Row = Row->Next;
    } 
    ui_column *Column = Row->FirstColumn;
    Assert(Column);
    for(;
        ColumnIndex;
        --ColumnIndex)
    {
        Column = Column->Next;
    }
    
    rectangle2 Result = Column->Bounds;
    
    END_BLOCK();
    
    return Result;
}

inline ui_element
BeginUIElement(ui_grid *Grid, u16 ColumnIndex, u16 RowIndex)
{
    ui_element Element;
    ZeroStruct(Element);
    
    rectangle2 CellBounds = GetCellBounds(Grid, ColumnIndex, RowIndex);
    
    Element.Grid = Grid;
    Element.Bounds = Rectangle2(V2Add(CellBounds.Min, V2(Grid->SpacingX, Grid->SpacingY)),
                                V2Subtract(CellBounds.Max, V2(Grid->SpacingX*2.0f, Grid->SpacingY*2.0f)));
    
    return Element;
}

inline void
SetUIElementDefaultAction(ui_element *Element, ui_interaction Interaction)
{
    Element->Interaction = Interaction;
}


inline void
EndUIElement(ui_element *Element)
{
    ui_grid *Grid = Element->Grid;
    ui_state *UIState = Grid->UIState;
    
    if(Element->Interaction.Type && Rectangle2IsIn(Element->Bounds, UIState->MouseP))
    {
        UIState->NextHotInteraction = Element->Interaction;
        Element->IsHot = true;
    }
    
    if(Element->Size)
    {
        // TODO(kstandbridge): Resize interaction
    }
}

internal ui_element
DrawTextElement_(ui_grid *Grid, render_group *RenderGroup, u16 ColumnIndex, u16 RowIndex, ui_interaction Interaction, string Text, 
                 element_colors Colors)
{
    ui_state *UIState = Grid->UIState;
    
    b32 IsHot = InteractionIsHot(UIState, Interaction);
    v4 TextColor = IsHot ? Colors.HotText : Colors.Text;
    v4 BackgroundColor = IsHot ? Colors.HotBackground : Colors.Background;
    v4 BorderColor = IsHot ? Colors.HotBorder : Colors.Border;
    
    if(InteractionIdsAreEqual(UIState->ClickedId, Interaction.Id) && 
       UIState->MouseDown)
    {
        TextColor = Colors.ClickedText;
        BackgroundColor = Colors.ClickedBackground;
        BorderColor = Colors.ClickedBorder;
    }
    
    
    ui_element Element = BeginUIElement(Grid, ColumnIndex, RowIndex);
    SetUIElementDefaultAction(&Element, Interaction);
    EndUIElement(&Element);
    
    f32 BorderThickness = 1.0f;
    if(InteractionIdsAreEqual(UIState->ClickedId, Interaction.Id) &&
       !Element.IsHot)
    {
        TextColor = Colors.SelectedText;
        BackgroundColor = Colors.SelectedBackground;
        BorderColor = Colors.SelectedBorder;
        BorderThickness = 2.0f;
    }
    
    v2 ElementDim = V2Subtract(Element.Bounds.Max, Element.Bounds.Min);
    v2 ElementHalfDim = V2Multiply(ElementDim, V2Set1(0.5f));
    
    rectangle2 TextBounds = GetTextSize(RenderGroup, Grid->Scale, V2Set1(0.0f), Text);
    v2 TextDim = Rectangle2GetDim(TextBounds);
    v2 TextHalfDim = V2Multiply(TextDim, V2Set1(0.5f));
    
    v2 TextOffset = V2Add(V2Subtract(Element.Bounds.Min, TextHalfDim),ElementHalfDim);
    PushRenderCommandText(RenderGroup, Grid->Scale, TextOffset, TextColor, Text);
    
    if(BackgroundColor.A > 0.0f)
    {
        PushRenderCommandRectangle(RenderGroup, BackgroundColor, Element.Bounds, 0.25f);
    }
    if(BorderColor.A > 0.0f)
    {
        PushRenderCommandRectangleOutline(RenderGroup, BorderThickness, BorderColor, Element.Bounds, 1.0f);
    }
    
    return Element;
}

// TODO(kstandbridge): Button format
// TODO(kstandbridge): Immedate Button, then turn this into Button_
// TODO(kstandbridge): BooleanButton, automatically toggle a boolean, think checkbox
// TODO(kstandbridge): SetU32Button, automatically set an U32, think radio to choose an enum
internal b32
Button(ui_grid *Grid, render_group *RenderGroup, u16 ColumnIndex, u16 RowIndex, ui_interaction_id Id, void *Target, string Text)
{
    ui_state *UIState = Grid->UIState;
    // TODO(kstandbridge): This shouldn't have a perm arena, later we will not load textures in the push draw routine
    b32 Result = false;
    
    ui_interaction Interaction;
    Interaction.Id = Id;
    Interaction.Type = Interaction_ImmediateButton;
    Interaction.Target = Target;
    
    element_colors Colors = 
        ElementColors(RGBColor(0, 0, 0, 255), RGBColor(0, 120, 215, 255), RGBColor(0, 84, 153, 255), RGBColor(0, 0, 0, 255),
                      RGBColor(225, 225, 225, 255), RGBColor(229, 241, 251, 255), RGBColor(204, 228, 247, 255), RGBColor(225, 225, 225, 255),
                      RGBColor(173, 173, 173, 255), RGBColor(0, 120, 215, 255), RGBColor(0, 84, 153, 255), RGBColor(0, 120, 215, 255));
    ui_element Element = DrawTextElement_(Grid, RenderGroup, ColumnIndex, RowIndex, Interaction, Text, Colors);
    
    if(InteractionsAreEqual(Interaction, UIState->ToExecute) &&
       Element.IsHot)
    {
        Result = true;
    }
    
    return Result;
}

inline ui_grid
BeginGrid(ui_state *UIState, memory_arena *TempArena, rectangle2 Bounds, u16 Rows, u16 Columns)
{
    Assert(Rows > 0);
    Assert(Columns > 0);
    
    ui_grid Result;
    Result.UIState = UIState;
    Result.Rows = Rows;
    Result.Columns = Columns;
    Result.SizeInitialized = false;
    
    Result.Scale = 1.0f;
    Result.SpacingX = 4.0f;
    Result.SpacingY = 4.0f;
    Result.FirstRow = 0;
    
    f32 DefaultRowHeight = UIState->LineAdvance + Result.SpacingY*4.0f;
    
    ui_row *CurrentRow = Result.FirstRow;
    for(u16 Row = 0;
        Row < Rows;
        ++Row)
    {
        if(CurrentRow == 0)
        {
            CurrentRow = PushStruct(TempArena, ui_row);
            Result.FirstRow = CurrentRow;
        }
        else
        {
            CurrentRow->Next = PushStruct(TempArena, ui_row);
            CurrentRow = CurrentRow->Next;
        }
        CurrentRow->Height = DefaultRowHeight;
        CurrentRow->FirstColumn = 0;
        CurrentRow->Next = 0;
        
        ui_column *CurrentColumn = CurrentRow->FirstColumn;
        for(u16 Column = 0;
            Column < Columns;
            ++Column)
        {
            if(CurrentColumn == 0)
            {
                CurrentColumn = PushStruct(TempArena, ui_column);
                CurrentRow->FirstColumn = CurrentColumn;
            }
            else
            {
                CurrentColumn->Next = PushStruct(TempArena, ui_column);
                CurrentColumn = CurrentColumn->Next;
            }
            CurrentColumn->Width = 0;
            CurrentColumn->Next = 0;
        }
    }
    
    Result.Bounds = Bounds;
    Result.Rows = Rows;
    Result.Columns = Columns;
    
    return Result;
}

inline void
EndGrid(ui_grid *Grid)
{
    Assert(Grid->SizeInitialized);
}

#define SIZE_AUTO 0.0f
inline void
SetColumnWidth(ui_grid *Grid, u16 RowIndex, u16 ColumnIndex, f32 Width)
{
    Assert(!Grid->SizeInitialized);
    Assert(RowIndex <= Grid->Rows);
    Assert(ColumnIndex <= Grid->Columns);
    
    ui_row *Row = Grid->FirstRow;
    Assert(Row);
    for(;
        RowIndex;
        --RowIndex)
    {
        Row = Row->Next;
    } 
    ui_column *Column = Row->FirstColumn;
    Assert(Column);
    for(;
        ColumnIndex;
        --ColumnIndex)
    {
        Column = Column->Next;
    }
    Column->Width = Width;
}

inline void
SetAllColumnWidths(ui_grid *Grid, u16 ColumnIndex, f32 Width)
{
    Assert(!Grid->SizeInitialized);
    Assert(ColumnIndex <= Grid->Columns);
    
    for(ui_row *Row = Grid->FirstRow;
        Row;
        Row = Row->Next)
    {
        ui_column *Column = Row->FirstColumn;
        Assert(Column);
        for(;
            ColumnIndex;
            --ColumnIndex)
        {
            Column = Column->Next;
        }
        Column->Width = Width;
    }
}

inline void
SetRowHeight(ui_grid *Grid, u16 RowIndex, f32 Height)
{
    Assert(!Grid->SizeInitialized);
    Assert(RowIndex <= Grid->Rows);
    
    ui_row *Row = Grid->FirstRow;
    Assert(Row);
    for(;
        RowIndex;
        --RowIndex)
    {
        Row = Row->Next;
    } 
    Row->Height = Height;
}

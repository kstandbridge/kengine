inline ui_value *
GetUIValue(ui_state *UIState, char *GUIDInit)
{
    memory_arena *Arena = UIState->PermArena;
    string GUID = String_(GetNullTerminiatedStringLength(GUIDInit), (u8 *)GUIDInit);
    u32 HashValue = StringToHashValue(GUID);
    u32 Index = HashValue % ArrayCount(UIState->ValueHash);
    
    ui_value *Result = UIState->ValueHash[Index];
    if(Result == 0)
    {
        Result = PushStruct(Arena, ui_value);
        UIState->ValueHash[Index] = Result;
        
        Result->GUID = GUID;
        Result->NextInHash = 0;
    }
    else
    {
        for(;;)
        {
            if(StringsAreEqual(GUID, Result->GUID))
            {
                break;
            }
            else
            {
                // NOTE(kstandbridge): Hash conflict
                if(Result->NextInHash)
                {
                    Result = Result->NextInHash;
                }
                else
                {
                    Result->NextInHash = PushStruct(Arena, ui_value);
                    Result = Result->NextInHash;
                    Result->GUID = GUID;
                    Result->NextInHash = 0;
                    break;
                }
            }
        }
    }
    
    return Result;
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
            AtY -= Platform->GetVerticleAdvance()*Scale;
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
                    Glyphs[CodePoint] = Platform->GetGlyphForCodePoint(Arena, CodePoint);
                }
                
                loaded_glyph *Glyph = Glyphs + CodePoint;
                Assert(Glyph->Bitmap.Memory);
                
                v2 Offset = V2(AtX, AtY);
                f32 BitmapScale = Scale*(f32)Glyph->Bitmap.Height;
                
                if(Op == TextOpText_Draw)
                {
                    PushRenderCommandBitmap(RenderGroup, &Glyph->Bitmap, BitmapScale, Offset, Color, 200.0f);
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
                
                f32 AdvanceX = (Scale*Platform->GetHorizontalAdvance(PrevCodePoint, CodePoint)) + Glyph->KerningChange;
                loaded_glyph *PreviousGlyph = Glyphs + PrevCodePoint;
                AdvanceX += PreviousGlyph->KerningChange;
                
                AtX += AdvanceX;
            }
            else
            {
                
                f32 AdvanceX = Platform->GetHorizontalAdvance(PrevCodePoint, CodePoint);
                if(AdvanceX)
                {
                    AtX += AdvanceX*Scale;
                }
                
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
            case Interaction_Draggable:
            {
                ((ui_value *)UIState->Interaction.Target)->V2_Value.X += UIState->dMouseP.X;
                ((ui_value *)UIState->Interaction.Target)->V2_Value.Y += UIState->dMouseP.Y;
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_DraggableX:
            {
                ((ui_value *)UIState->Interaction.Target)->F32_Value += UIState->dMouseP.X;
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_DraggableY:
            {
                ((ui_value *)UIState->Interaction.Target)->F32_Value += UIState->dMouseP.Y;
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_ImmediateButton:
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
            
            case Interaction_SetU32:
            {
                UIState->ClickedId = UIState->Interaction.Id;
                if(MouseUp)
                {
                    *(u32 *)UIState->Interaction.Target = UIState->Interaction.U32_Value;
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_AddDeltaTimeF32:
            {
                UIState->ClickedId = UIState->Interaction.Id;
                ((ui_value *)UIState->Interaction.Target)->F32_Value += 100.0f * Input->dtForFrame;
                if(MouseUp)
                {
                    EndInteraction = true; 
                }
            } break;
            
            case Interaction_SubtractDeltaTimeF32:
            {
                UIState->ClickedId = UIState->Interaction.Id;
                ((ui_value *)UIState->Interaction.Target)->F32_Value -= 100.0f * Input->dtForFrame;
                if(MouseUp)
                {
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

inline void
PushRenderCommandText(render_group *RenderGroup, f32 Scale, v2 P, v4 Color, string Text)
{
    // TODO(kstandbridge): NAMING Internally this is calling PushRenderCommandBitmap for each of the glyphs,
    // nothing is being drawn now, its a push call so shouldn't be DrawText
    
    TextOp_(RenderGroup, TextOpText_Draw, Scale, P, Color, Text);
}

inline rectangle2
GetTextSize(render_group *RenderGroup, f32 Scale, string Text)
{
    rectangle2 Result = TextOp_(RenderGroup, TextOpText_Size, Scale, V2Set1(0.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), Text);
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

inline ui_element
BeginUIElementWithBounds(ui_grid *Grid, rectangle2 Bounds)
{
    ui_element Element;
    ZeroStruct(Element);
    
    Element.Grid = Grid;
    Element.Bounds = Bounds;
    
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
    
    rectangle2 TextBounds = GetTextSize(RenderGroup, Grid->Scale, Text);
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

typedef enum text_position
{
    TextPosition_TopLeft,
    TextPosition_TopMiddle,
    TextPosition_TopRight,
    TextPosition_MiddleLeft,
    TextPosition_MiddleMiddle,
    TextPosition_MiddleRight,
    TextPosition_BottomLeft,
    TextPosition_BottomMiddle,
    TextPosition_BottomRight
} text_position;

inline v2
GetTextOffset(render_group *RenderGroup, rectangle2 Bounds, f32 Scale, f32 LineAdvance, string Text, text_position TextPosition)
{
    BEGIN_BLOCK("GetTextOffset");
    
    v2 ElementDim = V2Subtract(Bounds.Max, Bounds.Min);
    v2 ElementHalfDim = V2Multiply(ElementDim, V2Set1(0.5f));
    rectangle2 TextBounds = GetTextSize(RenderGroup, Scale, Text);
    v2 TextDim = Rectangle2GetDim(TextBounds);
    v2 TextHalfDim = V2Multiply(TextDim, V2Set1(0.5f));
    
    s32 LineCount = 0;
    for(u32 CharIndex = 0;
        CharIndex < Text.Size;
        ++CharIndex)
    {
        if(Text.Data[CharIndex] == '\n')
        {
            ++LineCount;
        }
    }
    
    v2 Result = V2(0, 0);
    
    switch(TextPosition)
    {
        case TextPosition_TopLeft:
        {
            Result = V2(Bounds.Min.X, 
                        Bounds.Max.Y - LineAdvance);
        } break;
        
        case TextPosition_TopMiddle:
        {
            Result = V2(Bounds.Min.X + ElementHalfDim.X - TextHalfDim.X, 
                        Bounds.Max.Y - LineAdvance);
        } break;
        
        case TextPosition_TopRight:
        {
            Result = V2(Bounds.Max.X - TextDim.X, 
                        Bounds.Max.Y - LineAdvance);
        } break;
        
        case TextPosition_MiddleLeft:
        {
            Result = V2(Bounds.Min.X,  
                        Bounds.Max.Y - ElementHalfDim.Y +
                        ((LineCount == 0) ? -TextHalfDim.Y : LineAdvance*(0.5f*(LineCount))));
        } break;
        
        case TextPosition_MiddleMiddle:
        {
            Result = V2(Bounds.Min.X + ElementHalfDim.X - TextHalfDim.X,
                        Bounds.Max.Y - ElementHalfDim.Y + 
                        ((LineCount == 0) ? -TextHalfDim.Y : LineAdvance*(0.5f*(LineCount))));
        } break;
        
        case TextPosition_MiddleRight:
        {
            Result = V2(Bounds.Max.X - TextDim.X, 
                        Bounds.Max.Y - ElementHalfDim.Y + 
                        ((LineCount == 0) ? -TextHalfDim.Y : LineAdvance*(0.5f*(LineCount))));
        } break;
        
        case TextPosition_BottomLeft:
        {
            Result = V2(Bounds.Min.X, 
                        Bounds.Min.Y + LineAdvance*LineCount);
        } break;
        
        case TextPosition_BottomMiddle:
        {
            Result = V2(Bounds.Min.X + ElementHalfDim.X - TextHalfDim.X,
                        Bounds.Min.Y + LineAdvance*LineCount);
        } break;
        
        case TextPosition_BottomRight:
        {
            Result = V2(Bounds.Max.X - TextDim.X, 
                        Bounds.Min.Y + LineAdvance*LineCount);
        } break;
        
        
        InvalidDefaultCase;
    }
    
    END_BLOCK();
    
    return Result;
}

inline void
DrawScrollbutton(ui_grid *Grid, render_group *RenderGroup, ui_interaction Interaction, string ButtonText, rectangle2 ButtonBounds)
{
    ui_state *UIState = Grid->UIState;
    colors *Colors = RenderGroup->Colors;
    
    ui_element Element = BeginUIElementWithBounds(Grid, ButtonBounds);
    SetUIElementDefaultAction(&Element, Interaction);
    EndUIElement(&Element);
    
    v2 ButtonTextOffset = GetTextOffset(RenderGroup, ButtonBounds, Grid->Scale, UIState->LineAdvance, ButtonText, TextPosition_MiddleMiddle);
    
    if(Element.IsHot)
    {
        if(InteractionIdsAreEqual(UIState->ClickedId, Interaction.Id) && 
           UIState->MouseDown)
        {
            PushRenderCommandRectangle(RenderGroup, Colors->ScrollButtonClicked, ButtonBounds, 2.0f);
            PushRenderCommandText(RenderGroup, Grid->Scale, ButtonTextOffset, Colors->ScrollButtonClickedText, ButtonText);
        }
        else
        {
            PushRenderCommandRectangle(RenderGroup, Colors->ScrollButtonHot, ButtonBounds, 2.0f);
            PushRenderCommandText(RenderGroup, Grid->Scale, ButtonTextOffset, Colors->ScrollButtonHotText, ButtonText);
        }
    }
    else
    {
        PushRenderCommandRectangle(RenderGroup, Colors->ScrollButton, ButtonBounds, 2.0f);
        PushRenderCommandText(RenderGroup, Grid->Scale, ButtonTextOffset, Colors->ScrollButtonText, ButtonText);
    }
}

inline void
DrawScrollbar(ui_grid *Grid, render_group *RenderGroup, ui_interaction Interaction, rectangle2 ScrollBounds)
{
    ui_state *UIState = Grid->UIState;
    colors *Colors = RenderGroup->Colors;
    
    ui_element Element = BeginUIElementWithBounds(Grid, ScrollBounds);
    SetUIElementDefaultAction(&Element, Interaction);
    EndUIElement(&Element);
    
    if(Element.IsHot)
    {
        if(InteractionIdsAreEqual(UIState->ClickedId, Interaction.Id) && 
           UIState->MouseDown)
        {
            PushRenderCommandRectangle(RenderGroup, Colors->ScrollBarClicked, ScrollBounds, 3.0f);
        }
        else
        {
            PushRenderCommandRectangle(RenderGroup, Colors->ScrollBarHot, ScrollBounds, 3.0f);
        }
    }
    else
    {
        PushRenderCommandRectangle(RenderGroup, Colors->ScrollBar, ScrollBounds, 3.0f);
    }
}

inline void
BeginClipRect(render_group *RenderGroup, rectangle2 BoundsInit)
{
    Assert(RenderGroup->OldClipRectIndex == 0);
    RenderGroup->OldClipRectIndex = RenderGroup->CurrentClipRectIndex;
    rectangle2i Bounds = Rectangle2i(BoundsInit.Min.X, BoundsInit.Max.X, BoundsInit.Min.Y, BoundsInit.Max.Y);
    RenderGroup->CurrentClipRectIndex = PushRenderCommandClipRectangle(RenderGroup, Bounds);
}

inline void
EndClipRect(render_group *RenderGroup)
{
    RenderGroup->CurrentClipRectIndex = RenderGroup->OldClipRectIndex;
    RenderGroup->OldClipRectIndex = 0;
}

internal void
Label(ui_grid *Grid, render_group *RenderGroup, u16 ColumnIndex, u16 RowIndex, string Text, text_position TextPosition)
{
    BEGIN_BLOCK("Label");
    
    ui_state *UIState = Grid->UIState;
    
    ui_interaction Interaction;
    Interaction.Type = Interaction_NOP;
    
    ui_element Element = BeginUIElement(Grid, ColumnIndex, RowIndex);
    SetUIElementDefaultAction(&Element, Interaction);
    EndUIElement(&Element);
    
    DEBUG_IF(ShowUIElementOutlines)
    {
        PushRenderCommandRectangleOutline(RenderGroup, 1.0f, V4(0.0f, 0.0f, 0.0f, 0.5f), Element.Bounds, 3.0f);
    }
    
    BeginClipRect(RenderGroup, Element.Bounds);
    {
        v2 TextOffset = GetTextOffset(RenderGroup, Element.Bounds, Grid->Scale, UIState->LineAdvance, Text, TextPosition);
        PushRenderCommandText(RenderGroup, Grid->Scale, TextOffset, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
    }
    EndClipRect(RenderGroup);
    
    END_BLOCK();
}

internal void
Textbox(ui_grid *Grid, render_group *RenderGroup, app_input *Input, u16 ColumnIndex, u16 RowIndex, string Text)
{
    BEGIN_BLOCK("Textbox");
    
    ui_state *UIState = Grid->UIState;
    memory_arena *Arena = &UIState->TranArena;
    colors *Colors = RenderGroup->Colors;
    
    rectangle2 CellBounds = GetCellBounds(Grid, ColumnIndex, RowIndex);
    rectangle2 Bounds = Rectangle2(V2Add(CellBounds.Min, V2(Grid->SpacingX, Grid->SpacingY)),
                                   V2Subtract(CellBounds.Max, V2(Grid->SpacingX*2.0f, Grid->SpacingY*2.0f)));
    
    DEBUG_IF(ShowUIElementOutlines)
    {
        PushRenderCommandRectangleOutline(RenderGroup, 1.0f, V4(0.0f, 0.0f, 0.0f, 0.5f), Bounds, 3.0f);
    }
    
    rectangle2 TextSize = GetTextSize(RenderGroup, Grid->Scale, Text);
    v2 TextDim = Rectangle2GetDim(TextSize);
    v2 ElementDim = Rectangle2GetDim(Bounds);
    ui_value *ValueY = GetUIValue(UIState, GenerateGUID("TextY"));
    ValueY->F32_Value -= Input->MouseZ*0.1f;
    
    b32 DrawVerticleBar = TextDim.Y > ElementDim.Y;
    b32 DrawHorizontalBar = TextDim.X > ElementDim.X;
    
    if(DrawVerticleBar)
    {
        // NOTE(kstandbridge): Up button
        {        
            v2 ButtonMax = Bounds.Max;
            v2 ButtonMin = V2Subtract(ButtonMax, V2Set1(UIState->LineAdvance));;
            rectangle2 ButtonBounds = Rectangle2(ButtonMin, ButtonMax);
            
            ui_interaction Interaction;
            Interaction.Id = InteractionIdFromPtr(ValueY);
            Interaction.Target = (void *)ValueY;
            Interaction.Type = Interaction_SubtractDeltaTimeF32;
            
            DrawScrollbutton(Grid, RenderGroup, Interaction, String("\\02C4"), ButtonBounds);
            
        }
        
        // NOTE(kstandbridge): Down button
        {        
            v2 ButtonMax = V2(Bounds.Max.X, Bounds.Min.Y + UIState->LineAdvance);
            if(DrawHorizontalBar)
            {
                ButtonMax.Y += UIState->LineAdvance;
            }
            v2 ButtonMin = V2Subtract(ButtonMax, V2Set1(UIState->LineAdvance));;
            rectangle2 ButtonBounds = Rectangle2(ButtonMin, ButtonMax);
            
            ui_interaction Interaction;
            Interaction.Id = InteractionIdFromPtr(ValueY);
            Interaction.Target = (void *)ValueY;
            Interaction.Type = Interaction_AddDeltaTimeF32;
            
            DrawScrollbutton(Grid, RenderGroup, Interaction, String("\\02C5"), ButtonBounds);
        }
        Bounds.Max.X -= UIState->LineAdvance;
    }
    
    ui_value *ValueX = GetUIValue(UIState, GenerateGUID("TextX"));
    
    if(DrawHorizontalBar)
    {
        
        // NOTE(kstandbridge): Left Button
        {        
            v2 ButtonMax = V2Add(Bounds.Min, V2Set1(UIState->LineAdvance));
            v2 ButtonMin = Bounds.Min;
            rectangle2 ButtonBounds = Rectangle2(ButtonMin, ButtonMax);
            
            ui_interaction Interaction;
            Interaction.Id = InteractionIdFromPtr(ValueX);
            Interaction.Target = (void *)ValueX;
            Interaction.Type = Interaction_AddDeltaTimeF32;
            
            DrawScrollbutton(Grid, RenderGroup, Interaction, String("<"), ButtonBounds);
        }
        
        // NOTE(kstandbridge): Right Button
        {        
            v2 ButtonMax = V2(Bounds.Max.X, Bounds.Min.Y + UIState->LineAdvance);
            v2 ButtonMin = V2Subtract(ButtonMax, V2Set1(UIState->LineAdvance));
            rectangle2 ButtonBounds = Rectangle2(ButtonMin, ButtonMax);
            
            ui_interaction Interaction;
            Interaction.Id = InteractionIdFromPtr(ValueX);
            Interaction.Target = (void *)ValueX;
            Interaction.Type = Interaction_SubtractDeltaTimeF32;
            
            DrawScrollbutton(Grid, RenderGroup, Interaction, String(">"), ButtonBounds);
        }
        
        Bounds.Min.Y += UIState->LineAdvance;
    }
    
    PushRenderCommandRectangle(RenderGroup, Colors->TextboxBackground, Bounds, 2.0f);
    v2 BoundsDim = Rectangle2GetDim(Bounds);
    f32 MaxY;
    f32 MaxX;
    BeginClipRect(RenderGroup, Bounds);
    {
        v2 TextOffset = GetTextOffset(RenderGroup, Bounds, Grid->Scale, UIState->LineAdvance, Text, TextPosition_TopLeft);
        
        if(ValueX->F32_Value > 0.0f)
        {
            ValueX->F32_Value = 0.0f;
        }
        
        MaxX = BoundsDim.X - TextDim.X;
        if(ValueX->F32_Value < MaxX)
        {
            ValueX->F32_Value = MaxX;
        }
        
        if(ValueY->F32_Value < 0.0f)
        {
            ValueY->F32_Value = 0.0f;
        }
        
        MaxY = TextDim.Y - BoundsDim.Y + UIState->LineAdvance;
        if(ValueY->F32_Value > MaxY)
        {
            ValueY->F32_Value = MaxY;
        }
        
        if(TextDim.X > BoundsDim.X)
        {
            TextOffset.X += ValueX->F32_Value;
        }
        if(TextDim.Y > BoundsDim.Y)
        {
            TextOffset.Y += ValueY->F32_Value;
        }
        PushRenderCommandText(RenderGroup, Grid->Scale, TextOffset, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
    }
    EndClipRect(RenderGroup);
    
    if(DrawVerticleBar)
    {
        v2 ScrollMax = V2(Bounds.Max.X + UIState->LineAdvance, Bounds.Max.Y - UIState->LineAdvance);
        v2 ScrollMin = V2(ScrollMax.X - UIState->LineAdvance, Bounds.Min.Y + UIState->LineAdvance);
        
        rectangle2 ScrollBounds = Rectangle2(ScrollMin, ScrollMax);
        v2 ScrollDim = Rectangle2GetDim(ScrollBounds);
        ScrollMin.Y += 0.5f*ScrollDim.Y;
        
        f32 Percent = ValueY->F32_Value / MaxY;
        ScrollMax.Y -= 0.50f*ScrollDim.Y*Percent;
        ScrollMin.Y -= 0.50f*ScrollDim.Y*Percent;
        ScrollBounds = Rectangle2(ScrollMin, ScrollMax);
        
        ui_interaction Interaction;
        Interaction.Id = InteractionIdFromPtr(ValueY);
        Interaction.Target = (void *)ValueY;
        Interaction.Type = Interaction_DraggableY;
        
        DrawScrollbar(Grid, RenderGroup, Interaction, ScrollBounds);
    }
    if(DrawHorizontalBar)
    {
        v2 ScrollMax = V2(Bounds.Max.X - UIState->LineAdvance, Bounds.Min.Y);
        v2 ScrollMin = V2(Bounds.Min.X + UIState->LineAdvance, Bounds.Min.Y - UIState->LineAdvance);
        
        rectangle2 ScrollBounds = Rectangle2(ScrollMin, ScrollMax);
        v2 ScrollDim = Rectangle2GetDim(ScrollBounds);
        ScrollMax.X -= 0.5f*ScrollDim.X;
        
        f32 Percent = ValueX->F32_Value / MaxX;
        ScrollMax.X += 0.50f*ScrollDim.X*Percent;
        ScrollMin.X += 0.50f*ScrollDim.X*Percent;
        ScrollBounds = Rectangle2(ScrollMin, ScrollMax);
        
        ui_interaction Interaction;
        Interaction.Id = InteractionIdFromPtr(ValueX);
        Interaction.Target = (void *)ValueX;
        Interaction.Type = Interaction_DraggableX;
        
        DrawScrollbar(Grid, RenderGroup, Interaction, ScrollBounds);
    }
    
    END_BLOCK();
}

internal rectangle2
TabView(ui_grid *Grid, render_group *RenderGroup, u16 ColumnIndex, u16 RowIndex, u32 TabCount, string *TabHeaders, u32 *CurrentTab)
{
    BEGIN_BLOCK("TabView");
    
    ui_state *UIState = Grid->UIState;
    colors *Colors = RenderGroup->Colors;
    
    rectangle2 CellBounds = GetCellBounds(Grid, ColumnIndex, RowIndex);
    rectangle2 Result = Rectangle2(V2Add(CellBounds.Min, V2(Grid->SpacingX, Grid->SpacingY)),
                                   V2Subtract(CellBounds.Max, V2(Grid->SpacingX*2.0f, Grid->SpacingY*2.0f)));
    v2 At = V2(Result.Min.X, Result.Max.Y - UIState->LineAdvance);
    for(u32 TabIndex = 0;
        TabIndex < TabCount;
        ++TabIndex)
    {
        string TabHeader = TabHeaders[TabIndex];
        rectangle2 TextBounds = GetTextSize(RenderGroup, Grid->Scale, TabHeader);
        v2 TextDim = Rectangle2GetDim(TextBounds);
        
        rectangle2 TabHeaderBounds = Rectangle2(At, V2(At.X + TextDim.X + Grid->SpacingX*2.0f, Result.Max.Y));
        
        if(TabIndex == *CurrentTab)
        {
            PushRenderCommandRectangle(RenderGroup, Colors->TabBackground, TabHeaderBounds, 1.0f);
        }
        else
        {
            ui_interaction Interaction;
            Interaction.Id = InteractionIdFromU32s(TabIndex, TabCount);
            Interaction.Type = Interaction_SetU32;
            Interaction.Target = (void *)CurrentTab;
            Interaction.U32_Value = TabIndex;
            
            ui_element Element = BeginUIElementWithBounds(Grid, TabHeaderBounds);
            SetUIElementDefaultAction(&Element, Interaction);
            EndUIElement(&Element);
            
            v4 TabBackgroundColor = Colors->TabHeaderBackground;
            if(InteractionIsHot(UIState, Interaction))
            {
                TabBackgroundColor = Colors->TabHeaderHotBackground;;
            }
            PushRenderCommandRectangle(RenderGroup, TabBackgroundColor, TabHeaderBounds, 1.0f);
        }
        
        PushRenderCommandRectangleOutline(RenderGroup, 1.0f, RGBColor(217, 217, 217, 255), TabHeaderBounds, 3.0f);
        PushRenderCommandText(RenderGroup, Grid->Scale, V2Add(At, V2(Grid->SpacingX*2.0f, Grid->SpacingY)), V4(0.0f, 0.0f, 0.0f, 1.0f), TabHeader);
        At.X = TabHeaderBounds.Max.X;
    }
    
    Result.Max.Y -= UIState->LineAdvance;
    PushRenderCommandRectangleOutline(RenderGroup, 1.0f, Colors->TabBorder, Result, 2.0f); 
    PushRenderCommandRectangle(RenderGroup, Colors->TabBackground, Result, 1.0f); 
    
    END_BLOCK();
    
    return Result;
}

internal void
Checkbox(ui_grid *Grid, render_group *RenderGroup, u16 ColumnIndex, u16 RowIndex, b32 *Target, string Text)
{
    BEGIN_BLOCK("Checkbox");
    
    ui_state *UIState = Grid->UIState;
    
    ui_interaction Interaction;
    // TODO(kstandbridge): Create macro to pass this
    Interaction.Id = InteractionIdFromPtr(Target);
    Interaction.Type = Interaction_SetU32;
    Interaction.Target = (void *)Target;
    Interaction.U32_Value = !*Target;
    
    ui_element Element = BeginUIElement(Grid, ColumnIndex, RowIndex);
    SetUIElementDefaultAction(&Element, Interaction);
    EndUIElement(&Element);
    
    rectangle2 RemainingBounds = Element.Bounds;
    
    DEBUG_IF(ShowUIElementOutlines)
    {
        PushRenderCommandRectangleOutline(RenderGroup, 1.0f, V4(0.0f, 0.0f, 0.0f, 0.5f), RemainingBounds, 3.0f);
    }
    
    rectangle2 CheckBounds = Rectangle2(RemainingBounds.Min, V2(RemainingBounds.Min.X + UIState->LineAdvance, RemainingBounds.Max.Y));
    string CheckText = String("\\2713");
    rectangle2 CheckTextBounds = GetTextSize(RenderGroup, Grid->Scale, CheckText);
    v2 CheckOffset = GetTextOffset(RenderGroup, CheckBounds, Grid->Scale, UIState->LineAdvance, CheckText, TextPosition_MiddleMiddle);
    rectangle2 CheckBorder = Rectangle2(V2Subtract(CheckOffset, V2Set1(2.5f)), 
                                        V2Add(CheckOffset, V2Set1(15.0f)));
    v4 CheckColor = RGBColor(0, 0, 0, 255);
    v4 CheckBorderColor = RGBColor(51, 51, 51, 255);
    if(InteractionIsHot(UIState, Interaction))
    {
        CheckColor = RGBColor(0, 120, 215, 255);
        CheckBorderColor = RGBColor(0, 120, 215, 255);
    }
    if(InteractionIdsAreEqual(UIState->ClickedId, Interaction.Id) &&
       Element.IsHot)
    {
        CheckColor = RGBColor(0, 84, 153, 255);
        CheckBorderColor = RGBColor(0, 84, 153, 255);
        PushRenderCommandRectangle(RenderGroup, RGBColor(204, 228, 247, 255), CheckBounds, 1.0f);
    }
    PushRenderCommandRectangleOutline(RenderGroup, 1.0f, CheckBorderColor, CheckBounds, 2.0f);
    if(*Target)
    {
        PushRenderCommandText(RenderGroup, Grid->Scale, CheckOffset, CheckColor, CheckText);
    }
    
    RemainingBounds.Min.X = CheckBounds.Max.X + Grid->SpacingX;
    
    v2 TextOffset = GetTextOffset(RenderGroup, RemainingBounds, Grid->Scale, UIState->LineAdvance, Text, TextPosition_MiddleLeft);
    PushRenderCommandText(RenderGroup, Grid->Scale, TextOffset, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
    
    END_BLOCK();
}

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

inline ui_grid
BeginGrid(ui_state *UIState, memory_arena *TempArena, rectangle2 Bounds, u16 Rows, u16 Columns)
{
    BEGIN_BLOCK("BeginGrid");
    
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
        CurrentRow->Height = SIZE_AUTO;
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
            CurrentColumn->Width = SIZE_AUTO;
            CurrentColumn->Next = 0;
        }
    }
    
    Result.Bounds = Bounds;
    Result.Rows = Rows;
    Result.Columns = Columns;
    
    END_BLOCK();
    
    return Result;
}

inline void
EndGrid(ui_grid *Grid)
{
    BEGIN_BLOCK("EndGrid");
    
    Assert(Grid->SizeInitialized);
    
    END_BLOCK();
}

typedef enum split_panel_orientation
{
    SplitPanel_Verticle,
    SplitPanel_Horizontal,
} split_panel_orientation;


#define BeginSplitPanelGrid(UIState, RenderGroup, TempArena, Bounds, Input, Orientation) \
BeginSplitPanelGrid_(UIState, RenderGroup, TempArena, Bounds, Input, Orientation, GenerateGUID("SplitPanel"))

inline ui_grid
BeginSplitPanelGrid_(ui_state *UIState, render_group *RenderGroup, memory_arena *TempArena, rectangle2 Bounds,
                     app_input *Input, split_panel_orientation Orientation, char *GUID)
{
    BEGIN_BLOCK("BeginSplitPanelGrid");
    
    ui_grid Result;
    v2 SplitterMax;
    v2 SplitterMin;
    ui_interaction Interaction;
    
    ui_value *Value = GetUIValue(UIState, GUID);
    
    
    Interaction.Id = InteractionIdFromPtr(Value);
    Interaction.Target = (void *)Value;
    
    if(Orientation == SplitPanel_Verticle)
    {
        Result = BeginGrid(UIState, TempArena, Bounds, 2, 1);
        
        if(Value->F32_Value == 0.0f)
        {
            Value->F32_Value = 0.5f*(Bounds.Max.Y - Bounds.Min.Y);
        }
        f32 MinSize = 0.25f*(Bounds.Max.Y - Bounds.Min.Y);
        f32 MaxSize = 0.75f*(Bounds.Max.Y - Bounds.Min.Y);
        if(Value->F32_Value < MinSize)
        {
            Value->F32_Value = MinSize;
        }
        if(Value->F32_Value > MaxSize)
        {
            Value->F32_Value = MaxSize;
        }
        
        SetRowHeight(&Result, 0, Value->F32_Value);
        SetRowHeight(&Result, 1, SIZE_AUTO);
        
        
        
        Interaction.Type = Interaction_DraggableY;
        
        SplitterMax = V2(Bounds.Max.X, Bounds.Max.Y - Value->F32_Value - Result.SpacingY*2.0f);
        SplitterMin = V2(Bounds.Min.X, SplitterMax.Y + Result.SpacingY*3.0f); 
        if(SplitterMin.Y > SplitterMax.Y)
        {
            f32 Temp = SplitterMax.Y;
            SplitterMax.Y = SplitterMin.Y;
            SplitterMin.Y = Temp;
        }
    }
    else
    {
        Assert(Orientation == SplitPanel_Horizontal);
        Result = BeginGrid(UIState, TempArena, Bounds, 1, 2);
        
        if(Value->F32_Value == 0.0f)
        {
            Value->F32_Value = 0.5f*(Bounds.Max.X - Bounds.Min.X);
        }
        f32 MinSize = 0.25f*(Bounds.Max.X - Bounds.Min.X);
        f32 MaxSize = 0.75f*(Bounds.Max.X - Bounds.Min.X);
        if(Value->F32_Value < MinSize)
        {
            Value->F32_Value = MinSize;
        }
        if(Value->F32_Value > MaxSize)
        {
            Value->F32_Value = MaxSize;
        }
        SetAllColumnWidths(&Result, 0, SIZE_AUTO);
        SetAllColumnWidths(&Result, 1, Value->F32_Value);
        
        Interaction.Type = Interaction_DraggableX;
        
        SplitterMax = V2(Bounds.Max.X - Value->F32_Value - Result.SpacingX*2.0f, Bounds.Max.Y);
        SplitterMin = V2(SplitterMax.X + Result.SpacingY*3.0f, Bounds.Min.Y); 
        if(SplitterMin.X > SplitterMax.X)
        {
            f32 Temp = SplitterMax.X;
            SplitterMax.X = SplitterMin.X;
            SplitterMin.X = Temp;
        }
    }
    
    rectangle2 SplitterBounds = Rectangle2(SplitterMin, SplitterMax);
    
    ui_element Element = BeginUIElementWithBounds(&Result, SplitterBounds);
    SetUIElementDefaultAction(&Element, Interaction);
    EndUIElement(&Element);
    
    if(Element.IsHot)
    {
        PushRenderCommandRectangle(RenderGroup, V4(1, 0, 0, 1), SplitterBounds, 2.0f);
    }
    else
    {
        PushRenderCommandRectangle(RenderGroup, V4(1, 0.5f, 0, 1), SplitterBounds, 2.0f);
    }
    
    END_BLOCK();
    
    return Result;
}
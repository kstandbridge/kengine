
typedef enum text_op
{
    TextOp_Draw,
    TextOp_Size,
} text_op;

internal rectangle2
TextOp_(ui_state *UiState, rectangle2 Bounds, f32 Depth, f32 Scale, v4 Color, string Text, text_op Op)
{
    Scale = 0.5f;
    rectangle2 Result = Rectangle2(Bounds.Min, Bounds.Min);
    v2 Offset = Bounds.Min;
    
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    
    f32 AtX = Offset.X;
    f32 AtY = Offset.Y + UiState->FontScale*UiState->FontAscent*Scale;
    
    for(umm Index = 0;
        Index < Text.Size;
        ++Index)
    {
        u32 CodePoint = Text.Data[Index];
        
        if(CodePoint == '\n')
        {
            AtY += UiState->FontScale*UiState->FontAscent*Scale;
            AtX = Offset.X;
        }
        else
        {
            b32 WordWrap = false;
            if(IsWhitespace(CodePoint))
            {
                f32 WordWidth = 0;
                for(umm WordIndex = 1;
                    (Index + WordIndex) < Text.Size;
                    ++WordIndex)
                {
                    u32 WordCodePoint = Text.Data[Index + WordIndex];
                    if(IsWhitespace(WordCodePoint))
                    {
                        break;
                    }
                    glyph_info *Info = UiState->GlyphInfos + WordCodePoint;
                    Assert(Info->CodePoint == WordCodePoint);
                    
                    WordWidth += UiState->FontScale*Info->AdvanceWidth*Scale;
                    
                    if(Index < Text.Size)
                    {
                        s32 Kerning = stbtt_GetCodepointKernAdvance(&UiState->FontInfo, CodePoint, Text.Data[Index + 1]);
                        if(Kerning > 0)
                        {
                            WordWidth += UiState->FontScale*Kerning*Scale;
                        }
                    }
                    
                }
                WordWrap = (AtX + WordWidth) > Bounds.Max.X;
            }
            
            if(WordWrap)
            {
                AtY += UiState->FontScale*UiState->FontAscent*Scale;
                AtX = Offset.X;
            }
            else
            {            
                glyph_info *Info = UiState->GlyphInfos + CodePoint;
                Assert(Info->CodePoint == CodePoint);
                
                v2 Size = V2(Scale*Info->Width, Scale*Info->Height);
                
                v2 GlyphOffset = V2(AtX, AtY + Info->YOffset*Scale);
                if(Op == TextOp_Draw)
                {
                    PushRenderCommandGlyph(RenderGroup, GlyphOffset, Depth, Size, Color, Info->UV);
                }
                else
                {
                    Assert(Op == TextOp_Size);
                    Result = Rectangle2Union(Result, Rectangle2(GlyphOffset, V2Add(GlyphOffset, Size)));
                }
                
                AtX += UiState->FontScale*Info->AdvanceWidth*Scale;
                
                if(Index < Text.Size)
                {
                    s32 Kerning = stbtt_GetCodepointKernAdvance(&UiState->FontInfo, CodePoint, Text.Data[Index + 1]);
                    AtX += UiState->FontScale*Kerning*Scale;
                }
            }
            
        }
    }
    
    return Result;
}

internal void
DrawTextAt(ui_state *UiState, rectangle2 Bounds, f32 Depth, f32 Scale, v4 Color, string Text)
{
    TextOp_(UiState, Bounds, Depth, Scale, Color, Text, TextOp_Draw);
}

internal rectangle2
GetTextBounds(ui_state *UiState, rectangle2 Bounds, f32 Depth, f32 Scale, v4 Color, string Text)
{
    rectangle2 Result = TextOp_(UiState, Bounds, Depth, Scale, Color, Text, TextOp_Size);
    return Result;
}

internal void
BeginGrid(ui_state *UiState, rectangle2 Bounds, u32 Columns, u32 Rows)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    
    ui_grid *Grid = PushStruct(Arena, ui_grid);
    Grid->Prev = Frame->CurrentGrid;
    Frame->CurrentGrid = Grid;
    
    Grid->Bounds = Bounds;
    Grid->Columns = Columns;
    Grid->Rows = Rows;
    
    Grid->GridSizeCalculated = false;
    Grid->ColumnWidths = PushArray(Arena, Columns, f32);
    for(u32 ColumnIndex = 0;
        ColumnIndex < Columns;
        ++ColumnIndex)
    {
        Grid->ColumnWidths[ColumnIndex] = 0.0f;
    }
    
    Grid->DefaultRowHeight = UiState->FontScale*UiState->FontAscent*2.0f;
    Grid->RowHeights = PushArray(Arena, Rows, f32);
    for(u32 RowIndex = 0;
        RowIndex < Rows;
        ++RowIndex)
    {
        Grid->RowHeights[RowIndex] = 0.0f;
    }
}

internal void
EndGrid(ui_state *UiState)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    Frame->CurrentGrid = Frame->CurrentGrid->Prev;
}

internal void
GridSetRowHeight(ui_state *UiState, u32 Row, f32 Height)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    Assert(Row <= Grid->Rows);
    
    // NOTE(kstandbridge): Sizes must be set before a control is drawn
    Assert(!Grid->GridSizeCalculated);
    
    Grid->RowHeights[Row] = Height;
}

internal void
GridSetColumnWidth(ui_state *UiState, u32 Column, f32 Width)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    Assert(Column <= Grid->Columns);
    
    // NOTE(kstandbridge): Sizes must be set before a control is drawn
    Assert(!Grid->GridSizeCalculated);
    
    Grid->ColumnWidths[Column] = Width;
}

internal rectangle2
GridGetCellBounds(ui_state *UiState, u32 Column, u32 Row)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    if(!Grid->GridSizeCalculated)
    {
        
        v2 Dim = Rectangle2GetDim(Grid->Bounds);
#if 0        
        f32 ColumnWidth = Dim.X / Grid->Columns;
        for(u32 ColumnIndex = 0;
            ColumnIndex < Grid->Columns;
            ++ColumnIndex)
        {
            Grid->ColumnWidths[ColumnIndex] = ColumnWidth;
        }
        
        f32 RowHeight = Dim.Y / Grid->Rows;
        for(u32 RowIndex = 0;
            RowIndex < Grid->Rows;
            ++RowIndex)
        {
            Grid->RowHeights[RowIndex] = RowHeight;
        }
#else
        u32 ColumnsFilled = 0;
        f32 WidthUsed = 0.0f;
        for(u32 ColumnIndex = 0;
            ColumnIndex < Grid->Columns;
            ++ColumnIndex)
        {
            if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
            {
                ++ColumnsFilled;
            }
            else
            {
                WidthUsed += Grid->ColumnWidths[ColumnIndex];
            }
        }
        if(ColumnsFilled > 0)
        {
            f32 WidthRemaining = Dim.X - WidthUsed;
            f32 ColumnWidth = WidthRemaining / ColumnsFilled;
            
            for(u32 ColumnIndex = 0;
                ColumnIndex < Grid->Columns;
                ++ColumnIndex)
            {
                if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
                {
                    Grid->ColumnWidths[ColumnIndex] = ColumnWidth;
                }
            }
        }
        
        u32 RowsFilled = 0;
        f32 HeightUsed = 0.0f;
        for(u32 RowIndex = 0;
            RowIndex < Grid->Rows;
            ++RowIndex)
        {
            if(Grid->RowHeights[RowIndex] == 0.0f)
            {
                ++RowsFilled;
            }
            else
            {
                HeightUsed += Grid->RowHeights[RowIndex];
            }
        }
        if(RowsFilled > 0)
        {
            f32 HeightRemaining = Dim.Y - HeightUsed;
            f32 RowHeight = HeightRemaining / RowsFilled;
            
            for(u32 RowIndex = 0;
                RowIndex < Grid->Rows;
                ++RowIndex)
            {
                if(Grid->RowHeights[RowIndex] == 0.0f)
                {
                    Grid->RowHeights[RowIndex] = RowHeight;
                }
            }
        }
#endif
        
        Grid->GridSizeCalculated = true;
    }
    
    v2 Min = Grid->Bounds.Min;
    for(u32 ColumnIndex = 0;
        ColumnIndex < Column;
        ++ColumnIndex)
    {
        Min.X += Grid->ColumnWidths[ColumnIndex];
    }
    
    for(u32 RowIndex = 0;
        RowIndex < Row;
        ++RowIndex)
    {
        Min.Y += Grid->RowHeights[RowIndex];
    }
    
    v2 Max = V2Add(Min, V2(Grid->ColumnWidths[Column], Grid->RowHeights[Row]));
    
    rectangle2 Result = Rectangle2(Min, Max);
    return Result;
}

typedef enum ui_interaction_state
{
    UIInteractionState_None,
    UIInteractionState_HotClicked,
    UIInteractionState_Hot,
    UIInteractionState_Selected
} ui_interaction_state;

internal ui_interaction_state
AddUIInteraction(ui_state *UiState, rectangle2 Bounds, ui_interaction Interaction)
{
    ui_interaction_state Result = UIInteractionState_None;
    
    Assert(Interaction.Type);
    
    if(Rectangle2IsIn(Bounds, UiState->MouseP))
    {
        UiState->NextHotInteraction = Interaction;
    }
    
    if(InteractionsAreEqual(Interaction, UiState->Interaction) &&
       InteractionsAreEqual(Interaction, UiState->NextHotInteraction))
    {
        Result = UIInteractionState_HotClicked;
    }
    else if(InteractionsAreEqual(Interaction, UiState->HotInteraction))
    {
        Result = UIInteractionState_Hot;
    }
    else if(InteractionsAreEqual(Interaction, UiState->SelectedInteration))
    {
        Result = UIInteractionState_Selected;
    }
    else
    {
        Result = UIInteractionState_None;
    }
    
    return Result;
}

#define Button(UiState, Column, Row, Enabled, Text) Button_(UiState, Column, Row, Enabled, Text, GenerateUIId(0))
internal b32
Button_(ui_state *UiState, u32 Column, u32 Row, b32 Enabled, string Text, ui_id Id)
{
    b32 Result = false;
    
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    rectangle2 Bounds = GridGetCellBounds(UiState, Column, Row);
    Bounds.Max = V2Subtract(Bounds.Max, V2Set1(GlobalMargin));
    
    v2 Dim = Rectangle2GetDim(Bounds);
    
    v4 TextColor = GlobalFormTextColor;
    
    if(Enabled)
    {    
        ui_interaction Interaction =
        {
            .Id = Id,
            .Type = UI_Interaction_ImmediateButton,
            .Target = 0
        };
        
        Result = InteractionsAreEqual(Interaction, UiState->ToExecute);
        
        
        ui_interaction_state InteractionState = AddUIInteraction(UiState, Bounds, Interaction);
        switch(InteractionState)
        {
            case UIInteractionState_HotClicked:
            {
                PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonClickedBackColor);
                PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonClickedBorderColor, 1);
            } break;
            
            case UIInteractionState_Hot:
            {
                PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonHotBackColor);
                PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonHotBorderColor, 1);
            } break;
            
            case UIInteractionState_Selected:
            {
                PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonBackColor);
                PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonHotBorderColor, 2);
            } break;
            
            default:
            {
                PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonBackColor);
                PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonBorderColor, 1);
            } break;
        }
    }
    else
    {
        TextColor = GlobalFormDisabledTextColor;
        PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonDisabledBackColor);
        PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonDisabledBorderColor, 1);
    }
    
    rectangle2 TextBounds = GetTextBounds(UiState, Bounds, 2.0f, 1.0f, TextColor, Text);
    
    v2 TextDim = Rectangle2GetDim(TextBounds);
    
    v2 TextOffset = V2Add(Bounds.Min,
                          V2Subtract(V2Multiply(Dim, V2Set1(0.5f)),
                                     V2Multiply(TextDim, V2Set1(0.5f))));
    
    DrawTextAt(UiState, Rectangle2(TextOffset, V2Add(TextOffset, TextDim))
               , 2.0f, 1.0f, TextColor, Text);
    
    return Result;
}

#define Label(UiState, Column, Row, Text) Label_(UiState, Column, Row, GenerateUIId(0), Text);
internal void
Label_(ui_state *UiState, u32 Column, u32 Row, ui_id Id, string Text)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    rectangle2 Bounds = GridGetCellBounds(UiState, Column, Row);
    
    DrawTextAt(UiState, Bounds, 2.0f, 1.0f, GlobalFormTextColor, Text);
    
}

internal void
Slider(ui_state *UiState, u32 Column, u32 Row, ui_id Id, f32 *Target, string Text)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    rectangle2 Bounds = GridGetCellBounds(UiState, Column, Row);
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_Draggable,
        .Target = Target
    };
    
    v4 ButtonColor = V4(1.0f, 0.5f, 0.0f, 1.0f);
    if(Rectangle2IsIn(Bounds, UiState->MouseP))
    {
        UiState->NextHotInteraction = Interaction;
        ButtonColor = V4(0.5f, 1.0f, 0.0f, 1.0f);
    }
    
    if(InteractionsAreEqual(Interaction, UiState->Interaction))
    {
        *Target += 0.01f*UiState->dMouseP.X;
    }
    
    PushRenderCommandRect(RenderGroup, Bounds, 1.0f, ButtonColor);
    
    DrawTextAt(UiState, Bounds, 2.0f, 1.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
}

internal rectangle2
GroupControl(ui_state *UiState, u32 Column, u32 Row, string Header)
{
    rectangle2 Result = GridGetCellBounds(UiState, Column, Row);
    
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    
    v2 TextP = V2(Result.Min.X + GlobalMargin, Result.Min.Y);
    rectangle2 TextBounds = GetTextBounds(UiState, Rectangle2(TextP, Result.Max),
                                          2.0f, 1.0f, GlobalFormTextColor, Header);
    TextBounds.Min.X -= GlobalMargin*0.25f;
    TextBounds.Max.X += GlobalMargin*0.25f;
    PushRenderCommandRect(RenderGroup, TextBounds, 2.0f, GlobalFormColor);
    DrawTextAt(UiState, Rectangle2(TextP, Result.Max), 
               3.0f, 1.0f, GlobalFormTextColor,Header);
    Result.Min.Y += GlobalMargin;
    Result.Max = V2Subtract(Result.Max, V2Set1(GlobalMargin));
    PushRenderCommandRectOutline(RenderGroup, Result, 1.0f,GlobalGroupBorder, 1.0f);
    Result.Min.Y += GlobalMargin;
    Result = Rectangle2AddRadiusTo(Result, -GlobalMargin);
    
    return Result;
}

internal rectangle2
TabControl(ui_state *UiState, u32 Column, u32 Row, u32 *SelectedIndex, string *Labels, u32 LabelCount)
{
    rectangle2 Result = GridGetCellBounds(UiState, Column, Row);
    
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    
    BeginGrid(UiState, Result, 1, 2);
    {
        GridSetRowHeight(UiState, 0, 20.0f);
        
        // NOTE(kstandbridge): Tab controls
        {
            rectangle2 Bounds = GridGetCellBounds(UiState, 0, 0);
            v2 At = Bounds.Min;
            
            for(u32 LabelIndex = 0;
                LabelIndex < LabelCount;
                ++LabelIndex)
            {
                string Text = Labels[LabelIndex];
                rectangle2 TextBounds = GetTextBounds(UiState, Rectangle2(At, Bounds.Max), 
                                                      1.0f, 1.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
                TextBounds.Max.X += GlobalMargin;
                if(LabelIndex == *SelectedIndex)
                {
                    rectangle2 ButtonBounds = Rectangle2(TextBounds.Min, V2(TextBounds.Max.X, Bounds.Max.Y));
                    PushRenderCommandRect(RenderGroup, ButtonBounds, 1.0f, 
                                          GlobalTabButtonSelected);
                    PushRenderCommandRect(RenderGroup, Rectangle2(V2(ButtonBounds.Min.X, ButtonBounds.Max.Y - 1.0f), ButtonBounds.Max), 3.0f, GlobalFormColor);
                }
                else
                {
                    rectangle2 ButtonBounds = Rectangle2(TextBounds.Min, V2(TextBounds.Max.X, Bounds.Max.Y));
                    ButtonBounds.Min.Y += 4.0f;
                    
                    ui_interaction Interaction =
                    {
                        .Id = GenerateUIId(Labels + LabelIndex),
                        .Type = UI_Interaction_ImmediateButton,
                        .Target = 0
                    };
                    
                    ui_interaction_state InteractionState = AddUIInteraction(UiState, ButtonBounds, Interaction);
                    switch(InteractionState)
                    {
                        case UIInteractionState_HotClicked:
                        {
                            PushRenderCommandRect(RenderGroup, ButtonBounds, 1.0f, 
                                                  GlobalTabButtonClicked);
                        } break;
                        case UIInteractionState_Hot:
                        {
                            PushRenderCommandRect(RenderGroup, ButtonBounds, 1.0f, 
                                                  GlobalTabButtonHot);
                        } break;
                        
                        default:
                        {
                            PushRenderCommandRect(RenderGroup, ButtonBounds, 1.0f, 
                                                  GlobalTabButtonBackground);
                        } break;
                    }
                    
                    PushRenderCommandRectOutline(RenderGroup, ButtonBounds, 2.0f, GlobalTabButtonBorder, 1.0f);
                    
                    if(InteractionsAreEqual(Interaction, UiState->ToExecute))
                    {
                        *SelectedIndex = LabelIndex;
                    }
                }
                At.X += GlobalMargin/2.0f;
                DrawTextAt(UiState, Rectangle2(V2(At.X, At.Y + 2.0f), Bounds.Max), 
                           1.0f, 1.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
                
                At.X = TextBounds.Max.X;
            }
        }
        
        // NOTE(kstandbridge): Tab content
        {
            Result = GridGetCellBounds(UiState, 0, 1);
            Result.Min.Y -= 1.0f;
            Result.Max = V2Subtract(Result.Max, V2Set1(GlobalMargin));
            PushRenderCommandRect(RenderGroup, Result, 1.0f, GlobalFormColor);
            PushRenderCommandRectOutline(RenderGroup, Result, 2.0f, GlobalTabButtonBorder, 1.0f);
            Result.Min.Y += 1.0f;
            Result.Min = V2Add(Result.Min, V2Set1(GlobalMargin));
        }
        
    }
    EndGrid(UiState);
    
    
    return Result;
}
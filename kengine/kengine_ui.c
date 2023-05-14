

ui_interaction_state
AddUIInteraction(ui_state *State, rectangle2 Bounds, ui_interaction Interaction)
{
    ui_interaction_state Result = UIInteractionState_None;
    Assert(Interaction.Type);
    
    if((!State->MenuOpen) ||
       (State->MenuGrid == State->CurrentGrid))
    {    
        if(Rectangle2IsIn(Bounds, State->MouseP))
        {
            State->NextHotInteraction = Interaction;
        }
        
        if(InteractionsAreEqual(Interaction, State->Interaction) &&
           InteractionsAreEqual(Interaction, State->NextHotInteraction))
        {
            Result = UIInteractionState_HotClicked;
        }
        else if(InteractionsAreEqual(Interaction, State->HotInteraction))
        {
            Result = UIInteractionState_Hot;
        }
        else if(InteractionsAreEqual(Interaction, State->SelectedInteration))
        {
            Result = UIInteractionState_Selected;
        }
        else
        {
            Result = UIInteractionState_None;
        }
    }
    
    return Result;
}

void
InitUI(ui_state **State, app_assets *Assets)
{
    *State = BootstrapPushStruct(ui_state, Arena);
#if KENGINE_INTERNAL
    Assert(Assets->IsInitialized);
    (*State)->IsInitialized = true;
#endif
    (*State)->Assets = Assets;
    
#if 1
    (*State)->DefaultFont = String("C:\\Windows\\Fonts\\seguisym.ttf");
#else
    (*State)->DefaultFont = String("C:\\Windows\\Fonts\\segoeui.ttf");
#endif
    
}

void
BeginUI(ui_state *State, app_input *Input, render_group *RenderGroup)
{
#if KENGINE_INTERNAL
    Assert(State->IsInitialized);
#endif
    State->MemoryFlush = BeginTemporaryMemory(&State->Arena);
    State->LastMouseP = State->MouseP;
    State->MouseP = Input->MouseP;
    State->dMouseP = V2Subtract(State->MouseP, State->LastMouseP);
    
    State->MenuOpen = false;
    State->MenuGrid = 0;
    State->CurrentGrid = 0;
    State->Input = Input;
    State->RenderGroup = RenderGroup;
}

void
EndUI(ui_state *State)
{
    app_input *Input = State->Input;
    
    State->ToExecute = State->NextToExecute;
    ClearInteraction(&State->NextToExecute);
    
    u32 TransistionCount = Input->MouseButtons[MouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[MouseButton_Left].EndedDown;
    if(TransistionCount % 2)
    {
        MouseButton = !MouseButton;
    }
    
    for(u32 TransitionIndex = 0;
        TransitionIndex <= TransistionCount;
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
        
        switch(State->Interaction.Type)
        {
            case UI_Interaction_Boolean:
            {
                if(MouseUp)
                {
                    b32 *Current = (b32 *)State->Interaction.Target;
                    *Current = !*Current;
                    EndInteraction = true;
                }
            } break;
            
            case UI_Interaction_SetU32:
            {
                if(MouseUp)
                {
                    u32 *Current = (u32 *)State->Interaction.Target;
                    *Current = State->Interaction.U32;
                    EndInteraction = true;
                }
            } break;
            
            case UI_Interaction_ImmediateButton:
            {
                if(MouseUp)
                {
                    State->NextToExecute = State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            case UI_Interaction_None:
            {
                State->HotInteraction = State->NextHotInteraction;
                if(MouseDown)
                {
                    State->Interaction = State->HotInteraction;
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
        
        if(MouseDown)
        {
            ClearInteraction(&State->SelectedInteration);
        }
        
        if(MouseUp)
        {
            State->SelectedInteration = State->Interaction;
        }
        
        if(EndInteraction)
        {
            ClearInteraction(&State->Interaction);
        }
        
        MouseButton = !MouseButton;
    }
    
    ClearInteraction(&State->NextHotInteraction);
    
    EndTemporaryMemory(State->MemoryFlush);
    CheckArena(&State->Arena);
}

void
BeginGrid(ui_state *UIState, rectangle2 Bounds, u32 Columns, u32 Rows)
{
    memory_arena *Arena = UIState->MemoryFlush.Arena;
    
    ui_grid *Grid = PushStruct(Arena, ui_grid);
    Grid->Prev = UIState->CurrentGrid;
    UIState->CurrentGrid = Grid;
    
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
    
    Grid->RowHeights = PushArray(Arena, Rows, f32);
    for(u32 RowIndex = 0;
        RowIndex < Rows;
        ++RowIndex)
    {
        Grid->RowHeights[RowIndex] = 0.0f;
    }
}

void
EndGrid(ui_state *UIState)
{
    Assert(UIState->CurrentGrid);
    UIState->CurrentGrid = UIState->CurrentGrid->Prev;
}

rectangle2
GridGetCellBounds(ui_state *UIState, u32 Column, u32 Row, f32 Margin)
{
    Assert(UIState->CurrentGrid);
    ui_grid *Grid = UIState->CurrentGrid;
    
    if(!Grid->GridSizeCalculated)
    {
        v2 Dim = Rectangle2GetDim(Grid->Bounds);
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
    if(Margin > 0.0f)
    {
        if(Grid->Columns == 1)
        {
            Result.Min.X += Margin;
            Result.Max.X -= Margin;
        }
        else if(Column == 0)
        {
            Result.Min.X += Margin;
            Result.Max.X -= Margin*0.5f;
        }
        else if((Column + 1) == Grid->Columns)
        {
            Result.Min.X += Margin*0.5f;
            Result.Max.X -= Margin;
        }
        else
        {
            Result.Min.X += Margin*0.5f;
            Result.Max.X -= Margin*0.5f;
        }
        
        if(Grid->Rows == 1)
        {
            Result.Min.Y += Margin;
            Result.Max.Y -= Margin;
        }
        else if(Row == 0)
        {
            Result.Min.Y += Margin;
            Result.Max.Y -= Margin*0.5f;
        }
        
        else if((Row + 1) == Grid->Rows)
        {
            Result.Min.Y += Margin*0.5f;
            Result.Max.Y -= Margin;
        }
        else
        {
            Result.Min.Y += Margin;
            Result.Max.Y -= Margin*0.5f;
        }
    }
    return Result;
}

rectangle2
TextOp_(ui_state *UIState, rectangle2 Bounds, f32 Depth, f32 Scale, v4 Color, string Text, text_op Op)
{
    rectangle2 Result = Rectangle2(Bounds.Min, Bounds.Min);
    render_group *RenderGroup = UIState->RenderGroup;
    app_assets *Assets = UIState->Assets;
    v2 Offset = Bounds.Min;
    
    font_asset *Font = GetFontInfo(Assets, UIState->DefaultFont);
    if(Font)
    {    
        f32 AtX = Offset.X;
        f32 AtY = Offset.Y + Font->Scale*Font->Ascent*Scale;
        
        for(umm Index = 0;
            Index < Text.Size;
            ++Index)
        {
            u32 CodePoint = Text.Data[Index];
            
            if(CodePoint == '\n')
            {
                AtY += Font->Scale*Font->Ascent*Scale;
                AtX = Offset.X;
            }
            else
            {
                if(((Index + 5) << Text.Size) &&
                   (Text.Data[Index + 0] == '\\') &&
                   (Text.Data[Index + 1] == 'u') &&
                   (IsHex(Text.Data[Index + 2])) &&
                   (IsHex(Text.Data[Index + 3])) &&
                   (IsHex(Text.Data[Index + 4])) &&
                   (IsHex(Text.Data[Index + 5])))
                {
                    CodePoint = ((GetHex(Text.Data[Index + 2]) << 12) |
                                 (GetHex(Text.Data[Index + 3]) << 8) |
                                 (GetHex(Text.Data[Index + 4]) << 4) |
                                 (GetHex(Text.Data[Index + 5]) << 0));
                    Index += 5;
                    //__debugbreak();
                }
                
                glyph_sprite_sheet *Sprite = GetSpriteSheetForCodepoint(Assets, CodePoint);
                if(Sprite)
                {                
                    b32 WordWrap = false;
                    if(IsWhitespace((char)CodePoint))
                    {
                        f32 WordWidth = 0;
                        for(umm WordIndex = 1;
                            (Index + WordIndex) < Text.Size;
                            ++WordIndex)
                        {
                            u32 WordCodePoint = Text.Data[Index + WordIndex];
                            if(IsWhitespace((char)WordCodePoint))
                            {
                                break;
                            }
                            glyph_info *Info = Sprite->GlyphInfos + WordCodePoint;
                            // TODO(kstandbridge): Enable this
                            //Assert(Info->CodePoint == WordCodePoint);
                            
                            WordWidth += Font->Scale*Info->AdvanceWidth*Scale;
                            
                            if(Index < Text.Size)
                            {
                                s32 Kerning = stbtt_GetCodepointKernAdvance(&Font->Info, CodePoint, Text.Data[Index + 1]);
                                if(Kerning > 0)
                                {
                                    WordWidth += Font->Scale*Kerning*Scale;
                                }
                            }
                            
                        }
                        WordWrap = (AtX + WordWidth) > Bounds.Max.X;
                    }
                    
                    if(WordWrap)
                    {
                        AtY += Font->Scale*Font->Ascent*Scale;
                        AtX = Offset.X;
                    }
                    else
                    {            
                        u32 CodePointOffset = (CodePoint - Sprite->CodePointStartAt);
                        glyph_info *Info = Sprite->GlyphInfos + CodePointOffset;
                        // TODO(kstandbridge): Enable this
                        //Assert(Info->CodePoint == CodePoint);
                        
                        v2 Size = V2(Scale*Info->Width, Scale*Info->Height);
                        
                        v2 GlyphOffset = V2(AtX, AtY + Info->YOffset*Scale);
                        if(Op == TextOp_Draw)
                        {
                            PushRenderCommandGlyph(RenderGroup, GlyphOffset, Depth, Size, Color, Info->UV, Sprite->Handle);
                        }
                        else
                        {
                            Assert(Op == TextOp_Size);
                            Result = Rectangle2Union(Result, Rectangle2(GlyphOffset, V2Add(GlyphOffset, Size)));
                        }
                        
                        AtX += Font->Scale*Info->AdvanceWidth*Scale;
                        
                        if(Index < Text.Size)
                        {
                            s32 Kerning = stbtt_GetCodepointKernAdvance(&Font->Info, CodePoint, Text.Data[Index + 1]);
                            AtX += Font->Scale*Kerning*Scale;
                        }
                    }
                }
                
            }
        }
    }
    
    return Result;
}

inline b32
MenuButton_(ui_state *UIState, u32 Index, f32 Scale, string Text, ui_id Id)
{
    b32 Result = false;
    
    rectangle2 Bounds = GridGetCellBounds(UIState, 0, Index, 0);
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_ImmediateButton,
        .Target = 0
    };
    
    ui_interaction_state InteractionState = AddUIInteraction(UIState, Bounds, Interaction);
    
    if(InteractionState == UIInteractionState_Hot)
    {
        if(WasPressed(UIState->Input->MouseButtons[MouseButton_Left]))
        {
            Result = true;
        }
        
        PushRenderCommandRect(UIState->RenderGroup, Bounds, 10.0f, RGBv4(128, 128, 128));
    }
    
    DrawTextAt(UIState, Rectangle2(V2(Bounds.Min.X + 20.0f, Bounds.Min.Y), Bounds.Max), 
               10.0f, Scale, V4(0, 0, 0, 1), Text);
    
    return Result;
}

inline void
MenuCheck_(ui_state *UIState, u32 Index, f32 Scale, string Text, b32 *Target, ui_id Id)
{
    rectangle2 Bounds = GridGetCellBounds(UIState, 0, Index, 0);
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_Boolean,
        .Target = Target
    };
    
    ui_interaction_state InteractionState = AddUIInteraction(UIState, Bounds, Interaction);
    if(InteractionState == UIInteractionState_Hot)
    {
        PushRenderCommandRect(UIState->RenderGroup, Bounds, 10.0f, RGBv4(128, 128, 128));
    }
    
    if(*Target)
    {
        DrawTextAt(UIState, 
                   Rectangle2(Bounds.Min, V2(Bounds.Min.X + 20.0f, Bounds.Max.Y)),
                   10.0f, Scale, V4(0, 0, 0, 1), String("\\u2714"));
    }
    
    DrawTextAt(UIState, 
               Rectangle2(V2(Bounds.Min.X + 20.0f, Bounds.Min.Y), Bounds.Max), 
               10.0f, Scale, V4(0, 0, 0, 1), Text);
}

inline void
MenuOption_(ui_state *UIState, u32 Index, f32 Scale, string Text, u32 *Target, u32 Value, ui_id Id)
{
    rectangle2 Bounds = GridGetCellBounds(UIState, 0, Index, 0);
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_SetU32,
        .Target = Target,
        .U32 = Value
    };
    
    ui_interaction_state InteractionState = AddUIInteraction(UIState, Bounds, Interaction);
    if(InteractionState == UIInteractionState_Hot)
    {
        PushRenderCommandRect(UIState->RenderGroup, Bounds, 10.0f, RGBv4(128, 128, 128));
    }
    
    if(*Target == Value)
    {
        DrawTextAt(UIState, 
                   Rectangle2(Bounds.Min, V2(Bounds.Min.X + 20.0f, Bounds.Max.Y)),
                   10.0f, Scale, V4(0, 0, 0, 1), String("\\u2714"));
    }
    
    DrawTextAt(UIState, 
               Rectangle2(V2(Bounds.Min.X + 20.0f, Bounds.Min.Y), Bounds.Max), 
               10.0f, Scale, V4(0, 0, 0, 1), Text);
}

inline void
MenuSplit(ui_state *UIState, u32 Index, f32 Scale)
{
    rectangle2 Bounds = GridGetCellBounds(UIState, 0, Index, 0);
    
    f32 HalfHeight = (Bounds.Max.Y - Bounds.Min.Y)*0.5f;
    Bounds.Min.Y += HalfHeight - (1.0f*Scale);
    Bounds.Max.Y -= HalfHeight + (1.0f*Scale);
    
    PushRenderCommandAlternateRectOutline(UIState->RenderGroup, Bounds, 10.0f, Scale * 0.5f, RGBv4(128, 128, 128), RGBv4(255, 255, 255));
}

inline b32
Menu_(ui_state *UIState, rectangle2 Bounds, f32 Scale, string Text, ui_id Id)
{
    b32 Result = false;
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_ImmediateButton,
        .Target = 0
    };
    
    ui_interaction_state InteractionState = AddUIInteraction(UIState, Bounds, Interaction);
    
    if(InteractionsAreEqual(Interaction, UIState->SelectedInteration))
    {
        Result = true;
    }
    
    if(InteractionState == UIInteractionState_Hot)
    {
        PushRenderCommandRect(UIState->RenderGroup, Bounds, 1.0f, RGBv4(128, 128, 128));
    }
    DrawTextAt(UIState, Bounds, 1.0f, Scale, V4(0, 0, 0, 1), Text);
    
    return Result;
}

inline b32
BeginMenu_(ui_state *UIState, rectangle2 Bounds, f32 Scale, string Text, u32 Entries, f32 Width, ui_id Id)
{
    b32 Result = false;
    
    font_asset *Font = GetFontInfo(UIState->Assets, UIState->DefaultFont);
    if(Font)
    {        
        if(Menu_(UIState, Bounds, Scale, Text, Id))
        {
            Assert(!UIState->MenuOpen);
            
            Result = true;
            PushRenderCommandAlternateRectOutline(UIState->RenderGroup, Bounds, 1.0f, 1.0f,
                                                  RGBv4(128, 128, 128), RGBv4(255, 255, 255));
            
            v2 MenuBoundsMin = V2(Bounds.Min.X, Bounds.Max.Y);
            
            rectangle2 MenuBounds = Rectangle2(MenuBoundsMin, 
                                               V2Add(MenuBoundsMin,
                                                     V2(Width, (Entries + 1)*Font->Scale*Font->Ascent*Scale)));
            PushRenderCommandRect(UIState->RenderGroup, MenuBounds, 10.0f, RGBv4(192, 192, 192));
            PushRenderCommandAlternateRectOutline(UIState->RenderGroup, MenuBounds, 10.0f, 1.0f,
                                                  RGBv4(128, 128, 128), RGBv4(255, 255, 255));
            BeginGrid(UIState, MenuBounds, 1, Entries);
            UIState->MenuOpen = true;
            UIState->MenuGrid = UIState->CurrentGrid;
        }
    }
    
    return Result;
}

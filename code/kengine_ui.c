
internal ui_layout
BeginUIFrame(ui_state *UiState, memory_arena *Arena, app_input *Input, loaded_bitmap *DrawBuffer, f32 Padding, f32 Scale)
{
    ui_layout Result;
    ZeroStruct(Result);
    
    Result.State = UiState;
    Result.Arena = Arena;
    
    Result.Scale = Scale;
    Result.MinRowHeight = Platform.DEBUGGetLineAdvance()*Scale;
    Result.MouseP = V2(Input->MouseX, Input->MouseY);
    Result.dMouseP = V2Subtract(Result.MouseP, UiState->LastMouseP);
    Result.Padding = Padding;
    
    Result.DrawBuffer = DrawBuffer;
    
    Result.FillRows = 0;
    Result.UsedHeight = 0;
    Result.FloatingElements = 0;
    
    Result.LastRow = 0;
    Result.CurrentRow = Result.LastRow;
    Result.CurrentColumn = 0;
    Result.CurrentElement = 0;
    Result.CurrentDiv = &Result.MainDiv;
    Result.CurrentDiv->Parent = Result.CurrentDiv;
    
    UiState->ToExecute = UiState->NextToExecute;
    ClearInteraction(&UiState->NextToExecute);
    
    return Result;
}

internal void
HandleUIInteractionsInternal(ui_layout *Layout, app_input *Input)
{
    // NOTE(kstandbridge): Input text
    ui_interaction SelectedInteraction = Layout->State->SelectedInteraction;
    if(Input->Text[0] != '\0')
    {
        if(SelectedInteraction.Type == UiInteraction_TextInput)
        {
            editable_string *Str = SelectedInteraction.Str;
            
            char *At = Input->Text;
            while(*At != '\0')
            {
                if(Str->Length < Str->Size)
                {
                    ++Str->Length;
                    umm Index = Str->Length;
                    while(Index > Str->SelectionStart)
                    {
                        Str->Data[Index] = Str->Data[Index - 1];
                        --Index;
                    }
                    Str->Data[Str->SelectionStart++] = *At;
                    Str->SelectionEnd = Str->SelectionStart;
                }
                ++At;
            }
            
        }
    }
    
    // NOTE(kstandbridge): Keyboard buttons
    for(keyboard_button_type Type = 0;
        Type != KeyboardButton_Count;
        ++Type)
    {
        u32 TransitionCount = Input->KeyboardButtons[Type].HalfTransitionCount;
        b32 KeyboardButton = Input->KeyboardButtons[Type].EndedDown;
        if(TransitionCount % 2)
        {
            KeyboardButton = !KeyboardButton;
        }
        
        for(u32 TransitionIndex = 0;
            TransitionIndex <= TransitionCount;
            ++TransitionIndex)
        {
            b32 KeyboardDown = false;
            b32 KeyboardUp = false;
            if(TransitionIndex != 0)
            {
                KeyboardDown = KeyboardButton;
                KeyboardUp = !KeyboardButton;
            }
            
            if(KeyboardDown)
            {
                if(SelectedInteraction.Type == UiInteraction_TextInput)
                {
                    editable_string *Str = SelectedInteraction.Str;
                    switch(Type)
                    {
                        case KeyboardButton_Delete:
                        {
                            if(Str->SelectionStart < Str->Length)
                            {
                                Str->SelectionStart++;
                                Str->SelectionEnd = Str->SelectionStart;
                            }
                            else
                            {
                                break;
                            }
                        } // NOTE(kstandbridge): No break intentional
                        case KeyboardButton_Backspace:
                        {
                            if(Str->Length > 0)
                            {
                                umm StartMoveIndex = Str->SelectionStart--;
                                while(StartMoveIndex < Str->Length)
                                {
                                    Str->Data[StartMoveIndex - 1] = Str->Data[StartMoveIndex++];
                                }
                                Str->Data[--Str->Length] = '\0';
                                Str->SelectionEnd = Str->SelectionStart;
                            }
                        } break;
                        case KeyboardButton_Right:
                        {
                            if(Str->SelectionStart < Str->Length)
                            {
                                ++Str->SelectionStart;
                            }
                            if(!Input->ShiftDown)
                            {
                                Str->SelectionEnd = Str->SelectionStart;
                            }
                        } break;
                        case KeyboardButton_Left:
                        {
                            if(Str->SelectionStart > 0)
                            {
                                --Str->SelectionStart;
                            }
                            if(!Input->ShiftDown)
                            {
                                Str->SelectionEnd = Str->SelectionStart;
                            }
                        } break;
                    }
                }
            }
            KeyboardButton = !KeyboardButton;
        }
    }
    
    // NOTE(kstandbridge): Mouse buttons
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
        b32 MouseDown = false;
        b32 MouseUp = false;
        if(TransitionIndex != 0)
        {
            MouseDown = MouseButton;
            MouseUp = !MouseButton;
        }
        
        b32 EndInteraction = false;
        
        if(MouseDown)
        {
            Layout->State->SelectedInteraction = Layout->State->HotInteraction;
            Layout->State->ClickedInteraction = Layout->State->HotInteraction;
        }
        
        switch(Layout->State->Interaction.Type)
        {
            case UiInteraction_ImmediateButton:
            {
                if(MouseUp)
                {
                    Layout->State->NextToExecute = Layout->State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            // TODO(kstandbridge): Is this the best place to do this?
            case UiInteraction_EditableBool:
            {
                if(MouseUp)
                {
                    *Layout->State->Interaction.Bool = !*Layout->State->Interaction.Bool;
                    EndInteraction = true;
                }
            } break;
            
            case UiInteraction_None:
            {
                Layout->State->HotInteraction = Layout->State->NextHotInteraction;
                if(MouseDown)
                {
                    Layout->State->Interaction = Layout->State->NextHotInteraction;
                }
            } break;
            
            case UiInteraction_MultipleChoiceOption:
            {
                if(MouseUp)
                {
                    Layout->State->NextToExecute = Layout->State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            default:
            {
                if(MouseUp)
                {
                    EndInteraction = true;
                }
            }
        }
        
        if(EndInteraction)
        {
            ClearInteraction(&Layout->State->Interaction);
            ClearInteraction(&Layout->State->ClickedInteraction);
        }
        
        MouseButton = !MouseButton;
    }
    
    ClearInteraction(&Layout->State->NextHotInteraction);
}

#if 0
internal void
DrawUIRow(ui_layout *Layout, ui_layout_row *LastRow, f32 AvailableWidth, f32 AvailableHeight, tile_render_work **CurrentRenderWork)
{
    f32 CurrentY = 0.0f;
    f32 TotalHeight = AvailableHeight;
    f32 RemainingHeight = TotalHeight - Layout->UsedHeight;
    f32 HeightPerFill = RemainingHeight / Layout->FillRows;
    
    for(ui_layout_row *Row = LastRow;
        Row != 0;
        Row = Row->Next)
    {
        f32 TotalWidth = AvailableWidth/Row->ColumnCount;
        v2 UiP = V2(TotalWidth, CurrentY);
        for(ui_layout_column *Column = Row->LastColumn;
            Column != 0;
            Column = Column->Next)
        {
            if(Column->HasRows)
            {
                DrawUIRow(Layout, Column->LastRow, AvailableWidth, RemainingHeight, CurrentRenderWork);
            }
            else
            {
                f32 RemainingWidth = TotalWidth - Column->UsedWidth;
                f32 WidthPerSpacer = RemainingWidth / Column->SpacerCount;
                
                ui_element *LastElement = 0;
                for(ui_element *Element = Column->ElementSentinal->Next;
                    Element != Column->ElementSentinal;
                    Element = Element->Next)
                {
                    {
                        v2 HeightDifference = V2(0.0f, Row->MaxHeight - Element->Dim.Y);
                        v2 TextOffset = V2Subtract(Element->TextOffset, HeightDifference);
                        
                        if(Element->Type != ElementType_Spacer)
                        {
                            if(!Element->IsFloating)
                            {
                                UiP.X -= Element->Dim.X;
                            }
                            
                            if(Row->Type == LayoutType_Fill)
                            {
                                switch(Element->LabelLayout)
                                {
                                    case TextLayout_TopLeft:
                                    {
                                        TextOffset.Y -= (1.0f*HeightPerFill) - 1.0f*(Element->Dim.Y);
                                        if(Column->SpacerCount == 0)
                                        {
                                            //TextOffset.X += 0.5f*(Element->Dim.X);
                                        }
                                    } break;
                                    case TextLayout_MiddleLeft:
                                    {
                                        TextOffset.Y -= (0.5f*HeightPerFill) - 0.5f*(Element->Dim.Y);
                                        if(Column->SpacerCount == 0)
                                        {
                                            //TextOffset.X += 0.5f*(Element->Dim.X);
                                        }
                                    } break;
                                    case TextLayout_BottomLeft:
                                    {
                                        if(Column->SpacerCount == 0)
                                        {
                                            //TextOffset.X += 0.5f*(Element->Dim.X);
                                        }
                                    } break;
                                    
                                    case TextLayout_TopMiddle:
                                    {
                                        TextOffset.Y -= (1.0f*HeightPerFill) - 1.0f*(Element->Dim.Y);
                                        TextOffset.X -= 0.25f*(Element->Dim.X);
                                    } break;
                                    case TextLayout_MiddleMiddle:
                                    {
                                        TextOffset.Y -= (0.5f*HeightPerFill) - 0.5f*(Element->Dim.Y);
                                        TextOffset.X -= 0.25f*(Element->Dim.X);
                                    } break;
                                    case TextLayout_BottomMiddle:
                                    {
                                        TextOffset.X -= 0.25f*(Element->Dim.X);
                                    } break;
                                    
                                    case TextLayout_Scrollable:
                                    {
                                        TextOffset = V2Add(TextOffset, Element->Interaction.Str->Offset);
                                    } break;
                                    
                                    InvalidDefaultCase;
                                }
                                
                                HeightDifference.Y = HeightPerFill - Element->Dim.Y;
                            }
                            else
                            {
                                Assert(Row->Type == LayoutType_Auto);
                            }
                            
                            if(Column->SpacerCount == 0)
                            {
                                HeightDifference.X = (RemainingWidth/Column->ElementCount);
                                TextOffset.X -= 0.5f*HeightDifference.X;
                                if(!Element->IsFloating)
                                {
                                    UiP.X -= HeightDifference.X;
                                }
                            }
                            
                            Element->Dim = V2Add(Element->Dim, HeightDifference);
                            
                            if(Element->IsFloating)
                            {
                                UiP.Y -= Element->Dim.Y;
                            }
                            else
                            {
                                UiP.Y = CurrentY;
                            }
                            
                            if(IsInRectangle(Rectangle2(UiP, V2Add(UiP, Element->Dim)), Layout->MouseP))
                            {
                                Layout->State->NextHotInteraction = Element->Interaction;
                            }
                        }
                        
                        loaded_bitmap *DrawBuffer = PushStruct(Layout->Arena, loaded_bitmap);
                        if(Element->Type == ElementType_Spacer)
                        {
                            UiP.X -= WidthPerSpacer;
                            DrawBuffer->Width = (s32)WidthPerSpacer;
                            DrawBuffer->Height = (s32)Row->MaxHeight;
                            
                        }
                        else
                        {
                            DrawBuffer->Width = (s32)Element->Dim.X;
                            DrawBuffer->Height = (s32)Element->Dim.Y;
                        }
                        DrawBuffer->Pitch = Layout->DrawBuffer->Pitch;
                        DrawBuffer->Memory = (u8 *)Layout->DrawBuffer->Memory + (s32)UiP.X*BITMAP_BYTES_PER_PIXEL + (s32)UiP.Y*DrawBuffer->Pitch;
                        
                        
                        render_group *RenderGroup = AllocateRenderGroup(Layout->Arena, Megabytes(4), DrawBuffer);
                        assets *Assets = Layout->State->Assets;
                        
                        switch(Element->Type)
                        {
                            
                            case ElementType_Spacer:
                            {
                                PushClear(RenderGroup, Colors.Clear);
                                
                            } break;
                            
                            case ElementType_TextBox:
                            {
                                
                                v4 BorderColor = InteractionIsSelected(Layout->State, Element->Interaction) ? Colors.SelectedTextBorder : Colors.TextBorder;
                                
                                editable_string *Str = Element->Interaction.Str;
                                s32 SelectionStart;
                                s32 SelectionEnd;
                                if(Str->SelectionStart > Str->SelectionEnd)
                                {
                                    SelectionStart = Str->SelectionEnd;
                                    SelectionEnd = Str->SelectionStart;
                                }
                                else
                                {
                                    SelectionStart = Str->SelectionStart;
                                    SelectionEnd = Str->SelectionEnd;
                                }
                                
                                f32 CaretHeight = Platform.DEBUGGetLineAdvance()*Layout->Scale*0.9f;
                                DrawSelectedTextElement(RenderGroup, Assets, V2Set1(0.0f), Element->Label, TextOffset, Element->Dim, Layout->Scale, BorderColor, Colors.Text, Colors.TextBackground, Colors.SelectedText, Colors.SelectedTextBackground, SelectionStart, SelectionEnd, CaretHeight);
                                
#if 0
                                if(InteractionIsSelected(Layout->State, Element->Interaction) &&
                                   (Element->Interaction.Type == UiInteraction_TextInput))
#endif
                                {
                                    
                                    v2 CaretP = V2(TextOffset.X, -TextOffset.Y - Layout->Padding*0.25f);
#if 0
                                    rectangle2 TextBounds = GetTextSize(Assets, Layout->Scale, StringInternal(Str->Length, Str->Data));
                                    CaretP.X = TextBounds.Max.X - TextBounds.Min.X;
#endif
                                    
                                    
                                    f32 Thickness;
                                    //if(Str->SelectionStart == Str->SelectionEnd)
                                    {
                                        Thickness = Layout->Scale*30.0f;
                                        if(Thickness < 1.0f)
                                        {
                                            Thickness = 1.0f;
                                        }
                                        
                                        PushRect(RenderGroup, CaretP, V2(Thickness, CaretHeight - Layout->Padding*1.5f), Colors.Caret, Colors.Caret);
                                    }
                                }
#if 0                        
                                if(InteractionIsSelected(Layout->State, Element->Interaction) &&
                                   (Element->Interaction.Type == UiInteraction_TextInput))
                                {
                                    
                                    editable_string *Str = Element->Interaction.Str;
                                    
                                    v2 CaretP = V2(0, -TextOffset.Y - Layout->Padding*0.25f);
                                    f32 CaretHeight = Platform.DEBUGGetLineAdvance()*Layout->Scale;
                                    if(Str->SelectionStart > 0)
                                    {
                                        rectangle2 TextBounds = GetTextSize(RenderGroup, Assets, V2Set1(0.0f), Layout->Scale, StringInternal(Str->SelectionStart, Str->Data), V4Set1(1.0f));
                                        CaretP.X = TextBounds.Max.X - TextBounds.Min.X;
                                    }
                                    
                                    CaretP.X += Layout->Padding;
                                    
                                    f32 Thickness;
                                    if(Str->SelectionStart == Str->SelectionEnd)
                                    {
                                        Thickness = Layout->Scale*3.0f;
                                        if(Thickness < 1.0f)
                                        {
                                            Thickness = 1.0f;
                                        }
                                        
                                        PushRect(RenderGroup, CaretP, V2(Thickness, CaretHeight - Layout->Padding*1.5f), Colors.Caret, Colors.Caret);
                                    }
                                    else
                                    {
                                        s32 SelectedCharacters = Str->SelectionStart - Str->SelectionEnd;
                                        s32 TotalCharaceters = SelectedCharacters;
                                        if(TotalCharaceters < 0)
                                        {
                                            TotalCharaceters *= -1;
                                        }
                                        s32 StartOfSelection;
                                        if(Str->SelectionEnd > Str->SelectionStart)
                                        {
                                            StartOfSelection = Str->SelectionEnd + SelectedCharacters;
                                        }
                                        else
                                        {
                                            StartOfSelection = Str->SelectionEnd;
                                        }
                                        
                                        string SelectedStr = StringInternal(TotalCharaceters, Str->Data + StartOfSelection);
                                        
                                        rectangle2 SelectedBounds = GetTextSize(RenderGroup, Assets, V2(0, 0), Layout->Scale, SelectedStr, V4Set1(1.0f));
                                        Thickness = SelectedBounds.Max.X - SelectedBounds.Min.X;
                                        if(Str->SelectionEnd < Str->SelectionStart)
                                        {
                                            CaretP.X -= Thickness;
                                        }
                                        PushRect(RenderGroup, CaretP, V2(Thickness, CaretHeight - Layout->Padding*1.5f), Colors.SelectedTextBackground, Colors.SelectedTextBackground);
                                        WriteLine(RenderGroup, Assets, 
                                                  V2Subtract(V2(CaretP.X, 0), V2(0, TextOffset.Y)), 
                                                  Layout->Scale, SelectedStr, Colors.SelectedText);
                                    }
                                }
#endif
                                
                            } break;
                            
                            case ElementType_Checkbox:
                            {
                                f32 Thickness = Layout->Scale*3.0f;
                                if(Thickness < 1.0f)
                                {
                                    Thickness = 1.0f;
                                }
                                
                                DrawTextElement(RenderGroup, Assets, V2Set1(0.0f), Element->Label, TextOffset, Element->Dim, Layout->Scale, Colors.Clear, Colors.Clear, Colors.Text);
                                
                                v2 CheckBoxP = V2(Layout->Padding, Layout->Padding*0.75f);
                                v2 CheckBoxDim = V2Set1(Element->Dim.Y - Layout->Padding*1.5f);
                                
                                v4 CheckBoxBorder;
                                v4 CheckBoxBackground;
                                if(InteractionIsClicked(Layout->State, Element->Interaction))
                                {
                                    CheckBoxBorder = Colors.CheckBoxBorderClicked;
                                    CheckBoxBackground = Colors.CheckBoxBackgroundClicked;
                                }
                                else
                                {
                                    CheckBoxBorder = Colors.CheckBoxBorder;
                                    CheckBoxBackground = Colors.CheckBoxBackground;
                                }
                                string CheckBoxText = (*Element->Interaction.Bool) ? String("\\2713") : String("");
                                
                                DrawTextElement(RenderGroup, Assets, CheckBoxP, CheckBoxText, V2Set1(-Layout->Padding*0.25f), CheckBoxDim, Layout->Scale, CheckBoxBackground, CheckBoxBorder, Colors.Text);
                                
                            } break;
                            case ElementType_DropDown:
                            {
                                v4 ButtonColor = Colors.Button;
                                if(InteractionIsSelected(Layout->State, Element->Interaction))
                                {
                                    ButtonColor = V4(1, 0, 0, 1);
                                }
                                DrawTextElement(RenderGroup, Assets, V2Set1(0.0f), Element->Label, TextOffset, Element->Dim, Layout->Scale, ButtonColor, Colors.ButtonBorder, Colors.Text);
                                
                            } break;
                            case ElementType_Static:
                            case ElementType_Slider:
                            case ElementType_Button:
                            {
                                v4 ButtonColor = InteractionIsHot(Layout->State, Element->Interaction) ? Colors.HotButton : Colors.Button;
                                
                                if(InteractionIsClicked(Layout->State, Element->Interaction))
                                {
                                    ButtonColor = Colors.ClickedButton;
                                }
                                
                                DrawTextElement(RenderGroup, Assets, V2Set1(0.0f), Element->Label, TextOffset, Element->Dim, Layout->Scale, ButtonColor, Colors.ButtonBorder, Colors.Text);
                                
                                
                                
                            } break;
                            
                            
                            InvalidDefaultCase;
                        }
                        
                        if(Element->IsFloating)
                        {
                            tile_render_work *Work = *CurrentRenderWork;
                            Work->Group = RenderGroup;
                            Work->ClipRect.MinX = 0;
                            Work->ClipRect.MaxX = DrawBuffer->Width;
                            Work->ClipRect.MinY = 0;
                            Work->ClipRect.MaxY = DrawBuffer->Height;
                            ++(*CurrentRenderWork);
                        }
                        else
                        {
                            // TODO(kstandbridge): Perhaps tile render work for larger viewports
                            tile_render_work *Work = PushStruct(Layout->Arena, tile_render_work);
                            Work->Group = RenderGroup;
                            Work->ClipRect.MinX = 0;
                            Work->ClipRect.MaxX = DrawBuffer->Width;
                            Work->ClipRect.MinY = 0;
                            Work->ClipRect.MaxY = DrawBuffer->Height;
                            Platform.AddWorkEntry(Platform.PerFrameWorkQueue, TileRenderWorkThread, Work);
                        }
                        
                    }
                    
                    
                    LastElement = Element;
                }
            }
        }
        
        if(Row->Type == LayoutType_Auto)
        {
            CurrentY += Row->MaxHeight;
        }
        else
        {
            Assert(Row->Type == LayoutType_Fill);
            CurrentY += HeightPerFill;
        }
        
    }
}
#endif

#if 0
internal void
DrawUIInternal(ui_layout *Layout)
{
    tile_render_work *FloatingRenderWorks = 0;
    if(Layout->FloatingElements > 0)
    {
        FloatingRenderWorks = PushArray(Layout->Arena, Layout->FloatingElements, tile_render_work);
    }
    tile_render_work *CurrentRenderWork = FloatingRenderWorks;
    
    DrawUIRow(Layout, Layout->LastRow, (f32)Layout->DrawBuffer->Width, (f32)Layout->DrawBuffer->Height, &CurrentRenderWork);
    
    Platform.CompleteAllWork(Platform.PerFrameWorkQueue);
    
    CurrentRenderWork = FloatingRenderWorks;;
    for(s32 FloatingRenderWorkIndex = 0;
        FloatingRenderWorkIndex < Layout->FloatingElements;
        ++FloatingRenderWorkIndex)
    {
        Platform.AddWorkEntry(Platform.PerFrameWorkQueue, TileRenderWorkThread, CurrentRenderWork);
        ++CurrentRenderWork;
    }
    
    Platform.CompleteAllWork(Platform.PerFrameWorkQueue);
}
#endif

// TODO(kstandbridge): Ditch this crap
global s32 ColArrayIndex;
global v4 ColArray[15];


inline void
DrawDiv(ui_layout *Layout, ui_div *Head, v2 Dim, v2 *P)
{
    for(ui_div *Div = Head;
        Div != 0;
        Div = Div->Next)
    {
        f32 StartX = P->X;
        f32 StartY = P->Y;
        //if(Div->Type == UI_Div_Horizontal)
        {
            if(Div->LastChild)
            {
                f32 CurrentY = P->Y;
                DrawDiv(Layout, Div->LastChild, Dim, P);
                P->Y = CurrentY;
                P->X -= Div->Dim.X;
            }
            
            P->X -= Div->Dim.X;
            
            Div->Dim = V2(Dim.X / Div->Parent->ChildCount, Dim.Y);
            
            loaded_bitmap *DrawBuffer = PushStruct(Layout->Arena, loaded_bitmap);
            DrawBuffer->Width = (s32)Div->Dim.X;
            DrawBuffer->Height = (s32)Div->Dim.Y;
            DrawBuffer->Pitch = Layout->DrawBuffer->Pitch;
            DrawBuffer->Memory = (u8 *)Layout->DrawBuffer->Memory + (s32)P->X*BITMAP_BYTES_PER_PIXEL + (s32)P->Y*DrawBuffer->Pitch;
            render_group *RenderGroup = AllocateRenderGroup(Layout->Arena, Megabytes(4), DrawBuffer);
            
            ColArray[0] = V4(0.0f, 0.0f, 1.0f, 1.0f);
            ColArray[1] = V4(0.0f, 1.0f, 0.0f, 1.0f);
            ColArray[2] = V4(0.0f, 1.0f, 1.0f, 1.0f);
            ColArray[3] = V4(1.0f, 0.0f, 0.0f, 1.0f);
            ColArray[4] = V4(1.0f, 0.0f, 1.0f, 1.0f);
            ColArray[5] = V4(0.0f, 0.0f, 0.5f, 1.0f);
            ColArray[6] = V4(0.0f, 0.5f, 0.0f, 1.0f);
            ColArray[7] = V4(0.0f, 0.5f, 0.5f, 1.0f);
            ColArray[8] = V4(0.5f, 0.0f, 0.0f, 1.0f);
            ColArray[9] = V4(0.5f, 0.0f, 0.5f, 1.0f);
            ColArray[10] = V4(0.0f, 0.0f, 0.3f, 1.0f);
            ColArray[11] = V4(0.0f, 0.3f, 0.0f, 1.0f);
            ColArray[12] = V4(0.0f, 0.3f, 0.3f, 1.0f);
            ColArray[13] = V4(0.3f, 0.0f, 0.0f, 1.0f);
            ColArray[14] = V4(0.3f, 0.0f, 0.3f, 1.0f);
            
            
            PushClear(RenderGroup, ColArray[ColArrayIndex]);
            ColArrayIndex = (ColArrayIndex + 1) % ArrayCount(ColArray);
            
            
            tile_render_work Work;
            ZeroStruct(Work);
            Work.Group = RenderGroup;
            Work.ClipRect.MinX = 0;
            Work.ClipRect.MaxX = DrawBuffer->Width;
            Work.ClipRect.MinY = 0;
            Work.ClipRect.MaxY = DrawBuffer->Height;
            TileRenderWorkThread(&Work);
        }
        P->X = StartX;
        P->Y += Div->Dim.Y;
    }
}

internal void
IterateRowColumns(ui_layout *Layout, ui_layout_row *LastRow, v2 Dim, v2 *P)
{
    for(ui_layout_row *Row = LastRow;
        Row != 0;
        Row = Row->Next)
    {
        f32 StartX = P->X;
        for(ui_layout_column *Column = Row->LastColumn;
            Column != 0;
            Column = Column->Next)
        {
            
            if(Column->HasRows)
            {
                Assert(Column->LastRow);
                f32 CurrentY = P->Y;
                IterateRowColumns(Layout, Column->LastRow, Dim, P);
                P->Y = CurrentY;
                P->X -= Column->Dim.X;
            }
            else
            {
                P->X -= Column->Dim.X;
                
                loaded_bitmap *DrawBuffer = PushStruct(Layout->Arena, loaded_bitmap);
                DrawBuffer->Width = (s32)Column->Dim.X;
                DrawBuffer->Height = (s32)Column->Dim.Y;
                DrawBuffer->Pitch = Layout->DrawBuffer->Pitch;
                DrawBuffer->Memory = (u8 *)Layout->DrawBuffer->Memory + (s32)P->X*BITMAP_BYTES_PER_PIXEL + (s32)P->Y*DrawBuffer->Pitch;
                render_group *RenderGroup = AllocateRenderGroup(Layout->Arena, Megabytes(4), DrawBuffer);
                
                ColArray[0] = V4(0.0f, 0.0f, 1.0f, 1.0f);
                ColArray[1] = V4(0.0f, 1.0f, 0.0f, 1.0f);
                ColArray[2] = V4(0.0f, 1.0f, 1.0f, 1.0f);
                ColArray[3] = V4(1.0f, 0.0f, 0.0f, 1.0f);
                ColArray[4] = V4(1.0f, 0.0f, 1.0f, 1.0f);
                ColArray[5] = V4(0.0f, 0.0f, 0.5f, 1.0f);
                ColArray[6] = V4(0.0f, 0.5f, 0.0f, 1.0f);
                ColArray[7] = V4(0.0f, 0.5f, 0.5f, 1.0f);
                ColArray[8] = V4(0.5f, 0.0f, 0.0f, 1.0f);
                ColArray[9] = V4(0.5f, 0.0f, 0.5f, 1.0f);
                ColArray[10] = V4(0.0f, 0.0f, 0.3f, 1.0f);
                ColArray[11] = V4(0.0f, 0.3f, 0.0f, 1.0f);
                ColArray[12] = V4(0.0f, 0.3f, 0.3f, 1.0f);
                ColArray[13] = V4(0.3f, 0.0f, 0.0f, 1.0f);
                ColArray[14] = V4(0.3f, 0.0f, 0.3f, 1.0f);
                
                
                PushClear(RenderGroup, ColArray[ColArrayIndex]);
                ColArrayIndex = (ColArrayIndex + 1) % ArrayCount(ColArray);
                
                
                tile_render_work Work;
                ZeroStruct(Work);
                Work.Group = RenderGroup;
                Work.ClipRect.MinX = 0;
                Work.ClipRect.MaxX = DrawBuffer->Width;
                Work.ClipRect.MinY = 0;
                Work.ClipRect.MaxY = DrawBuffer->Height;
                TileRenderWorkThread(&Work);
                
            }
        }
        P->X = StartX;
        P->Y += Row->Dim.Y;
    }
}

internal void
CalculateUIDims(ui_layout *Layout, ui_layout_row *LastRow, v2 RowDim)
{
    for(ui_layout_row *Row = LastRow;
        Row != 0;
        Row = Row->Next)
    {
        Row->Dim = RowDim;
        for(ui_layout_column *Column = Row->LastColumn;
            Column != 0;
            Column = Column->Next)
        {
            Column->Dim = V2(Row->Dim.X / Row->ColumnCount, Row->Dim.Y);
            if(Column->HasRows)
            {
                CalculateUIDims(Layout, Column->LastRow, V2(Column->Dim.X, Column->Dim.Y / Column->RowCount));
            }
        }
    }
}

inline void
BeginDiv(ui_layout *Layout, ui_div_type Type)
{
    ui_div *Div = PushStruct(Layout->Arena, ui_div);
    Div->Type = Type;
    Div->Next = Layout->CurrentDiv;
    Layout->CurrentDiv = Div;
    
}

inline void
BeginChildDiv(ui_layout *Layout, ui_div_type Type)
{
    ui_div *Div = PushStruct(Layout->Arena, ui_div );
    Div->Type = Type;
    Div->Parent = Layout->CurrentDiv;
    Div->Next = Layout->CurrentDiv->LastChild;
    Layout->CurrentDiv->LastChild = Div;
}

inline void
EndChildDiv(ui_layout *Layout)
{
    Layout->CurrentDiv = Layout->CurrentDiv->Parent;
    ++Layout->CurrentDiv->ChildCount;
}

inline void
EndDiv(ui_layout *Layout)
{
    Layout->CurrentDiv = Layout->CurrentDiv->Parent;
    ++Layout->CurrentDiv->ChildCount;
}

inline void
CalculateSizeForDiv(ui_layout *Layout, ui_div *Head, v2 Dim)
{
    for(ui_div *Div = Head;
        Div != 0;
        Div = Div->Next)
    {
        if(Div->Type == UI_Div_Horizontal)
        {
            Div->Dim = V2(Dim.X / Div->Parent->ChildCount, Dim.Y);
        }
        else
        {
            Assert(Div->Type == UI_Div_Vertical);
            Div->Dim = V2(Dim.X, Dim.Y / Div->Parent->ChildCount);
        }
        if(Div->LastChild)
        {
            CalculateSizeForDiv(Layout, Div->LastChild, Div->Dim);
        }
    }
}

#if 0

//
// Main
//     Sub
//     Sub
// End
//
//
//

#endif

internal void
EndUIFrame(ui_layout *Layout, app_input *Input)
{
    HandleUIInteractionsInternal(Layout, Input);
    
    Layout->State->LastMouseP = Layout->MouseP;
    
    
    ColArrayIndex = 0;
    v2 P = V2((f32)Layout->DrawBuffer->Width, 0.0f);
    
#if 0    
    v2 Dim = V2((f32)Layout->DrawBuffer->Width, (f32)Layout->DrawBuffer->Height);
    CalculateSizeForDiv(Layout, &Layout->MainDiv, Dim);
    DrawDiv(Layout, &Layout->MainDiv, Dim, &P);
#else
    CalculateUIDims(Layout, Layout->LastRow, V2((f32)Layout->DrawBuffer->Width, (f32)Layout->DrawBuffer->Height / Layout->RowCount));
    //DrawUIInternal(Layout);
    IterateRowColumns(Layout, Layout->LastRow, V2((f32)Layout->DrawBuffer->Width, (f32)Layout->DrawBuffer->Height), &P);
#endif
}

inline void
EndColumn(ui_layout *Layout)
{
    Layout->CurrentColumn = Layout->CurrentColumn->Parent;
}

inline void
BeginColumn(ui_layout *Layout)
{
    ui_layout_column *Column = PushStruct(Layout->Arena, ui_layout_column);
    ZeroStruct(*Column);
    Column->Next = Layout->CurrentRow->LastColumn;
    Layout->CurrentRow->LastColumn = Column;
    
    ++Layout->CurrentRow->ColumnCount;
    Column->Parent = Layout->CurrentColumn;
    Layout->CurrentColumn = Column;
}

inline void
SetRowLayoutType(ui_layout *Layout, ui_layout_type Type)
{
    Assert(Layout->CurrentRow);
    Layout->CurrentRow->Type = Type;
}

inline void
BeginRow(ui_layout *Layout)
{
    if(Layout->CurrentColumn == 0)
    {
        ui_layout_row *Row = PushStruct(Layout->Arena, ui_layout_row);
        ZeroStruct(*Row);
        Row->Next = Layout->LastRow;
        Layout->LastRow = Row;
        
        ++Layout->RowCount;
        Row->Parent = Layout->CurrentRow;
        Layout->CurrentRow = Row;
    }
    else
    {
        ui_layout_row *Row = PushStruct(Layout->Arena, ui_layout_row);
        ZeroStruct(*Row);
        Row->Next = Layout->CurrentColumn->LastRow;
        Layout->CurrentColumn->LastRow = Row;
        
        Layout->CurrentColumn->HasRows = true;
        ++Layout->CurrentColumn->RowCount;
        
        Row->Parent = Layout->CurrentRow;
        Layout->CurrentRow = Row;
    }
    
}

inline void
EndRow(ui_layout *Layout)
{
    ui_layout_row *Row = Layout->CurrentRow;
    
    if(Row->Type == LayoutType_Fill)
    {
        ++Layout->FillRows;
    }
    else
    {
        Assert(Row->Type == LayoutType_Auto);
        
    }
    Layout->CurrentRow = Row->Parent;
    
#if 0    
    
    for(ui_layout_column *Column = Row->ColumnSentinal->Next;
        Column != Row->ColumnSentinal;
        Column = Column->Next)
    {
        Column->Dim.X = Row->Dim.X / Row->ColumnCount;
        if(Column->HasRows)
        {
            // TODO(kstandbridge): I'm not sure what we would do in this case?
        }
        else
        {
            ui_element *LastElement = 0;
            for(ui_element *Element = Column->ElementSentinal->Next;
                Element != Column->ElementSentinal;
                Element = Element->Next)
            {
                if(Element->Type == ElementType_Spacer)
                {
                    ++Column->SpacerCount;
                }
                else if(Element->IsFloating)
                {
                    if(LastElement)
                    {
                        Element->Dim = LastElement->Dim;
                        Element->TextOffset = LastElement->TextOffset;
                    }
                    else
                    {
                        Assert(!"First element cannot be floating as it needs a parent to float from")
                    }
                }
                else
                {
                    ++Column->ElementCount;
                    string LabelStr;
                    if(Element->Label.Size == 0)
                    {
                        LabelStr = String("K");
                    }
                    else
                    {
                        LabelStr = Element->Label;
                    }
                    
                    rectangle2 TextBounds = GetTextSize(Layout->State->Assets, Layout->Scale, LabelStr);
                    Element->TextOffset = V2(-Layout->Padding, TextBounds.Min.Y - Layout->Padding);
                    Element->Dim = V2(TextBounds.Max.X - TextBounds.Min.X, 
                                      TextBounds.Max.Y - TextBounds.Min.Y);
                    Element->Dim = V2Add(Element->Dim, V2Set1(Layout->Padding*2.0f));
                    
                    if(Element->Type == ElementType_Checkbox)
                    {
                        Element->Dim.X += Element->Dim.Y - Layout->Padding * 0.5f;
                        Element->TextOffset.X -= Element->Dim.Y - Layout->Padding * 0.5f; 
                    }
                    
                    
                    if((Element->MinDim.X != 0) && 
                       (Element->Dim.X < Element->MinDim.X))
                    {
                        Element->TextOffset.X -= (Element->MinDim.X - Element->Dim.X)*0.5f;
                        Element->Dim.X = Element->MinDim.X;
                    }
                    if((Element->MinDim.Y != 0) && 
                       (Element->Dim.Y < Element->MinDim.Y))
                    {
                        Element->Dim.Y = Element->MinDim.Y;
                    }
                    
                    if((Element->MaxDim.X != 0) &&
                       (Element->Dim.X > Element->MaxDim.X))
                    {
                        Element->TextOffset.X += (Element->Dim.X - Element->MaxDim.X)*0.5f;
                        Element->Dim.X = Element->MaxDim.X;
                    }
                    if((Element->MaxDim.Y != 0) && 
                       (Element->Dim.Y > Element->MaxDim.Y))
                    {
                        Element->Dim.Y = Element->MaxDim.Y;
                    }
                    
                    if(Element->Dim.X > Layout->DrawBuffer->Width - Column->UsedWidth)
                    {
                        Element->Dim.X = Layout->DrawBuffer->Width - Column->UsedWidth;
                    }
                    
                    Column->UsedWidth += Element->Dim.X;
                    if(Element->Dim.Y > Row->MaxHeight)
                    {
                        Row->MaxHeight = Element->Dim.Y;
                    }
                }
                LastElement = Element;
            }
        }
    }
    
    if(Row->Type == LayoutType_Auto)
    {
        Layout->UsedHeight += Row->MaxHeight;
    }
#endif
    
}

#if 0
inline ui_element *
PushElementInternal(ui_layout *Layout, ui_element_type ElementType, ui_element_type InteractionType, u32 ID, string Label, ui_text_layout_type LabelLayout, void *Target)
{
    ui_element *Element = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Element);
    
    if(Layout->CurrentColumn->ElementSentinal == 0)
    {
        Layout->CurrentColumn->ElementSentinal = PushStruct(Layout->Arena, ui_element);
        DLIST_INIT(Layout->CurrentColumn->ElementSentinal);
    }
    
    DLIST_INSERT(Layout->CurrentColumn->ElementSentinal, Element);
    
    Element->Type = ElementType;
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = ID;
    Element->Interaction.Type = InteractionType;
    Element->Interaction.Generic = Target;
    Element->Label = Label;
    Element->LabelLayout = LabelLayout;
    
    return Element;
}

inline void
PushSpacerElement(ui_layout *Layout)
{
    PushElementInternal(Layout, ElementType_Spacer, UiInteraction_NOP, 0, String(""), TextLayout_None, 0);
}

inline void
PushStaticElement(ui_layout *Layout, u32 ID, string Label, ui_text_layout_type LabelLayout)
{
    PushElementInternal(Layout, ElementType_Static, UiInteraction_None, ID, Label, LabelLayout, 0);
}


inline b32
PushButtonElement(ui_layout *Layout, u32 ID, string Label)
{
    ui_element *Element = PushElementInternal(Layout, ElementType_Button, UiInteraction_ImmediateButton, ID, Label, TextLayout_MiddleMiddle, 0);
    
    b32 Result = InteractionsAreEqual(Element->Interaction, Layout->State->ToExecute);
    return Result;
}

inline void
PushScrollElement(ui_layout *Layout, u32 ID, string Label, v2 *TargetP)
{
    ui_element *Element = PushElementInternal(Layout, ElementType_Slider, UiInteraction_Draggable, ID, Label, TextLayout_MiddleMiddle, TargetP);
    
    if(InteractionsAreEqual(Element->Interaction, Layout->State->Interaction))
    {
        v2 *P = Layout->State->Interaction.P;
        *P = V2Add(*P, V2Multiply(V2Set1(0.1f), Layout->dMouseP));
    }
}

inline void
PushCheckboxElement(ui_layout *Layout, u32 ID, string Label, b32 *TargetBool)
{
    ui_element *Element = PushElementInternal(Layout, ElementType_Checkbox, UiInteraction_EditableBool, ID, Label, TextLayout_MiddleMiddle, TargetBool);
}

inline void
PushTextInputElement(ui_layout *Layout, u32 ID, editable_string *Target)
{
    PushElementInternal(Layout, ElementType_TextBox, UiInteraction_TextInput, ID, StringInternal(Target->Length, Target->Data), TextLayout_Scrollable, Target);
}

inline void
PushDropDownElement(ui_layout *Layout, u32 ID, string *Labels, s32 LabelCount, s32 *Target)
{
    ui_element *Element = PushElementInternal(Layout, ElementType_DropDown, UiInteraction_ImmediateButton, ID, Labels[*Target], TextLayout_MiddleMiddle, Target);
    
    for(s32 LabelIndex = LabelCount - 1;
        LabelIndex > 0;
        --LabelIndex)
    {
        ui_interaction Interaction;
        ZeroStruct(Interaction);
        Interaction.ID = LabelIndex + 72; // TODO(kstandbridge): How??!?
        Interaction.Type = UiInteraction_MultipleChoiceOption;
        Interaction.Generic = Target;
        
        if(InteractionsAreEqual(Interaction, Layout->State->ToExecute))
        {
            *Target = LabelIndex;
        }
        
#if 0   
        // TODO(kstandbridge): Fix the drop down choices
        if(InteractionIsSelected(Layout->State, Element->Interaction))
        {
            ui_element *Choice = PushStruct(Layout->Arena, ui_element);
            ZeroStruct(*Choice);
            Choice->Next = Element->Next;
            Element->Next = Choice;
            Layout->LastRow->LastElement = Element;
            
            Choice->Type = ElementType_Button;
            Choice->Interaction = Interaction;
            Choice->Label = Labels[LabelIndex];
            Choice->IsFloating = true;
            
            ++Layout->FloatingElements;
        }
#endif
        
    }
    
}

inline void
SetElementMinDim(ui_layout *Layout, f32 Width, f32 Height)
{
    Layout->CurrentElement->MinDim = V2(Width, Height);
}

inline void
SetElementMaxDim(ui_layout *Layout, f32 Width, f32 Height)
{
    Layout->CurrentElement->MaxDim = V2(Width, Height);
}
#endif

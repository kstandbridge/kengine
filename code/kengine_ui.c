
internal void
DrawTextElement(ui_layout *Layout, v2 P, string Str, v2 TextOffset, v2 Dim, f32 Scale, v4 BackgroundColor, v4 BorderColor, v4 TextColor)
{
    PushRect(Layout->RenderGroup, P, Dim, BackgroundColor, BackgroundColor);
    
    f32 Thickness = Scale*3.0f;
    if(Thickness < 1.0f)
    {
        Thickness = 1.0f;
    }
    PushRectOutline(Layout->RenderGroup, P, Dim, BorderColor, BorderColor, Thickness);
    
    WriteLine(Layout->RenderGroup, Layout->State->Assets, V2Subtract(P, TextOffset), Scale, Str, TextColor);
}

internal ui_layout
BeginUIFrame(ui_state *UiState, memory_arena *Arena, render_group *RenderGroup, app_input *Input, f32 Padding, f32 Scale)
{
    ui_layout Result;
    ZeroStruct(Result);
    
    Result.State = UiState;
    Result.Arena = Arena;
    Result.RenderGroup = RenderGroup;
    
    Result.Scale = Scale;
    Result.MouseP = V2(Input->MouseX, Input->MouseY);
    Result.dMouseP = V2Subtract(Result.MouseP, UiState->LastMouseP);
    Result.Padding = Padding;
    
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
            
            case UiInteraction_None:
            {
                Layout->State->HotInteraction = Layout->State->NextHotInteraction;
                if(MouseDown)
                {
                    Layout->State->Interaction = Layout->State->NextHotInteraction;
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

internal void
DrawUIInternal(ui_layout *Layout)
{
    f32 CurrentY = 0.0f;
    f32 TotalHeight = (f32)Layout->RenderGroup->OutputTarget->Height;
    f32 RemainingHeight = TotalHeight - Layout->UsedHeight;
    f32 HeightPerFill = RemainingHeight / Layout->FillRows;
    for(ui_layout_row *Row = Layout->LastRow;
        Row;
        Row = Row->Next)
    {
        f32 TotalWidth = (f32)Layout->RenderGroup->OutputTarget->Width;
        f32 RemainingWidth = TotalWidth - Row->UsedWidth;
        f32 WidthPerSpacer = RemainingWidth / Row->SpacerCount;
        v2 P = V2(TotalWidth, CurrentY);
        
        for(ui_element *Element = Row->LastElement;
            Element;
            Element = Element->Next)
        {
            if(Element->Type == ElementType_Spacer)
            {
                P.X -= WidthPerSpacer;
            }
            else
            {
                P.X -= Element->Dim.X;
                v2 HeightDifference = V2(0.0f, Row->MaxHeight - Element->Dim.Y);
                v2 TextOffset = V2Subtract(Element->TextOffset, HeightDifference);
                if(Row->Type == LayoutType_Fill)
                {
                    TextOffset.Y -= (0.5f*HeightPerFill) - 0.5f*(Element->Dim.Y);
                    HeightDifference.Y = HeightPerFill - Element->Dim.Y;
                }
                else
                {
                    Assert(Row->Type == LayoutType_Auto);
                }
                
                if(Row->SpacerCount == 0)
                {
                    HeightDifference.X = (RemainingWidth/Row->ElementCount);
                    TextOffset.X -= 0.5f*HeightDifference.X;
                    P.X -= HeightDifference.X;
                }
                
                Element->Dim = V2Add(Element->Dim, HeightDifference);
                
                if(IsInRectangle(Rectangle2(P, V2Add(P, Element->Dim)), Layout->MouseP))
                {
                    Layout->State->NextHotInteraction = Element->Interaction;
                }
                
                switch(Element->Type)
                {
                    case ElementType_TextBox:
                    {
                        v4 BorderColor = InteractionIsSelected(Layout->State, Element->Interaction) ? Colors.SelectedTextBorder : Colors.TextBorder;
                        
                        DrawTextElement(Layout, P, Element->Label, TextOffset, Element->Dim, Layout->Scale, Colors.TextBackground, BorderColor, Colors.Text);
                        
                        if(InteractionIsSelected(Layout->State, Element->Interaction) &&
                           (Element->Interaction.Type == UiInteraction_TextInput))
                        {
                            
                            editable_string *Str = Element->Interaction.Str;
                            
                            v2 CaretP = V2(0, Layout->Padding);
                            if(Str->SelectionStart > 0)
                            {
                                rectangle2 TextBounds = GetTextSize(Layout->RenderGroup, Layout->State->Assets, V2(0, 0), Layout->Scale, StringInternal(Str->SelectionStart, Str->Data), V4Set1(1.0f));
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
                                
                                PushRect(Layout->RenderGroup, V2Add(P, CaretP), V2(Thickness, Element->Dim.Y - Layout->Padding*1.5f), Colors.Caret, Colors.Caret);
                                
                                // TODO(kstandbridge): Remove debug info
                                DrawTextElement(Layout, V2Subtract(P, V2(0, Row->MaxHeight)), FormatString(Layout->Arena, "%d / %d", Str->SelectionStart, Str->SelectionEnd), TextOffset, Element->Dim, Layout->Scale, Colors.TextBackground, Colors.ButtonBorder, Colors.Text);
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
                                rectangle2 SelectedBounds = GetTextSize(Layout->RenderGroup, Layout->State->Assets, V2(0, 0), Layout->Scale, SelectedStr, V4Set1(1.0f));
                                Thickness = SelectedBounds.Max.X - SelectedBounds.Min.X;
                                if(Str->SelectionEnd < Str->SelectionStart)
                                {
                                    CaretP.X -= Thickness;
                                }
                                PushRect(Layout->RenderGroup, V2Add(P, CaretP), V2(Thickness, Element->Dim.Y - Layout->Padding*1.5f), Colors.SelectedTextBackground, Colors.SelectedTextBackground);
                                WriteLine(Layout->RenderGroup, Layout->State->Assets, 
                                          V2Subtract(V2Add(P, V2(CaretP.X, 0)), V2(0, TextOffset.Y)), 
                                          Layout->Scale, SelectedStr, Colors.SelectedText);
                                
                                // TODO(kstandbridge): Remove debug info
                                DrawTextElement(Layout, V2Subtract(P, V2(0, Row->MaxHeight)), FormatString(Layout->Arena, "%d / %d - %S", Str->SelectionStart, Str->SelectionEnd, SelectedStr), TextOffset, Element->Dim, Layout->Scale, Colors.TextBackground, Colors.ButtonBorder, Colors.Text);
                            }
                        }
                        
                    } break;
                    
                    case ElementType_Static:
                    case ElementType_Checkbox:
                    case ElementType_Slider:
                    case ElementType_Button:
                    {
                        v4 ButtonColor = InteractionIsHot(Layout->State, Element->Interaction) ? Colors.HotButton : Colors.Button;
                        
                        if(InteractionIsClicked(Layout->State, Element->Interaction))
                        {
                            ButtonColor = Colors.ClickedButton;
                        }
                        
                        DrawTextElement(Layout, P, Element->Label, TextOffset, Element->Dim, Layout->Scale, ButtonColor, Colors.ButtonBorder, Colors.Text);
                        
                        
                        
                    } break;
                    
                    InvalidDefaultCase;
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

internal void
EndUIFrame(ui_layout *Layout, app_input *Input)
{
    Assert(!Layout->IsCreatingRow);
    
    HandleUIInteractionsInternal(Layout, Input);
    
    Layout->State->LastMouseP = Layout->MouseP;
    
    DrawUIInternal(Layout);
}

inline void
BeginRow(ui_layout *Layout, ui_layout_type Type)
{
    Assert(!Layout->IsCreatingRow);
    Layout->IsCreatingRow = true;
    
    ui_layout_row *Row = PushStruct(Layout->Arena, ui_layout_row);
    ZeroStruct(*Row);
    Row->Next = Layout->LastRow;
    Layout->LastRow = Row;
    
    Row->Type = Type;
}

inline void
EndRow(ui_layout *Layout)
{
    Assert(Layout->IsCreatingRow);
    Layout->IsCreatingRow = false;
    
    ui_layout_row *Row = Layout->LastRow;
    if(Row->Type == LayoutType_Fill)
    {
        ++Layout->FillRows;
    }
    else
    {
        Assert(Row->Type == LayoutType_Auto);
    }
    
    for(ui_element *Element = Row->LastElement;
        Element;
        Element = Element->Next)
    {
        if(Element->Type == ElementType_Spacer)
        {
            ++Row->SpacerCount;
        }
        else
        {
            ++Row->ElementCount;
            rectangle2 TextBounds = GetTextSize(Layout->RenderGroup, Layout->State->Assets, V2(0, 0), Layout->Scale, Element->Label, V4Set1(1.0f));
            Element->TextOffset = V2(-Layout->Padding, TextBounds.Min.Y - Layout->Padding);
            Element->Dim = V2(TextBounds.Max.X - TextBounds.Min.X, 
                              TextBounds.Max.Y - TextBounds.Min.Y);
            Element->Dim = V2Add(Element->Dim, V2Set1(Layout->Padding*2.0f));
            
            if((Element->MinDim.X != 0) && 
               (Element->Dim.X < Element->MinDim.X))
            {
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
                Element->Dim.X = Element->MaxDim.X;
            }
            if((Element->MaxDim.Y != 0) && 
               (Element->Dim.Y > Element->MaxDim.Y))
            {
                Element->Dim.Y = Element->MaxDim.Y;
            }
            
            Row->UsedWidth += Element->Dim.X;
            if(Element->Dim.Y > Row->MaxHeight)
            {
                Row->MaxHeight = Element->Dim.Y;
            }
        }
    }
    
    if(Row->Type == LayoutType_Auto)
    {
        Layout->UsedHeight += Row->MaxHeight;
    }
}

inline ui_element *
PushElementInternal(ui_layout *Layout, ui_element_type ElementType, ui_element_type InteractionType, u32 ID, string Str, void *Target)
{
    Assert(Layout->IsCreatingRow);
    
    ui_element *Element = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Element);
    Element->Next = Layout->LastRow->LastElement;
    Layout->LastRow->LastElement = Element;
    
    Element->Type = ElementType;
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = ID;
    Element->Interaction.Type = InteractionType;
    Element->Interaction.Generic = Target;
    Element->Label = Str;
    
    return Element;
}

inline void
PushSpacerElement(ui_layout *Layout)
{
    Assert(Layout->IsCreatingRow);
    
    PushElementInternal(Layout, ElementType_Spacer, UiInteraction_NOP, 0, String(""), 0);
}

inline void
PushStaticElement(ui_layout *Layout, u32 ID, string Str)
{
    Assert(Layout->IsCreatingRow);
    
    PushElementInternal(Layout, ElementType_Static, UiInteraction_None, ID, Str, 0);
}


inline b32
PushButtonElement(ui_layout *Layout, u32 ID, string Str)
{
    Assert(Layout->IsCreatingRow);
    
    ui_element *Element = PushElementInternal(Layout, ElementType_Button, UiInteraction_ImmediateButton, ID, Str, 0);
    
    b32 Result = InteractionsAreEqual(Element->Interaction, Layout->State->ToExecute);
    return Result;
}

inline void
PushScrollElement(ui_layout *Layout, u32 ID, string Str, v2 *TargetP)
{
    Assert(Layout->IsCreatingRow);
    
    ui_element *Element = PushElementInternal(Layout, ElementType_Slider, UiInteraction_Draggable, ID, Str, TargetP);
    
    if(InteractionsAreEqual(Element->Interaction, Layout->State->Interaction))
    {
        v2 *P = Layout->State->Interaction.P;
        *P = V2Add(*P, V2Multiply(V2Set1(0.01f), Layout->dMouseP));
    }
}

inline void
PushTextInputElement(ui_layout *Layout, u32 ID, editable_string *Target)
{
    Assert(Layout->IsCreatingRow);
    
    PushElementInternal(Layout, ElementType_TextBox, UiInteraction_TextInput, ID, StringInternal(Target->Length, Target->Data), Target);
}

inline void
SetElementMinDim(ui_layout *Layout, f32 Width, f32 Height)
{
    Assert(Layout->IsCreatingRow);
    
    Layout->LastRow->LastElement->MinDim = V2(Width, Height);
}

inline void
SetElementMaxDim(ui_layout *Layout, f32 Width, f32 Height)
{
    Assert(Layout->IsCreatingRow);
    
    Layout->LastRow->LastElement->MaxDim = V2(Width, Height);
}

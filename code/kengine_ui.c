
inline void
SetRowWidth(ui_layout *Layout, f32 Width)
{
    Layout->CurrentElement->Parent->SetWidthChildCount = 1;
    Layout->CurrentElement->Parent->UsedDim.X = Width;
    Layout->CurrentElement->Dim.X = Width;
}

inline void
BeginRow(ui_layout *Layout)
{
    ui_element *Element = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Element);
    Element->UsedDim.Y = Layout->DefaultRowHeight + Layout->Padding*2.0f;
    if((Layout->CurrentElement->FirstChild == 0) || 
       (Layout->CurrentElement->FirstChild->Type != Element_Row))
    {
        ++Layout->CurrentElement->ChildCount;
    }
    
    Element->Next = Layout->CurrentElement->FirstChild;
    Layout->CurrentElement->FirstChild = Element;
    
    
    Element->Parent = Layout->CurrentElement;
    Layout->CurrentElement = Element;
    
    Element->Type = Element_Row;
    
    if((Element->Next) &&
       (Element->Next->Dim.X > 0.0f))
    {
        SetRowWidth(Layout, Element->Next->Dim.X);
    }
}

inline void
EndRow(ui_layout *Layout)
{
    Layout->CurrentElement = Layout->CurrentElement->Parent;
    
}

inline ui_element *
PushElement(ui_layout *Layout)
{
    ui_element *Result = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Result);
    Result->Next = Layout->CurrentElement->FirstChild;
    Layout->CurrentElement->FirstChild = Result;
    
    ++Layout->CurrentElement->ChildCount;
    
    return Result;
}

inline void
Spacer(ui_layout *Layout)
{
    ui_element *Element = PushElement(Layout);
    Element->Type = Element_Spacer;
}

inline void
Label(ui_layout *Layout, string Text)
{
    ui_element *Element = PushElement(Layout);
    Element->Type = Element_Label;
    Element->Text = Text;
}

inline void
Checkbox(ui_layout *Layout, string Text, b32 *Target)
{
    ui_element *Element = PushElement(Layout);
    Element->Type = Element_Checkbox;
    Element->Text = Text;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = ++Layout->CurrentId;
    Element->Interaction.Type = Interaction_EditableBool;
    Element->Interaction.Generic = Target;
}

inline void
Textbox(ui_layout *Layout, editable_string *Target)
{
    ui_element *Element = PushElement(Layout);
    Element->Type = Element_Textbox;
    Element->Text = StringInternal(Target->Length, Target->Data);;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = ++Layout->CurrentId;
    Element->Interaction.Type = Interaction_EditableText;
    Element->Interaction.Generic = Target;
}

inline void
MultilineTextbox(ui_layout *Layout, editable_string *Target)
{
    ui_element *Element = PushElement(Layout);
    Element->Type = Element_MultilineTextbox;
    Element->Text = StringInternal(Target->Length, Target->Data);;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = ++Layout->CurrentId;
    Element->Interaction.Type = Interaction_EditableMultilineText;
    Element->Interaction.Generic = Target;
}

inline b32
Button(ui_state *State, ui_layout *Layout, string Text)
{
    ui_element *Element = PushElement(Layout);
    Element->Type = Element_Button;
    Element->Text = Text;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = ++Layout->CurrentId;
    Element->Interaction.Type = Interaction_ImmediateButton;
    
    b32 Result = InteractionsAreEqual(Element->Interaction, State->ToExecute);
    return Result;
}

internal ui_layout *
BeginUIFrame(memory_arena *Arena, ui_state *State, app_input *Input, assets *Assets, f32 Scale, f32 Padding, loaded_bitmap *DrawBuffer)
{
    ui_layout *Result = PushStruct(Arena, ui_layout);
    ZeroStruct(*Result);
    
    Result->Arena = Arena;
    Result->Assets = Assets;
    Result->Scale = Scale;
    Result->Padding = Padding;
    Result->DefaultRowHeight = Platform.DEBUGGetLineAdvance()*Result->Scale;
    
    Result->DrawBuffer = DrawBuffer;
    
    Result->SentinalElement.Dim = V2((f32)DrawBuffer->Width, (f32)DrawBuffer->Height);
    Result->CurrentElement = &Result->SentinalElement;
    
    Result->CurrentId = 0;
    
    
    Result->MouseP = V2(Input->MouseX, Input->MouseY);
    Result->dMouseP = V2Subtract(Result->MouseP, State->LastMouseP);
    Result->MouseZ = Input->MouseZ;
    Result->DeltaTime = Input->dtForFrame;
    
    State->ToExecute = State->NextToExecute;
    ClearInteraction(&State->NextToExecute);
    
    return Result;
}

// TODO(kstandbridge): Ditch this crap
global s32 ColArrayIndex;
global v4 ColArray[25];

inline void
DrawSpacer(ui_layout *Layout, render_group *RenderGroup, ui_element *Element, v2 Offset)
{
    PushRect(RenderGroup, V2Add(Offset, V2Set1(Layout->Padding)), V2Subtract(Element->Dim, V2Set1(Layout->Padding*2.0f)), ColArray[ColArrayIndex], ColArray[ColArrayIndex]);
    ColArrayIndex = (ColArrayIndex + 1) % ArrayCount(ColArray);
}

inline void
DrawLabel(ui_layout *Layout, render_group *RenderGroup, ui_element *Element, v2 Offset)
{
    v2 TextP = V2(Layout->Padding, Layout->Padding + Element->TextBounds.Max.Y*0.5f);
    TextP = V2Add(TextP, Offset);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.LabelText, Rectangle2(Offset, V2Add(Offset, Element->Dim)));
}

inline void
DrawCheckbox(ui_state *State, ui_layout *Layout, render_group *RenderGroup, ui_element *Element, v2 Offset)
{
    v2 CheckboxP = V2(Layout->Padding, Layout->Padding*2.0f);
    CheckboxP = V2Add(CheckboxP, Offset);
    v2 CheckboxDim = V2Set1(Platform.DEBUGGetLineAdvance()*Layout->Scale*0.75f);
    string CheckText = (*Element->Interaction.Bool) ? String("\\2713") : String("");
    
    v4 CheckboxBackground = Colors.CheckboxBackground;
    v4 CheckboxBorder = Colors.CheckboxBorder;
    v4 CheckboxText = Colors.CheckboxText;
    
    if(InteractionIsHot(State, Element->Interaction))
    {
        CheckboxBackground = Colors.CheckboxHotBackground;
        CheckboxBorder = Colors.CheckboxHotBorder;
        CheckboxText = Colors.CheckboxHotText;
    }
    
    if(InteractionIsClicked(State, Element->Interaction))
    {
        CheckboxBackground = Colors.CheckboxClickedBackground;
        CheckboxBorder = Colors.CheckboxClickedBorder;
        CheckboxText = Colors.CheckboxClickedText;
    }
    
    PushRect(RenderGroup, CheckboxP, CheckboxDim, CheckboxBackground, CheckboxBackground);
    
    PushRectOutline(RenderGroup, CheckboxP, CheckboxDim, CheckboxBorder, CheckboxBorder, Layout->Scale);
    if(CheckText.Size > 0)
    {
        WriteLine(RenderGroup, Layout->Assets, V2Subtract(CheckboxP, V2Set1(-Layout->Padding)), Layout->Scale, CheckText, CheckboxText, Rectangle2(Offset, V2Add(Offset, Element->Dim)));
    }
    
    v2 TextP = V2(CheckboxP.X + CheckboxDim.X + Layout->Padding, Offset.Y + Layout->Padding + Element->TextBounds.Max.Y*0.5f);
    
    if(InteractionIsSelected(State, Element->Interaction))
    {
        v2 OutlineP = V2Subtract(TextP, V2(Layout->Padding*0.5f, Layout->Padding*1.5f));
        v2 OutlineDim = V2Add(Element->TextBounds.Max, V2(Layout->Padding*1.5f, Layout->Padding*3.0f));
        PushRectOutline(RenderGroup, OutlineP, OutlineDim, Colors.CheckboxSelectedBackground, Colors.CheckboxSelectedBackgroundAlt, Layout->Scale);
    }
    
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.CheckboxText, Rectangle2(Offset, V2Add(Offset, Element->Dim)));
    
}

inline void
DrawTextbox(ui_state *State, ui_layout *Layout, render_group *RenderGroup, ui_element *Element, v2 TextOffset, v2 Offset)
{
    PushRect(RenderGroup, V2Add(Offset, V2Set1(Layout->Padding)), V2Subtract(Element->Dim, V2Set1(Layout->Padding*2.0f)), 
             Colors.TextboxBackground, Colors.TextboxBackground);
    
    v4 TextboxBorderColor = Colors.TextboxBorder;
    if(InteractionIsSelected(State, Element->Interaction))
    {
        TextboxBorderColor = Colors.TextboxSelectedBorder;
    }
    
    v2 TextP = V2(Layout->Padding*2.0f, Layout->Padding + Element->TextBounds.Max.Y*0.5f);
    TextP = V2Add(TextP, Offset);
    TextP = V2Add(TextP, TextOffset);
    
    v2 TextClipMin = V2Add(Offset, V2Set1(Layout->Padding*2.0f));
    v2 TextClipMax = V2Add(TextClipMin, V2Subtract(Element->Dim, V2Set1(Layout->Padding*7.0f)));
    rectangle2 TextClip = Rectangle2(TextClipMin, TextClipMax); 
    
    if(InteractionIsSelected(State, Element->Interaction))
    {
        editable_string *Text = Element->Interaction.Text;
        s32 SelectionStart;
        s32 SelectionEnd;
        if(Text->SelectionStart > Text->SelectionEnd)
        {
            SelectionStart = Text->SelectionEnd;
            SelectionEnd = Text->SelectionStart;
        }
        else
        {
            SelectionStart = Text->SelectionStart;
            SelectionEnd = Text->SelectionEnd;
        }
        f32 CaretHeight = Platform.DEBUGGetLineAdvance()*Layout->Scale*0.75f;
        WriteSelectedLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, 
                          Colors.TextboxText, Colors.TextboxSelectedText, Colors.TextboxSelectedBackground, 
                          SelectionStart, SelectionEnd, CaretHeight, TextClip);
    }
    else
    {
        WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.TextboxText, TextClip);
    }
    
    
    PushRectOutline(RenderGroup, V2Add(Offset, V2Set1(Layout->Padding)), V2Subtract(Element->Dim, V2Set1(Layout->Padding*2.0f)), 
                    TextboxBorderColor, TextboxBorderColor, Layout->Scale);
}

inline void
DrawMultilineTextbox(ui_state *State, ui_layout *Layout, render_group *RenderGroup, ui_element *Element, v2 Offset)
{
    editable_string *Text = Element->Interaction.Text;
    DrawTextbox(State, Layout, RenderGroup, Element, Text->Offset, Offset);
    
    string UpText = String("\\02C4");
    string DownText = String("\\02C5");
    rectangle2 TextBounds = GetTextSize(Layout->Assets, Layout->Scale, UpText);
    v2 ArrowDim = V2Subtract(TextBounds.Max, TextBounds.Min);
    
    v2 TextDim = V2Subtract(Element->TextBounds.Max, Element->TextBounds.Min);
    f32 LineAdvance = Layout->DefaultRowHeight*3.0f*Layout->DeltaTime;
    
    v2 Dim = V2(ArrowDim.X + Layout->Padding*2.0f, Element->Dim.Y - Layout->Padding*2.0f - Layout->Scale*6.0f);
    v2 P = V2(Element->Dim.X - Layout->Padding - Layout->Scale*3.0f - Dim.X, Layout->Padding + Layout->Scale*3.0f);
    P = V2Add(P, Offset);
    PushRect(RenderGroup, P, Dim, Colors.ScrollbarBackground, Colors.ScrollbarBackground);
    
    v4 ButtonBackColor;
    v4 ButtonTextColor;
    v4 SliderColor;
    
    v2 TextP = V2(P.X + Dim.X*0.5f - ArrowDim.X*0.5f - Layout->Padding*0.25f, P.Y + Layout->Padding*0.5f);
    v2 ButtonP = V2(P.X, P.Y + Layout->Scale*3.0f);
    v2 ButtonDim = V2Set1(Dim.X - Layout->Scale*3.0f);
    
    // TODO(kstandbridge): This shouldn't be done at the drawing stage.
    if(IsInRectangle(Rectangle2(ButtonP, V2Add(ButtonP, ButtonDim)), Layout->MouseP))
    {
        if(InteractionIsClicked(State, Element->Interaction))
        {
            ButtonBackColor = Colors.ScrollbarClickedBackground;
            ButtonTextColor = Colors.ScrollbarClickedText;
            Text->Offset.Y += LineAdvance;
        }
        else
        {
            ButtonBackColor = Colors.ScrollbarHotBackground;
            ButtonTextColor = Colors.ScrollbarHotText;
        }
    }
    else
    {
        ButtonBackColor = Colors.ScrollbarBackground;
        ButtonTextColor = Colors.ScrollbarText;
    }
    PushRect(RenderGroup, ButtonP, ButtonDim, ButtonBackColor, ButtonBackColor);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, DownText, ButtonTextColor, Rectangle2(Offset, V2Add(Offset, Element->Dim)));
    
    v2 SliderP = V2Add(ButtonP, V2(Layout->Padding*0.25f, ButtonDim.Y));
    
    
    f32 Max = TextDim.Y;
    f32 Min = Element->Dim.Y - Layout->DefaultRowHeight - Layout->Padding;
    
    v2 SliderDim = V2Subtract(Dim, V2(Layout->Padding*0.5f, ButtonDim.Y*2.0f));
    f32 Calculated = ((Text->Offset.Y - Min) / (Max - Min));
    f32 SliderRemaining = SliderDim.Y*0.25f;
    SliderP.Y += (1.0f - Calculated)*SliderRemaining;
    SliderDim.Y *= 0.75f;
    
    // TODO(kstandbridge): This shouldn't be done at the drawing stage.
    if(IsInRectangle(Rectangle2(SliderP, V2Add(SliderP, SliderDim)), Layout->MouseP))
    {
        if(InteractionIsClicked(State, Element->Interaction))
        {
            SliderColor = Colors.ScrollbarClickedSlider;
            Text->Offset.Y -= Layout->dMouseP.Y;
        }
        else
        {
            SliderColor = Colors.ScrollbarHotSlider;
        }
    }
    else
    {
        SliderColor = Colors.ScrollbarSlider;
    }
    PushRect(RenderGroup, SliderP, SliderDim, SliderColor, SliderColor);
    
    TextP.Y = Dim.Y - ArrowDim.Y - Layout->Padding;
    ButtonP = V2(P.X, Dim.Y - Dim.X + Layout->Padding);
    ButtonDim = V2Set1(Dim.X);
    
    // TODO(kstandbridge): This shouldn't be done at the drawing stage.
    if(IsInRectangle(Rectangle2(ButtonP, V2Add(ButtonP, ButtonDim)), Layout->MouseP))
    {
        if(InteractionIsClicked(State, Element->Interaction))
        {
            ButtonBackColor = Colors.ScrollbarClickedBackground;
            ButtonTextColor = Colors.ScrollbarClickedText;
            Text->Offset.Y -= LineAdvance;
        }
        else
        {
            ButtonBackColor = Colors.ScrollbarHotBackground;
            ButtonTextColor = Colors.ScrollbarHotText;
        }
    }
    else
    {
        ButtonBackColor = Colors.ScrollbarBackground;
        ButtonTextColor = Colors.ScrollbarText;
    }
    PushRect(RenderGroup, ButtonP, ButtonDim, ButtonBackColor, ButtonBackColor);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, UpText, ButtonTextColor, Rectangle2(Offset, V2Add(Offset, Element->Dim)));
    
    // TODO(kstandbridge): This shouldn't be done at the drawing stage.
    if(InteractionIsHot(State, Element->Interaction))
    {
        Text->Offset.Y -= LineAdvance*(Layout->MouseZ*0.05f);
    }
    
    if(Text->Offset.Y > Max)
    {
        Text->Offset.Y = Max;
    }
    if(Text->Offset.Y < Min)
    {
        Text->Offset.Y = Min;
    }
    
}

inline void
DrawButton(ui_state *State, ui_layout *Layout, render_group *RenderGroup, ui_element *Element, v2 Offset)
{
    v4 BackgroundColor = Colors.ButtonBackground;
    v4 BorderColor = Colors.ButtonBorder;
    
    if(InteractionIsClicked(State, Element->Interaction))
    {
        BackgroundColor = Colors.ButtonClickedBackground;
        BorderColor = Colors.ButtonClickedBorder;
    }
    else if(InteractionIsHot(State, Element->Interaction))
    {
        BackgroundColor = Colors.ButtonHotBackground;
        BorderColor = Colors.ButtonHotBorder;
    }
    else if(InteractionIsSelected(State, Element->Interaction))
    {
        BackgroundColor = Colors.ButtonSelectedBackground;
        BorderColor = Colors.ButtonSelectedBorder;
    }
    
    v2 PaddedOffset = V2Add(Offset, V2Set1(Layout->Padding));
    PushRect(RenderGroup, PaddedOffset, V2Subtract(Element->Dim, V2Set1(Layout->Padding*2.0f)), 
             BackgroundColor, BackgroundColor);
    
    if(!InteractionIsSelected(State, Element->Interaction))
    {
        PushRectOutline(RenderGroup, PaddedOffset, V2Subtract(Element->Dim, V2Set1(Layout->Padding*2.0f)), 
                        BorderColor, BorderColor, Layout->Scale);
    }
    else
    {
        PushRectOutline(RenderGroup, PaddedOffset, V2Subtract(Element->Dim, V2Set1(Layout->Padding*2.0f)), 
                        Colors.ButtonSelectedBorder, Colors.ButtonSelectedBorder, Layout->Scale*3.0f);
        
        f32 Thickness = Layout->Scale*3.0f;
        if(Thickness < 1.0f)
        {
            Thickness = 1.0f;
        }
        v2 SelectedP = V2Add(PaddedOffset, V2Set1(Thickness*3.0f));
        v2 SelectedDim = V2Subtract(Element->Dim, V2Set1(Layout->Padding*2.5f));
        SelectedDim = V2Subtract(SelectedDim, V2Set1(Thickness*4.0f));
        PushRectOutline(RenderGroup, SelectedP, SelectedDim, Colors.ButtonSelectedBackground, Colors.ButtonSelectedBorderAlt, Layout->Scale);
    }
    
    f32 TextDim = Element->TextBounds.Max.X - Element->TextBounds.Min.X;
    v2 TextP = V2(Element->Dim.X*0.5f - TextDim*0.5f, Element->TextBounds.Max.Y*0.5f);
    TextP = V2Add(PaddedOffset, TextP);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.LabelText, Rectangle2(Offset, V2Add(Offset, Element->Dim)));
    
}

internal void
DrawElements(ui_state *State, ui_layout *Layout, render_group *RenderGroup, ui_element *FirstChild, v2 P)
{
    for(ui_element *Element = FirstChild;
        Element;
        Element = Element->Next)
    {
        if(Element->Type == Element_Row)
        {
            Assert(Element->FirstChild);
            
            f32 StartY = P.Y;
            f32 AdvanceX = Element->Dim.X;
            while(Element->Type == Element_Row)
            {
                DrawElements(State, Layout, RenderGroup, Element->FirstChild, P);
                P.Y += Element->FirstChild->Dim.Y;
                if(Element->Next && Element->Next->Type == Element_Row)
                {
                    Element = Element->Next;
                }
                else
                {
                    break;
                }
            }
            P.Y = StartY;
            P.X -= AdvanceX;
        }
        else
        {
            P.X -= Element->Dim.X;
            
            if(IsInRectangle(Rectangle2(P, V2Add(P, Element->Dim)), Layout->MouseP))
            {
                State->NextHotInteraction = Element->Interaction;
            }
            
            switch(Element->Type)
            {
                case Element_Row:
                {
                    InvalidCodePath;
                } break;
                
                case Element_Spacer:
                {
                    DrawSpacer(Layout, RenderGroup, Element, P);
                } break;
                
                case Element_Label:
                {
                    DrawLabel(Layout, RenderGroup, Element, P);
                } break;
                
                case Element_Checkbox:
                {
                    DrawCheckbox(State, Layout, RenderGroup, Element, P);
                } break;
                
                case Element_Textbox:
                {
                    DrawTextbox(State, Layout, RenderGroup, Element, V2Set1(0), P);
                } break;
                
                case Element_MultilineTextbox:
                {
                    DrawMultilineTextbox(State, Layout, RenderGroup, Element, P);
                } break;
                
                case Element_Button:
                {
                    DrawButton(State, Layout, RenderGroup, Element, P);
                } break;
                
                InvalidDefaultCase;
            }
        }
    }
}

internal void
CalculateElementDims(ui_layout *Layout, ui_element *FirstChild, s32 ChildCount, v2 Dim)
{
    b32 RowStart = false;
    v2 SubDim = Dim;
    
    for(ui_element *Element = FirstChild;
        Element;
        Element = Element->Next)
    {
        if(Element->Dim.X == 0)
        {
            Element->Dim.X = Dim.X / ChildCount;
        }
        
        Element->Dim.Y = Dim.Y;
        
        if(Element->Type != Element_Row)
        {
            if(Element->UsedDim.X > 0.0f)
            {
                Element->Dim.X = Element->UsedDim.X;
            }
            if(Element->Text.Data)
            {
                Element->TextBounds = GetTextSize(Layout->Assets, Layout->Scale, Element->Text);
            }
        }
        
        if(Element->Type == Element_Row)
        {
            if(!RowStart)
            {
                ui_element *FirstRow = Element;
                s32 RowCount = 0;
                s32 FullRowCount = 0;
                f32 UsedHeight = 0.0f;
                while(FirstRow != 0)
                {
                    if(FirstRow->Type != Element_Row)
                    {
                        break;
                    }
                    if(FirstRow->UsedDim.Y == 0.0f)
                    {
                        ++RowCount;
                    }
                    else
                    {
                        UsedHeight += FirstRow->UsedDim.Y;
                    }
                    ++FullRowCount;
                    FirstRow = FirstRow->Next;
                }
                SubDim = V2(Element->Dim.X, (Element->Dim.Y - UsedHeight) / RowCount);
                
                RowStart = true;
            }
            
            
            s32 SubChildCount = (Element->ChildCount - Element->SetWidthChildCount);
            v2 SubDimRemaining = V2(SubDim.X - Element->UsedDim.X, SubDim.Y);
            if(Element->UsedDim.Y > 0.0f)
            {
                SubDimRemaining.Y = Element->UsedDim.Y;
            }
            
            CalculateElementDims(Layout, Element->FirstChild, SubChildCount, SubDimRemaining);
        }
        else
        {
            RowStart = false;
        }
    }
}

internal void
EndUIFrame(ui_state *State, ui_layout *Layout, app_input *Input)
{
    
    // NOTE(kstandbridge): Handling interactions
    {    
        // NOTE(kstandbridge): Input text
        ui_interaction SelectedInteraction = State->SelectedInteraction;
        if(Input->Text[0] != '\0')
        {
            if((SelectedInteraction.Type == Interaction_EditableText) ||
               (SelectedInteraction.Type == Interaction_EditableMultilineText))
            {
                editable_string *Text = SelectedInteraction.Text;
                
                char *At = Input->Text;
                while(*At != '\0')
                {
                    if(Text->Length < Text->Size)
                    {
                        ++Text->Length;
                        umm Index = Text->Length;
                        while(Index > Text->SelectionStart)
                        {
                            Text->Data[Index] = Text->Data[Index - 1];
                            --Index;
                        }
                        Text->Data[Text->SelectionStart++] = *At;
                        Text->SelectionEnd = Text->SelectionStart;
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
                    if((SelectedInteraction.Type == Interaction_EditableText) ||
                       (SelectedInteraction.Type == Interaction_EditableMultilineText))
                    {
                        editable_string *Text = SelectedInteraction.Text;
                        switch(Type)
                        {
                            case KeyboardButton_Delete:
                            {
                                if(Text->SelectionStart < Text->Length)
                                {
                                    Text->SelectionStart++;
                                    Text->SelectionEnd = Text->SelectionStart;
                                }
                                else
                                {
                                    break;
                                }
                            } // NOTE(kstandbridge): No break intentional
                            case KeyboardButton_Backspace:
                            {
                                if(Text->Length > 0)
                                {
                                    umm StartMoveIndex = Text->SelectionStart--;
                                    while(StartMoveIndex < Text->Length)
                                    {
                                        Text->Data[StartMoveIndex - 1] = Text->Data[StartMoveIndex++];
                                    }
                                    Text->Data[--Text->Length] = '\0';
                                    Text->SelectionEnd = Text->SelectionStart;
                                }
                            } break;
                            case KeyboardButton_Right:
                            {
                                if(Text->SelectionStart < Text->Length)
                                {
                                    ++Text->SelectionStart;
                                }
                                if(!Input->ShiftDown)
                                {
                                    Text->SelectionEnd = Text->SelectionStart;
                                }
                            } break;
                            case KeyboardButton_Left:
                            {
                                if(Text->SelectionStart > 0)
                                {
                                    --Text->SelectionStart;
                                }
                                if(!Input->ShiftDown)
                                {
                                    Text->SelectionEnd = Text->SelectionStart;
                                }
                            } break;
                            
                            case KeyboardButton_Home:
                            {
                                if(Input->ControlDown)
                                {
                                    Text->SelectionStart = 0;
                                    Text->SelectionEnd = 0;
                                    Text->Offset.Y = 0.0f;
                                }
                                else
                                {
                                    u32 Index = Text->SelectionEnd;
                                    while(Index > 0 && Text->Data[Index] != '\n')
                                    {
                                        --Index;
                                    }
                                    if(Text->Data[Index] == '\n')
                                    {
                                        ++Index;
                                    }
                                    Text->SelectionStart = Index;
                                    Text->SelectionEnd = Index;
                                }
                            } break;
                            
                            case KeyboardButton_End:
                            {
                                if(Input->ControlDown)
                                {
                                    Text->SelectionStart = Text->Length;
                                    Text->SelectionEnd = Text->Length;
                                    Text->Offset.Y = F32Max;
                                }
                                else
                                {
                                    u32 Index = Text->SelectionEnd;
                                    while(Index < Text->Length && Text->Data[Index] != '\n')
                                    {
                                        ++Index;
                                    }
                                    Text->SelectionStart = Index;
                                    Text->SelectionEnd = Index;
                                }
                            } break;
                            
                            
                            case KeyboardButton_A:
                            {
                                if(Input->ControlDown)
                                {
                                    Text->SelectionStart = 0;
                                    Text->SelectionEnd = Text->Length;
                                }
                            } break;
                            
                            case KeyboardButton_C:
                            {
                                if(Input->ControlDown)
                                {
                                    s32 SelectedCharacters = Text->SelectionStart - Text->SelectionEnd;
                                    s32 TotalCharaceters = SelectedCharacters;
                                    if(TotalCharaceters < 0)
                                    {
                                        TotalCharaceters *= -1;
                                    }
                                    s32 StartOfSelection;
                                    if(Text->SelectionEnd > Text->SelectionStart)
                                    {
                                        StartOfSelection = Text->SelectionEnd + SelectedCharacters;
                                    }
                                    else
                                    {
                                        StartOfSelection = Text->SelectionEnd;
                                    }
                                    
                                    string SelectedText = StringInternal(TotalCharaceters, Text->Data + StartOfSelection);
                                    
                                    Platform.SetClipboardText(SelectedText);
                                }
                            } break;
                            
                        }
                    }
                }
                KeyboardButton = !KeyboardButton;
            }
        }
        
        // NOTE(kstandbridge): Mouse buttons
        // TODO(kstandbridge): Is mouse wheel an interaction?
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
                State->SelectedInteraction = State->HotInteraction;
                State->ClickedInteraction = State->HotInteraction;
            }
            
            switch(State->Interaction.Type)
            {
                
                case Interaction_ImmediateButton:
                {
                    if(MouseUp)
                    {
                        State->NextToExecute = State->Interaction;
                        EndInteraction = true;
                    }
                } break;
                
                case Interaction_EditableBool:
                {
                    if(MouseUp)
                    {
                        *State->Interaction.Bool = !*State->Interaction.Bool;
                        EndInteraction = true;
                    }
                } break;
                
                case Interaction_None:
                {
                    State->HotInteraction = State->NextHotInteraction;
                    if(MouseDown)
                    {
                        State->Interaction = State->NextHotInteraction;
                    }
                } break;
                
#if 0                
                case UiInteraction_MultipleChoiceOption:
                {
                    if(MouseUp)
                    {
                        Layout->State->NextToExecute = Layout->State->Interaction;
                        EndInteraction = true;
                    }
                } break;
#endif
                
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
                ClearInteraction(&State->Interaction);
                ClearInteraction(&State->ClickedInteraction);
            }
            
            MouseButton = !MouseButton;
        }
        
        ClearInteraction(&State->NextHotInteraction);
        
        
        State->LastMouseP = Layout->MouseP;
    }
    
    // NOTE(kstandbridge): Figure out the sizes ready for drawing
    {    
        v2 Dim = V2((f32)Layout->DrawBuffer->Width, (f32)Layout->DrawBuffer->Height);
        ui_element *Element = &Layout->SentinalElement;
        
        s32 ChildCount = Element->ChildCount - Element->SetWidthChildCount;
        v2 DimRemaining = V2(Dim.X - Element->UsedDim.X, Dim.Y);
        
        CalculateElementDims(Layout, Element->FirstChild, ChildCount, DimRemaining);
    }
    
    // NOTE(kstandbridge): Draw all the elements
    {    
        ColArrayIndex = 0;
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
        ColArray[15] = V4(0.0f, 0.0f, 0.75f, 1.0f);
        ColArray[16] = V4(0.0f, 0.75f, 0.0f, 1.0f);
        ColArray[17] = V4(0.0f, 0.75f, 0.75f, 1.0f);
        ColArray[18] = V4(0.75f, 0.0f, 0.0f, 1.0f);
        ColArray[19] = V4(0.75f, 0.0f, 0.75f, 1.0f);
        ColArray[20] = V4(0.0f, 0.0f, 0.25f, 1.0f);
        ColArray[21] = V4(0.0f, 0.25f, 0.0f, 1.0f);
        ColArray[22] = V4(0.0f, 0.25f, 0.25f, 1.0f);
        ColArray[23] = V4(0.25f, 0.0f, 0.0f, 1.0f);
        ColArray[24] = V4(0.25f, 0.0f, 0.25f, 1.0f);
        
        render_group *RenderGroup = AllocateRenderGroup(Layout->Arena, Megabytes(4), Layout->DrawBuffer);
        
        PushClear(RenderGroup, Colors.Clear);
        
        v2 P = V2((f32)Layout->DrawBuffer->Width, 0.0f);
        DrawElements(State, Layout, RenderGroup, Layout->SentinalElement.FirstChild, P);
        
        RenderGroupToOutput(RenderGroup);
        
    }
    
}

inline void
SetRowHeight(ui_layout *Layout, f32 Height)
{
    ui_element *Row = Layout->CurrentElement;
    Assert(Row->Type == Element_Row);
    if(Row->Type == Element_Row)
    {
        Row->UsedDim.Y = Height;
    }
}

inline void
SetRowFill(ui_layout *Layout)
{
    ui_element *Row = Layout->CurrentElement;
    Assert(Row->Type == Element_Row);
    if(Row->Type == Element_Row)
    {
        Row->UsedDim.Y = 0.0f;
    }
}
inline void
SetControlWidth(ui_layout *Layout, f32 Width)
{
    Assert(Width > 0.0f);
    if(Width > 0.0f)
    {
        if(Layout->CurrentElement->FirstChild->UsedDim.X == 0.0f)
        {
            ++Layout->CurrentElement->SetWidthChildCount;
        }
        
        Layout->CurrentElement->FirstChild->UsedDim.X = Width;
        Layout->CurrentElement->UsedDim.X += Width;
    }
}

#define Splitter(Layout, ...) Spacer(Layout)
#define DropDown(Layout, ...) Spacer(Layout)

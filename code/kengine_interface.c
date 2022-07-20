
inline void
SetRowWidth(layout *Layout, f32 Width)
{
    Layout->CurrentElement->Parent->SetWidthChildCount = 1;
    Layout->CurrentElement->Parent->UsedDim.X = Width;
    Layout->CurrentElement->Dim.X = Width;
}

inline void
BeginRow(layout *Layout)
{
    element *Element = PushStruct(Layout->Arena, element);
    ZeroStruct(*Element);
    Element->UsedDim.Y = Layout->DefaultRowHeight + Layout->DefaultPadding*2.0f;
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
EndRow(layout *Layout)
{
    Layout->CurrentElement = Layout->CurrentElement->Parent;
    
}

inline element *
PushElement(layout *Layout)
{
    element *Result = PushStruct(Layout->Arena, element);
    ZeroStruct(*Result);
    Result->Padding.Top = Layout->DefaultPadding;
    Result->Padding.Right = Layout->DefaultPadding;
    Result->Padding.Bottom = Layout->DefaultPadding;
    Result->Padding.Left = Layout->DefaultPadding;
    Result->Next = Layout->CurrentElement->FirstChild;
    Layout->CurrentElement->FirstChild = Result;
    
    ++Layout->CurrentElement->ChildCount;
    
    return Result;
}

inline void
Spacer(layout *Layout)
{
    element *Element = PushElement(Layout);
    Element->Type = Element_Spacer;
}

inline void
Label(layout *Layout, string Text)
{
    element *Element = PushElement(Layout);
    Element->Type = Element_Label;
    Element->Text = Text;
}

inline void
Checkbox(layout *Layout, string Text, b32 *Target)
{
    element *Element = PushElement(Layout);
    Element->Type = Element_Checkbox;
    Element->Text = Text;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = InterfaceId(++Layout->CurrentId);
    Element->Interaction.Type = Interaction_EditableBool;
    Element->Interaction.Generic = Target;
}

inline void
Textbox(layout *Layout, editable_string *Target)
{
    element *Element = PushElement(Layout);
    Element->Type = Element_Textbox;
    Element->Text = StringInternal(Target->Length, Target->Data);;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = InterfaceId(++Layout->CurrentId);
    Element->Interaction.Type = Interaction_Textbox;
    Element->Interaction.Generic = Target;
}

inline void
MultilineTextbox(layout *Layout, editable_string *Target)
{
    element *Element = PushElement(Layout);
    Element->Text = StringInternal(Target->Length, Target->Data);;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = InterfaceId(++Layout->CurrentId);
    Element->Interaction.Type = Interaction_Textbox;
    Element->Interaction.Generic = Target;
}

inline b32
Button(interface_state *State, layout *Layout, string Text)
{
    element *Element = PushElement(Layout);
    Element->Type = Element_Button;
    Element->Text = Text;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = InterfaceId(++Layout->CurrentId);
    Element->Interaction.Type = Interaction_ImmediateButton;
    
    b32 Result = InteractionsAreEqual(Element->Interaction, State->ToExecute);
    return Result;
}

inline b32
ContinousButton(interface_state *State, layout *Layout, string Text)
{
    element *Element = PushElement(Layout);
    Element->Type = Element_Button;
    Element->Text = Text;
    
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = InterfaceId(++Layout->CurrentId);
    Element->Interaction.Type = Interaction_ContinousButton;
    
    b32 Result = InteractionsAreEqual(Element->Interaction, State->ClickedInteraction);
    return Result;
}

inline void
VerticleSlider(layout *Layout, f32 *Target)
{
    element *Element = PushElement(Layout);
    Element->Type = Element_VerticleSlider;
    ZeroStruct(Element->Interaction);
    Element->Interaction.ID = InterfaceId(++Layout->CurrentId);
    Element->Interaction.Type = Interaction_EditableFloat;
    Element->Interaction.Float = Target;
}

inline layout
BeginUIFrame(memory_arena *Arena, interface_state *State, app_input *Input, assets *Assets, f32 Scale, f32 DefaultPadding, app_render_commands *RenderCommands)
{
    layout Result;
    ZeroStruct(Result);
    
    Result.Arena = Arena;
    Result.Assets = Assets;
    Result.Scale = Scale;
    Result.DefaultPadding = DefaultPadding;
    Result.DefaultRowHeight = Platform.DEBUGGetLineAdvance()*Result.Scale;
    
    Result.RenderCommands = RenderCommands;
    
    Result.SentinalElement.Dim = V2((f32)Result.RenderCommands->Width, (f32)Result.RenderCommands->Height);
    Result.CurrentId = 0;
    
    Result.MouseP = V2(Input->MouseX, Input->MouseY);
    Result.dMouseP = V2Subtract(Result.MouseP, State->LastMouseP);
    Result.MouseZ = Input->MouseZ;
    Result.DeltaTime = Input->dtForFrame;
    
    State->ToExecute = State->NextToExecute;
    ClearInteraction(&State->NextToExecute);
    
    return Result;
}

// TODO(kstandbridge): Ditch this crap
global s32 ColArrayIndex;
global v4 ColArray[25];

inline void
DrawSpacer(render_group *RenderGroup, element *Element, v2 Offset)
{
    element_padding *Padding = &Element->Padding;
    PushRect(RenderGroup, 
             V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
             V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
             ColArray[ColArrayIndex], ColArray[ColArrayIndex], 0.0f);
    ColArrayIndex = (ColArrayIndex + 1) % ArrayCount(ColArray);
}

inline void
DrawLabel(layout *Layout, render_group *RenderGroup, element *Element, v2 Offset)
{
    element_padding *Padding = &Element->Padding;
    PushRect(RenderGroup, 
             V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
             V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
             Colors.LabelBackground, Colors.LabelBackground, 0.0f);
    
    v2 TextP = V2(Padding->Left, Padding->Bottom + Element->TextBounds.Max.Y*0.5f);
    TextP = V2Add(TextP, Offset);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.LabelText, Rectangle2(Offset, V2Add(Offset, Element->Dim)), 1.0f);
    //PushText(RenderGroup, TextP, Colors.LabelText, Element->Text, 1.0f);
}

inline void
DrawCheckbox(interface_state *State, layout *Layout, render_group *RenderGroup, element *Element, v2 Offset)
{
    element_padding *Padding = &Element->Padding;
    v2 CheckboxP = V2(Padding->Left, Padding->Bottom*2.0f);
    CheckboxP = V2Add(CheckboxP, Offset);
    v2 CheckboxDim = V2Set1(Platform.DEBUGGetLineAdvance()*Layout->Scale*1.0f);
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
    
    PushRect(RenderGroup, CheckboxP, CheckboxDim, CheckboxBackground, CheckboxBackground, 0.0f);
    PushRectOutline(RenderGroup, CheckboxP, CheckboxDim, CheckboxBorder, CheckboxBorder, Layout->Scale*0.25f, 1.0f);
    if(CheckText.Size > 0)
    {
        v2 CheckTextP = V2Add(CheckboxP, V2Multiply(CheckboxDim, V2Set1(0.5f)));
        rectangle2 CheckTextRect = GetTextSize(Layout->Assets, Layout->Scale, CheckText);
        v2 CheckTextDim = V2Subtract(CheckTextRect.Max, CheckTextRect.Min);
        CheckTextP = V2Subtract(CheckTextP, V2Multiply(CheckTextDim, V2Set1(0.5f)));
        WriteLine(RenderGroup, Layout->Assets, CheckTextP, Layout->Scale, CheckText, CheckboxText, Rectangle2(Offset, V2Add(Offset, Element->Dim)), 2.0f);
    }
    
    v2 TextP = V2(Padding->Left, Padding->Bottom + Element->TextBounds.Max.Y*0.5f);
    TextP.X += CheckboxDim.X + Padding->Left + Padding->Right;
    TextP = V2Add(TextP, Offset);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.LabelText, Rectangle2(Offset, V2Add(Offset, Element->Dim)), 1.0f);
}

inline void
DrawTextbox(interface_state *State, layout *Layout, render_group *RenderGroup, element *Element, v2 Offset)
{
    element_padding *Padding = &Element->Padding;
    
    v2 TextOffset = Element->Interaction.Text->Offset;
    
    PushRect(RenderGroup, 
             V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
             V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
             Colors.TextboxBackground, Colors.TextboxBackground, 0.0f);
    
    v4 TextboxBorderColor = Colors.TextboxBorder;
    if(InteractionIsSelected(State, Element->Interaction))
    {
        TextboxBorderColor = Colors.TextboxSelectedBorder;
    }
    
    v2 TextP = V2((Padding->Left + Padding->Right)*2.0f, (Padding->Top + Padding->Bottom) + Element->TextBounds.Max.Y*0.5f);
    TextP = V2Add(TextP, Offset);
    TextP = V2Add(TextP, TextOffset);
    
    v2 TextClipMin = V2Add(Offset, V2((Padding->Left + Padding->Right)*2.0f, (Padding->Top + Padding->Bottom)*2.0f));
    v2 TextClipMax = V2Add(TextClipMin, V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right)*7.0f, (Padding->Top + Padding->Bottom)*7.0f)));
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
                          SelectionStart, SelectionEnd, CaretHeight, TextClip, 1.0f);
    }
    else
    {
        //PushText(RenderGroup, TextP, Colors.TextboxText, Element->Text, 4.0f);
        WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.TextboxText, TextClip, 1.0f);
    }
    
    PushRectOutline(RenderGroup, 
                    V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
                    V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
                    TextboxBorderColor, TextboxBorderColor, Layout->Scale, 2.0f);
}

inline void
DrawMultilineTextboxDeprecated(interface_state *State, layout *Layout, render_group *RenderGroup, element *Element, v2 Offset)
{
    element_padding *Padding = &Element->Padding;
    
    editable_string *Text = Element->Interaction.Text;
    DrawTextbox(State, Layout, RenderGroup, Element, Offset);
    
    string UpText = String("\\02C4");
    string DownText = String("\\02C5");
    rectangle2 TextBounds = GetTextSize(Layout->Assets, Layout->Scale, UpText);
    v2 ArrowDim = V2Subtract(TextBounds.Max, TextBounds.Min);
    
    v2 TextDim = V2Subtract(Element->TextBounds.Max, Element->TextBounds.Min);
    f32 LineAdvance = Layout->DefaultRowHeight*3.0f*Layout->DeltaTime;
    
    v2 Dim = V2(ArrowDim.X + (Padding->Left + Padding->Right)*2.0f, Element->Dim.Y - (Padding->Top + Padding->Bottom)*2.0f - Layout->Scale*6.0f);
    v2 P = V2(Element->Dim.X - (Padding->Left + Padding->Right) - Layout->Scale*3.0f - Dim.X, (Padding->Top + Padding->Bottom) + Layout->Scale*3.0f);
    P = V2Add(P, Offset);
    PushRect(RenderGroup, P, Dim, Colors.ScrollbarBackground, Colors.ScrollbarBackground, 0.0f);
    
    v4 ButtonBackColor;
    v4 ButtonTextColor;
    v4 SliderColor;
    
    v2 TextP = V2(P.X + Dim.X*0.5f - ArrowDim.X*0.5f - (Padding->Left + Padding->Right)*0.25f, P.Y + (Padding->Top + Padding->Bottom)*0.5f);
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
    PushRect(RenderGroup, ButtonP, ButtonDim, ButtonBackColor, ButtonBackColor, 1.0f);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, DownText, ButtonTextColor, Rectangle2(Offset, V2Add(Offset, Element->Dim)), 2.0f);
    
    v2 SliderP = V2Add(ButtonP, V2((Padding->Left + Padding->Right)*0.25f, ButtonDim.Y));
    
    
    f32 Max = TextDim.Y;
    f32 Min = Element->Dim.Y - Layout->DefaultRowHeight - (Padding->Top + Padding->Bottom);
    
    v2 SliderDim = V2Subtract(Dim, V2((Padding->Left + Padding->Right)*0.5f, ButtonDim.Y*2.0f));
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
    PushRect(RenderGroup, SliderP, SliderDim, SliderColor, SliderColor, 3.0f);
    
    TextP.Y = Dim.Y - ArrowDim.Y - (Padding->Top + Padding->Bottom);
    ButtonP = V2(P.X, Dim.Y - Dim.X + (Padding->Top + Padding->Bottom));
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
    PushRect(RenderGroup, ButtonP, ButtonDim, ButtonBackColor, ButtonBackColor, 4.0f);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, UpText, ButtonTextColor, Rectangle2(Offset, V2Add(Offset, Element->Dim)), 5.0f);
    
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
DrawButton(interface_state *State, layout *Layout, render_group *RenderGroup, element *Element, v2 Offset)
{
    Layout;
    element_padding *Padding = &Element->Padding;
    
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
    
    PushRect(RenderGroup, 
             V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
             V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
             BackgroundColor, BackgroundColor, 0.0f);
    
    if(!InteractionIsSelected(State, Element->Interaction))
    {
        PushRectOutline(RenderGroup, 
                        V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
                        V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
                        BorderColor, BorderColor, Layout->Scale*0.25f, 1.0f);
    }
    else
    {
        PushRectOutline(RenderGroup, 
                        V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
                        V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
                        Colors.ButtonSelectedBorder, Colors.ButtonSelectedBorder, Layout->Scale*0.25f, 1.0f);
        
        f32 Thickness = Layout->Scale*3.0f;
        if(Thickness < 1.0f)
        {
            Thickness = 1.0f;
        }
        v2 SelectedP = V2Add(V2Add(Offset, V2(Padding->Left, Padding->Bottom)), V2Set1(Thickness*3.0f));
        v2 SelectedDim = V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right)*2.5f, (Padding->Top + Padding->Bottom)*2.5f));
        SelectedDim = V2Subtract(SelectedDim, V2Set1(Thickness*4.0f));
        PushRectOutline(RenderGroup, SelectedP, SelectedDim, Colors.ButtonSelectedBackground, Colors.ButtonSelectedBorderAlt, Layout->Scale, 2.0f);
    }
    
    v2 TextDim = V2Subtract(Element->TextBounds.Max, Element->TextBounds.Min);
    v2 TextP = V2(Element->Dim.X*0.5f - TextDim.X*0.5f, Element->Dim.Y*0.5f - TextDim.Y*0.5f);
    TextP = V2Add(TextP, Offset);
    WriteLine(RenderGroup, Layout->Assets, TextP, Layout->Scale, Element->Text, Colors.LabelText, Rectangle2(Offset, V2Add(Offset, Element->Dim)), 3.0f);
    //PushText(RenderGroup, TextP, Colors.LabelText, Element->Text, 3.0f);
    
}

inline void
DrawSlider(interface_state *State, layout *Layout, render_group *RenderGroup, element *Element, v2 Offset)
{
    State; Layout;
    
    element_padding *Padding = &Element->Padding;
    PushRect(RenderGroup, 
             V2Add(Offset, V2(Padding->Left, Padding->Bottom)), 
             V2Subtract(Element->Dim, V2((Padding->Left + Padding->Right), (Padding->Top + Padding->Bottom))), 
             ColArray[ColArrayIndex], ColArray[ColArrayIndex], 0.0f);
    ColArrayIndex = (ColArrayIndex + 1) % ArrayCount(ColArray);
}

internal void
DrawElements(interface_state *State, layout *Layout, render_group *RenderGroup, element *FirstChild, v2 P)
{
    for(element *Element = FirstChild;
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
                    DrawSpacer(RenderGroup, Element, P);
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
                    DrawTextbox(State, Layout, RenderGroup, Element, P);
                } break;
                
                case Element_Button:
                {
                    DrawButton(State, Layout, RenderGroup, Element, P);
                } break;
                
                case Element_VerticleSlider:
                {
                    DrawSlider(State, Layout, RenderGroup, Element, P);
                } break;
                
                InvalidDefaultCase;
            }
        }
    }
}

internal void
CalculateElementDims(layout *Layout, element *FirstChild, s32 ChildCount, v2 Dim)
{
    b32 RowStart = false;
    v2 SubDim = Dim;
    
    for(element *Element = FirstChild;
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
                element *FirstRow = Element;
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
KeyboardInteract(interface_state *State, layout *Layout, app_input *Input)
{
    Layout;
    if(Input->Text[0] != '\0')
    {
        if(State->SelectedInteraction.Type == Interaction_Textbox)
        {
            editable_string *Text = State->SelectedInteraction.Text;
            
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
                if(State->SelectedInteraction.Type == Interaction_Textbox)
                {
                    editable_string *Text = State->SelectedInteraction.Text;
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
}

internal void
MouseMoveInteract(interface_state *State, layout *Layout, app_input *Input)
{
    Input;
    switch(State->Interaction.Type)
    {
        
        // TODO(kstandbridge): Drag
        // TODO(kstandbridge): Resize
        // TODO(kstandbridge): Move
        
        case Interaction_EditableFloat:
        {
            *State->Interaction.Float += Layout->dMouseP.Y;
        } break;
        
        case Interaction_TextboxSelecting:
        {
            DEBUGTextLine("SetEndTextFromPos( %.03f / %.03f )", Layout->MouseP.X, Layout->MouseP.Y);
        } break;
    }
}

internal void
MouseButtonInteract(interface_state *State, layout *Layout, app_input *Input)
{
    Layout;
    u32 HalfTransitionCount = Input->MouseButtons[MouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[MouseButton_Left].EndedDown;
    if(HalfTransitionCount % 2)
    {
        MouseButton = !MouseButton;
    }
    
    for(u32 TransitionIndex = 0;
        TransitionIndex <= HalfTransitionCount;
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
        
        if(MouseDown)
        {
            State->ClickedInteraction = State->HotInteraction;
            State->SelectedInteraction = State->HotInteraction;
        }
        
        switch(State->Interaction.Type)
        {
            case Interaction_EditableBool:
            {
                if(MouseUp)
                {
                    *State->Interaction.Bool = !*State->Interaction.Bool;
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_Textbox:
            {
                State->Interaction.Type = Interaction_TextboxSelecting;
            } break;
            
            case Interaction_TextboxSelecting:
            {
                if(MouseUp)
                {
                    State->Interaction.Type = Interaction_Textbox;
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_ImmediateButton:
            {
                if(MouseUp)
                {
                    State->NextToExecute = State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_None:
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
        
        if(EndInteraction)
        {
            ClearInteraction(&State->ClickedInteraction);
            ClearInteraction(&State->Interaction);
        }
        
        MouseButton = !MouseButton;
    }
}

internal void
Interact(interface_state *State, layout *Layout, app_input *Input)
{
    KeyboardInteract(State, Layout, Input);
    MouseMoveInteract(State, Layout, Input);
    if(IsInRectangle(Rectangle2(V2Set1(0.0f), V2((f32)Layout->RenderCommands->Width, (f32)Layout->RenderCommands->Height)), Layout->MouseP))
    {
        MouseButtonInteract(State, Layout, Input);
    }
}

internal void
EndUIFrame(interface_state *State, layout *Layout, app_input *Input)
{
    // NOTE(kstandbridge): Figure out the sizes ready for drawing
    {    
        v2 Dim = V2((f32)Layout->RenderCommands->Width, (f32)Layout->RenderCommands->Height);
        element *Element = &Layout->SentinalElement;
        
        s32 ChildCount = Element->ChildCount - Element->SetWidthChildCount;
        v2 DimRemaining = V2(Dim.X - Element->UsedDim.X, Dim.Y);
        
        DimRemaining.X -= Layout->DefaultPadding*2.0f;
        DimRemaining.Y -= Layout->DefaultPadding*2.0f;
        
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
        
        render_group RenderGroup = BeginRenderGroup(Layout->Assets, Layout->RenderCommands);
        
        PushClear(&RenderGroup, Colors.Clear);
        
        v2 P = V2((f32)Layout->RenderCommands->Width - Layout->DefaultPadding, Layout->DefaultPadding);
        DrawElements(State, Layout, &RenderGroup, Layout->SentinalElement.FirstChild, P);
        
        EndRenderGroup(&RenderGroup);
        
    }
    
    Interact(State, Layout, Input);
    State->LastMouseP = Layout->MouseP;
}

inline void
SetRowHeight(layout *Layout, f32 Height)
{
    element *Row = Layout->CurrentElement;
    Assert(Row->Type == Element_Row);
    if(Row->Type == Element_Row)
    {
        Row->UsedDim.Y = Height;
    }
}

inline void
SetRowFill(layout *Layout)
{
    element *Row = Layout->CurrentElement;
    Assert(Row->Type == Element_Row);
    if(Row->Type == Element_Row)
    {
        Row->UsedDim.Y = 0.0f;
    }
}
inline void
SetControlWidth(layout *Layout, f32 Width)
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

inline void
SetControlPadding(layout *Layout, f32 Top, f32 Right, f32 Bottom, f32 Left)
{
    Layout->CurrentElement->FirstChild->Padding.Top = Top;
    Layout->CurrentElement->FirstChild->Padding.Right = Right;
    Layout->CurrentElement->FirstChild->Padding.Bottom = Bottom;
    Layout->CurrentElement->FirstChild->Padding.Left = Left;
}

#define Splitter(Layout, ...) Spacer(Layout)
#define DropDown(Layout, ...) Spacer(Layout)

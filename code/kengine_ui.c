
internal void
TextElement(ui_layout *Layout, v2 P, string Str, ui_interaction Interaction, f32 TextOffset, f32 Padding, f32 Scale)
{
    rectangle2 Bounds = TextOpInternal(TextOp_SizeText, Layout->RenderGroup, Layout->State->Assets, P, Scale, Str);
    
    v2 Dim = V2(Bounds.Max.X - Bounds.Min.X, 
                Bounds.Max.Y - Bounds.Min.Y);
    Dim.Y += Padding;
    
    b32 IsHot = InteractionIsHot(Layout->State, Interaction);
    
    v4 ButtonColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 0, 0, 1);
    
    PushRect(Layout->RenderGroup, P, Dim, ButtonColor);
    WriteLine(Layout->RenderGroup, Layout->State->Assets, V2Subtract(P, V2(0.0f, TextOffset)), Scale, Str);
    
    if(IsInRectangle(Bounds, Layout->MouseP))
    {
        Layout->State->NextHotInteraction = Interaction;
    }
}

internal ui_layout
BeginUIFrame(ui_state *UiState, memory_arena *Arena, render_group *RenderGroup, app_input *Input, f32 Scale)
{
    ui_layout Result;
    ZeroStruct(Result);
    
    Result.State = UiState;
    Result.Arena = Arena;
    Result.RenderGroup = RenderGroup;
    
    Result.Scale = Scale;
    Result.MouseP = V2(Input->MouseX, Input->MouseY);
    Result.dMouseP = V2Subtract(Result.MouseP, UiState->LastMouseP);
    
    UiState->ToExecute = UiState->NextToExecute;
    ClearInteraction(&UiState->NextToExecute);
    
    return Result;
}

internal void
HandleUIInteractionsInternal(ui_layout *Layout, app_input *Input)
{
    u32 TransitionCount = Input->MouseButtons[AppInputMouseButton_Left].HalfTransitionCount;
    b32 MouseButton = Input->MouseButtons[AppInputMouseButton_Left].EndedDown;
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
        
        switch(Layout->State->Interaction.Type)
        {
            case UiInteraction_ImmediateButton:
            case UiInteraction_Draggable:
            {
                if(MouseUp)
                {
                    Layout->State->NextToExecute = Layout->State->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            // TODO(kstandbridge): InteractionSelect that could choose a textbox or row in list
            
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
        }
        
        MouseButton = !MouseButton;
    }
    
    ClearInteraction(&Layout->State->NextHotInteraction);
}

internal void
DrawUIInternal(ui_layout *Layout)
{
    f32 CurrentY = 0.0f;
    
    s32 RowCount = 0;
    s32 FillRows = 0;
    f32 UsedHeight = 0.0f;
    for(ui_layout_row *Row = Layout->LastRow;
        Row;
        Row = Row->Next)
    {
        // TODO(kstandbridge): This could be moved to EndRow?
        ++RowCount;
        if(Row->Type == LayoutType_Fill)
        {
            ++FillRows;
        }
        else
        {
            Assert(Row->Type == LayoutType_Auto);
        }
        
        {
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
                    rectangle2 TextBounds = GetTextSize(Layout->RenderGroup, Layout->State->Assets, V2(0, 0), Layout->Scale, Element->Label);
                    Element->TextOffset = TextBounds.Min.Y;
                    Element->Dim = V2(TextBounds.Max.X - TextBounds.Min.X, 
                                      TextBounds.Max.Y - TextBounds.Min.Y);
                    
                    Row->UsedWidth += Element->Dim.X;
                    if(Element->Dim.Y > Row->MaxHeight)
                    {
                        Row->MaxHeight = Element->Dim.Y;
                    }
                }
            }
            if(Row->Type == LayoutType_Auto)
            {
                UsedHeight += Row->MaxHeight;
            }
        }
    }
    
    f32 TotalHeight = (f32)Layout->RenderGroup->OutputTarget->Height;
    f32 RemainingHeight = TotalHeight - UsedHeight;
    f32 HeightPerFill = RemainingHeight / FillRows;
    for(ui_layout_row *Row = Layout->LastRow;
        Row;
        Row = Row->Next)
    {
        f32 TotalWidth = (f32)Layout->RenderGroup->OutputTarget->Width;
        f32 RemainingWidth = TotalWidth - Row->UsedWidth;
        f32 WidthPerSpacer = RemainingWidth / Row->SpacerCount;
        v2 P = V2(TotalWidth, CurrentY);
        if(Row->Type == LayoutType_Fill)
        {
            P.Y += (0.5f*HeightPerFill) - (0.5f*Row->MaxHeight);
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
                P.X -= WidthPerSpacer;
            }
            else
            {
                P.X -= Element->Dim.X;
                f32 HeightDifference = (Row->MaxHeight - Element->Dim.Y);
                TextElement(Layout, P, Element->Label, Element->Interaction, Element->TextOffset - HeightDifference, HeightDifference, Layout->Scale);
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
    HandleUIInteractionsInternal(Layout, Input);
    
    Layout->State->LastMouseP = Layout->MouseP;
    
    DrawUIInternal(Layout);
}

inline void
PushRow(ui_layout *Layout, ui_layout_type Type)
{
    ui_layout_row *Row = PushStruct(Layout->Arena, ui_layout_row);
    ZeroStruct(*Row);
    Row->Next = Layout->LastRow;
    Layout->LastRow = Row;
    
    Row->Type = Type;
}

inline ui_element *
PushElementInternal(ui_layout *Layout, ui_element_type ElementType, ui_element_type InteractionType, u32 ID, string Str, void *Target)
{
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

inline b32
PushButtonElement(ui_layout *Layout, u32 ID, string Str)
{
    ui_element *Element = PushElementInternal(Layout, ElementType_Button, UiInteraction_ImmediateButton, ID, Str, 0);
    
    b32 Result = InteractionsAreEqual(Element->Interaction, Layout->State->ToExecute);
    return Result;
}

inline void
PushScrollElement(ui_layout *Layout, u32 ID, string Str, v2 *TargetP)
{
    ui_element *Element = PushElementInternal(Layout, ElementType_Slider, UiInteraction_Draggable, ID, Str, TargetP);
    
    if(InteractionsAreEqual(Element->Interaction, Layout->State->Interaction))
    {
        v2 *P = Layout->State->Interaction.P;
        *P = V2Add(*P, Layout->dMouseP);
    }
}

inline void
PushSpacerElement(ui_layout *Layout)
{
    PushElementInternal(Layout, ElementType_Spacer, UiInteraction_NOP, 0, String(""), 0);
}
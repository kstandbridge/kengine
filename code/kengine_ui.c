
inline void
BeginRow(ui_layout *Layout)
{
    ui_element *Element = PushStruct(Layout->Arena, ui_element);
    ZeroStruct(*Element);
    
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

internal ui_layout *
BeginUIFrame(memory_arena *Arena, ui_state *State, app_input *Input, assets *Assets, f32 Scale, f32 Padding, loaded_bitmap *DrawBuffer)
{
    ui_layout *Result = PushStruct(Arena, ui_layout);
    ZeroStruct(*Result);
    
    Result->Arena = Arena;
    Result->Assets = Assets;
    Result->Scale = Scale;
    Result->Padding = Padding;
    Result->DrawBuffer = DrawBuffer;
    Result->SentinalElement.Dim = V2((f32)DrawBuffer->Width, (f32)DrawBuffer->Height);
    Result->CurrentElement = &Result->SentinalElement;
    
    Result->CurrentId = 0;
    
    
    Result->MouseP = V2(Input->MouseX, Input->MouseY);
    Result->dMouseP = V2Subtract(Result->MouseP, State->LastMouseP);
    
    
    State->ToExecute = State->NextToExecute;
    ClearInteraction(&State->NextToExecute);
    
    return Result;
}

// TODO(kstandbridge): Ditch this crap
global s32 ColArrayIndex;
global v4 ColArray[25];

inline void
DrawSpacer(render_group *RenderGroup)
{
    PushClear(RenderGroup, ColArray[ColArrayIndex]);
    ColArrayIndex = (ColArrayIndex + 1) % ArrayCount(ColArray);
}

inline void
DrawLabel(ui_layout *Layout, render_group *RenderGroup, ui_element *Element)
{
    v2 TextP = V2Set1(0);
    
    DrawTextElement(RenderGroup, Layout->Assets, TextP, Element->Text, Element->TextOffset, Element->Dim, Layout->Scale, 
                    Colors.LabelBackground, Colors.LabelBorder, Colors.LabelText);
    
}

inline void
DrawCheckbox(ui_state *State, ui_layout *Layout, render_group *RenderGroup, ui_element *Element)
{
    PushClear(RenderGroup, Colors.Clear);
    
    v2 CheckboxP = V2(Layout->Padding, Layout->Padding*0.75f);
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
    
    DrawTextElement(RenderGroup, Layout->Assets, CheckboxP, CheckText, V2Set1(-Layout->Padding), CheckboxDim, Layout->Scale, 
                    CheckboxBackground, CheckboxBorder, CheckboxText);
    
    v2 TextP = V2(CheckboxP.X + CheckboxDim.X, 0);
    DrawTextElement(RenderGroup, Layout->Assets, TextP, Element->Text, Element->TextOffset, Element->Dim, Layout->Scale, 
                    Colors.CheckboxBackground, Colors.CheckboxBackground, Colors.CheckboxText);
    
    
    
    
}

internal void
DrawElements(ui_state *State, ui_layout *Layout, ui_element *FirstChild, v2 P)
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
                DrawElements(State, Layout, Element->FirstChild, P);
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
            
            loaded_bitmap *DrawBuffer = PushStruct(Layout->Arena, loaded_bitmap);
            DrawBuffer->Width = (s32)Element->Dim.X;
            DrawBuffer->Height = (s32)Element->Dim.Y;
            DrawBuffer->Pitch = Layout->DrawBuffer->Pitch;
            DrawBuffer->Memory = (u8 *)Layout->DrawBuffer->Memory + (s32)P.X*BITMAP_BYTES_PER_PIXEL + (s32)P.Y*DrawBuffer->Pitch;
            
            //DEBUGTextLine(FormatString(Layout->Arena, "At: %.02f %.02f Dim: %.02f %.02f", P.X, P.Y, Element->Dim.X, Element->Dim.Y));
            render_group *RenderGroup = AllocateRenderGroup(Layout->Arena, Megabytes(4), DrawBuffer);
            
            switch(Element->Type)
            {
                case Element_Row:
                {
                    InvalidCodePath;
                } break;
                
                case Element_Spacer:
                {
                    DrawSpacer(RenderGroup);
                } break;
                
                case Element_Label:
                {
                    DrawLabel(Layout, RenderGroup, Element);
                    
                } break;
                
                case Element_Checkbox:
                {
                    DrawCheckbox(State, Layout, RenderGroup, Element);
                } break;
                
                InvalidDefaultCase;
            }
            
            tile_render_work *Work = PushStruct(Layout->Arena, tile_render_work);
            ZeroStruct(*Work);
            Work->Group = RenderGroup;
            Work->ClipRect.MinX = 0;
            Work->ClipRect.MaxX = DrawBuffer->Width;
            Work->ClipRect.MinY = 0;
            Work->ClipRect.MaxY = DrawBuffer->Height;
            Platform.AddWorkEntry(Platform.PerFrameWorkQueue, TileRenderWorkThread, Work);
        }
    }
    
    Platform.CompleteAllWork(Platform.PerFrameWorkQueue);
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
        Element->Dim.X = Dim.X / ChildCount;
        Element->Dim.Y = Dim.Y;
        
        if(Element->Type != Element_Row)
        {
            if(Element->UsedDim.X > 0.0f)
            {
                Element->Dim.X = Element->UsedDim.X;
            }
            Element->TextBounds = GetTextSize(Layout->Assets, Layout->Scale, Element->Text);
            Element->TextOffset = V2(-Layout->Padding, Element->TextBounds.Min.Y - Layout->Padding);
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
                State->SelectedInteraction = State->HotInteraction;
                State->ClickedInteraction = State->HotInteraction;
            }
            
            switch(State->Interaction.Type)
            {
                
#if 0                
                case UiInteraction_ImmediateButton:
                {
                    if(MouseUp)
                    {
                        Layout->State->NextToExecute = Layout->State->Interaction;
                        EndInteraction = true;
                    }
                } break;
#endif
                
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
    
    
    
    
    v2 Dim = V2((f32)Layout->DrawBuffer->Width, (f32)Layout->DrawBuffer->Height);
    ui_element *Element = &Layout->SentinalElement;
    
    s32 ChildCount = Element->ChildCount - Element->SetWidthChildCount;
    v2 DimRemaining = V2(Dim.X - Element->UsedDim.X, Dim.Y);
    
    CalculateElementDims(Layout, Element->FirstChild, ChildCount, DimRemaining);
    
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
    
    v2 P = V2((f32)Layout->DrawBuffer->Width, 0.0f);
    DrawElements(State, Layout, Layout->SentinalElement.FirstChild, P);
    
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

#define Textbox(Layout, ...) Spacer(Layout)
#define Splitter(Layout, ...) Spacer(Layout)
#define DropDown(Layout, ...) Spacer(Layout)
#define Button(Layout, ...) Spacer(Layout)

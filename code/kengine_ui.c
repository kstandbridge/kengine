
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
    rectangle2 Result = InvertedInfinityRectangle2();
    
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
                    PushRenderCommandBitmap(RenderGroup, &Glyph->Bitmap, BitmapScale, V2Add(Offset, V2(2.0f, -2.0f)), V4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
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
                UIState->ClickedId = InteractionIdFromPtr(0);
                UIState->HotInteraction = UIState->NextHotInteraction;
                if(MouseDown)
                {
                    UIState->Interaction = UIState->HotInteraction;
                }
            } break;
            
            default:
            {
                UIState->ClickedId = InteractionIdFromPtr(0);
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
DrawText(render_group *RenderGroup, f32 Scale, v2 P, v4 Color, string Text)
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

inline ui_frame
BeginUIFrame(ui_state *UIState, v2 UpperLeftCorner)
{
    ui_frame Result;
    Result.UIState = UIState;
    Result.UpperLeftCorner = UpperLeftCorner;
    Result.Scale = 1.0f;
    Result.SpacingX = 10.0f;
    Result.SpacingY = 10.0f;
    Result.NoLineFeed = 0;
    Result.NextYDelta = 0;
    Result.At = V2(UpperLeftCorner.X, UpperLeftCorner.Y);
    Result.TotalBounds = InvertedInfinityRectangle2();
    return Result;
}

inline rectangle2
EndUIFrame(ui_frame *UIFrame)
{
    rectangle2 Result = UIFrame->TotalBounds;
    return Result;
}

inline ui_element
BeginUIElement(ui_frame *Frame, v2 Dim)
{
    ui_element Element;
    ZeroStruct(Element);
    
    Element.UIFrame = Frame;
    Element.Dim = Dim;
    
    return Element;
}

inline void
SetUIElementDefaultAction(ui_element *Element, ui_interaction Interaction)
{
    Element->Interaction = Interaction;
}

inline void
AdvanceFrameAt(ui_frame *UIFrame, rectangle2 Bounds)
{
    UIFrame->NextYDelta = Minimum(UIFrame->NextYDelta, Bounds.Min.Y - UIFrame->At.Y);
    
    if(UIFrame->NoLineFeed)
    {
        UIFrame->At.X = Bounds.Max.X + UIFrame->SpacingX;
    }
    else
    {
        UIFrame->At.Y += UIFrame->NextYDelta;
        UIFrame->At.X = UIFrame->UpperLeftCorner.X;
    }
}

inline void
EndUIElement(ui_element *Element)
{
    ui_frame *UIFrame = Element->UIFrame;
    ui_state *UIState = UIFrame->UIState;
    
    v2 Margin = V2(UIFrame->SpacingX, UIFrame->SpacingY);
    v2 TotalDim = V2Add(Element->Dim, Margin);
    v2 TotalMinCorner = V2(UIFrame->At.X, UIFrame->At.Y - TotalDim.Y);
    v2 TotalMaxCorner = V2Add(TotalMinCorner , TotalDim);
    
    v2 InteriorMinCorner = V2Add(TotalMinCorner, Margin);
    v2 InteriorMaxCorner = V2Add(InteriorMinCorner, Element->Dim);
    Element->Bounds = Rectangle2(InteriorMinCorner, InteriorMaxCorner);
    
    if(Element->Interaction.Type && IsInRectangle(Element->Bounds, UIState->MouseP))
    {
        UIState->NextHotInteraction = Element->Interaction;
        Element->IsHot = true;
    }
    
    if(Element->Size)
    {
        // TODO(kstandbridge): Resize interaction
    }
    
    rectangle2 TotalBounds = Rectangle2(TotalMinCorner, TotalMaxCorner);
    UIFrame->TotalBounds = Rectangle2Union(UIFrame->TotalBounds, TotalBounds);
    AdvanceFrameAt(UIFrame, TotalBounds);
}

internal ui_element
DrawTextElement_(ui_frame *UIFrame, render_group *RenderGroup, ui_interaction Interaction, f32 Width, string Text, 
                 element_colors Colors)
{
    ui_state *UIState = UIFrame->UIState;
    
    rectangle2 TextBounds = GetTextSize(RenderGroup, UIFrame->Scale, UIFrame->At, Text);
    v2 TextDim = GetDim(TextBounds);
    v2 ElementDim = V2(Width, TextDim.Y + UIFrame->SpacingY*2.0f);
    
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
    
    ui_element Element = BeginUIElement(UIFrame, ElementDim);
    SetUIElementDefaultAction(&Element, Interaction);
    EndUIElement(&Element);
    
    v2 TextOffset = V2Add(Element.Bounds.Min, V2(Width*0.5f - TextDim.X*0.5f, UIFrame->SpacingY));
    DrawText(RenderGroup, UIFrame->Scale, TextOffset, TextColor, Text);
    
    if(BackgroundColor.A > 0.0f)
    {
        PushRenderCommandRectangle(RenderGroup, BackgroundColor, Element.Bounds, 0.25f);
    }
    if(BorderColor.A > 0.0f)
    {
        PushRenderCommandRectangleOutline(RenderGroup, 1.0f, BorderColor, Element.Bounds, 1.0f);
    }
    
    return Element;
}

// TODO(kstandbridge): Button format
internal b32
Button(ui_frame *UIFrame, render_group *RenderGroup, f32 Width, ui_interaction_id Id, void *Target, string Text)
{
    ui_state *UIState = UIFrame->UIState;
    // TODO(kstandbridge): This shouldn't have a perm arena, later we will not load textures in the push draw routine
    b32 Result = false;
    
    ui_interaction Interaction;
    Interaction.Id = Id;
    Interaction.Type = Interaction_ImmediateButton;
    Interaction.Target = Target;
    
    element_colors Colors = ElementColors(RGBColor(255, 255, 255, 255), RGBColor(0, 120, 215, 255), RGBColor(0, 84, 153, 255),
                                          RGBColor(225, 225, 225, 255), RGBColor(229, 241, 251, 255), RGBColor(204, 228, 247, 255),
                                          RGBColor(173, 173, 173, 255), RGBColor(0, 120, 215, 255), RGBColor(0, 84, 153, 255));
    ui_element Element = DrawTextElement_(UIFrame, RenderGroup, Interaction, Width, Text, Colors);
    
    if(InteractionsAreEqual(Interaction, UIState->ToExecute) &&
       Element.IsHot)
    {
        Result = true;
    }
    
    return Result;
}

inline void
BeginRow(ui_frame *UIFrame)
{
    ++UIFrame->NoLineFeed;
}

inline void
EndRow(ui_frame *UIFrame)
{
    Assert(UIFrame->NoLineFeed > 0);
    --UIFrame->NoLineFeed;
    
    AdvanceFrameAt(UIFrame, Rectangle2(UIFrame->At, UIFrame->At));
}
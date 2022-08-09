
inline void
ClearInteraction(interaction *Interaction)
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
TextOp_(render_group *RenderGroup, memory_arena *Arena, loaded_glyph *Glyphs, text_op_type Op, f32 Scale, v2 P, v4 Color, string Text)
{
    // TODO(kstandbridge): Consoliate these parameters, remember GlobalState, LocalState, Misc
    
    rectangle2 Result = InvertedInfinityRectangle2();
    
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
Interact(u_i_state *UIState, app_input *Input)
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
                if(MouseUp)
                {
                    UIState->NextToExecute = UIState->Interaction;
                    EndInteraction = true;
                }
            } break;
            
            case Interaction_Select:
            {
                if(MouseUp)
                {
                    // TODO(kstandbridge): Select something?
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

inline b32
InteractionIdsAreEqual(interaction_id A, interaction_id B)
{
    b32 Result = ((A.Value[0].U64 == B.Value[0].U64) &&
                  (A.Value[1].U64 == B.Value[1].U64));
    
    return Result;
}

inline b32
InteractionsAreEqual(interaction A, interaction B)
{
    b32 Result = (InteractionIdsAreEqual(A.Id, B.Id) &&
                  (A.Type == B.Type) &&
                  (A.Target == B.Target) &&
                  (A.Generic == A.Generic));
    
    return Result;
}

inline b32
InteractionIsHot(u_i_state *UIState, interaction Interaction)
{
    b32 Result = InteractionsAreEqual(UIState->HotInteraction, Interaction);
    
    if(Interaction.Type == Interaction_None)
    {
        Result = false;
    }
    
    return Result;
}

inline void
DrawText(render_group *RenderGroup, memory_arena *Arena, loaded_glyph *Glyphs, f32 Scale, v2 P, v4 Color, string Text)
{
    // TODO(kstandbridge): NAMING Internally this is calling PushRenderCommandBitmap for each of the glyphs,
    // nothing is being drawn now, its a push call so shouldn't be DrawText
    
    // TODO(kstandbridge): Consoliate these parameters, remember GlobalState, LocalState, Misc
    TextOp_(RenderGroup, Arena, Glyphs, TextOpText_Draw, Scale, P, Color, Text);
}

inline rectangle2
GetTextSize(render_group *RenderGroup, memory_arena *Arena, loaded_glyph *Glyphs, f32 Scale, v2 P, string Text)
{
    // TODO(kstandbridge): Consoliate these parameters, remember GlobalState, LocalState, Misc
    rectangle2 Result = TextOp_(RenderGroup, Arena, Glyphs, TextOpText_Size, Scale, P, V4(1.0f, 1.0f, 1.0f, 1.0f), Text);
    return Result;
}

inline void
BeginUIFrame(u_i_state *UIState, app_input *Input)
{
    UIState->LastMouseP = UIState->MouseP;
    UIState->MouseP = V2(Input->MouseX, Input->MouseY);
    UIState->dMouseP = V2Subtract(UIState->LastMouseP, UIState->MouseP);
}

inline void
EndUIFrame(u_i_state *UIState, app_input *Input)
{
    Interact(UIState, Input);
    UIState->ToExecute = UIState->NextToExecute;
    ClearInteraction(&UIState->NextToExecute);
    ClearInteraction(&UIState->NextHotInteraction);
}

// TODO(kstandbridge): Button format
internal b32
Button(u_i_state *UIState, render_group *RenderGroup, interaction_id Id, void *Target, string Text)
{
    // TODO(kstandbridge): This shouldn't have a perm arena, later we will not load textures in the push draw routine
    b32 Result = false;
    
    f32 Scale = 1.0f;
    v2 P = V2(10.0f, 100.0f);
    
    interaction Interaction;
    Interaction.Id = Id;
    Interaction.Type = Interaction_ImmediateButton;
    Interaction.Target = Target;
    
    if(InteractionsAreEqual(Interaction, UIState->ToExecute))
    {
        Result = true;
    }
    
    rectangle2 TextBounds;
    // BeginElement()
    {
        TextBounds = GetTextSize(RenderGroup, RenderGroup->Arena, RenderGroup->Glyphs, Scale, P, Text);
    }
    // EndElement()
    {
        // TODO(kstandbridge): Maybe add a border and calculate exact bounds of entire element
        
        if(IsInRectangle(TextBounds, UIState->MouseP))
        {
            UIState->NextHotInteraction = Interaction;
        }
    }
    
    b32 IsHot = InteractionIsHot(UIState, Interaction);
    v4 TextColor = IsHot ? V4(1.0f, 1.0f, 0.0f, 1.0f) : V4(1.0f, 1.0f, 1.0f, 1.0f);
    
    DrawText(RenderGroup, RenderGroup->Arena, RenderGroup->Glyphs, Scale, P, TextColor, Text);
    
    return Result;
}


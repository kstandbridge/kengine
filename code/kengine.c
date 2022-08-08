#include "kengine.h"

global platform_api Platform;

#include "kengine_sort.c"
#include "kengine_render_group.c"

// TODO(kstandbridge): Move to UI code file
typedef enum
{
    TextOpText_Draw,
    TextOpText_Size,
} text_op_type;
internal rectangle2
TextOp_(render_group *RenderGroup, memory_arena *Arena, loaded_glyph *Glyphs, text_op_type Op, f32 Scale, v2 P, string Text)
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
                    PushRenderCommandBitmap(RenderGroup, &Glyph->Bitmap, BitmapScale, Offset, V4(1.0f, 1.0f, 1.0f, 1.0f), 2.0f);
                    PushRenderCommandBitmap(RenderGroup, &Glyph->Bitmap, BitmapScale, V2Add(Offset, V2(2.0f, -2.0f)), V4(0.0f, 0.0f, 0.0f, 1.0f), 1.0f);
                }
                else
                {
                    Assert(Op == TextOpText_Size);
                    
                    bitmap_dim Dim = GetBitmapDim(&Glyph->Bitmap, BitmapScale, Offset);
                    rectangle2 GlyphDim = Rectangle2(Dim.P, Dim.Size);
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

inline void
DrawText(render_group *RenderGroup, memory_arena *Arena, loaded_glyph *Glyphs, f32 Scale, v2 P, string Text)
{
    // TODO(kstandbridge): Consoliate these parameters, remember GlobalState, LocalState, Misc
    TextOp_(RenderGroup, Arena, Glyphs, TextOpText_Draw, Scale, P, Text);
}

inline rectangle2
GetTextSize(render_group *RenderGroup, memory_arena *Arena, loaded_glyph *Glyphs, f32 Scale, v2 P, string Text)
{
    // TODO(kstandbridge): Consoliate these parameters, remember GlobalState, LocalState, Misc
    rectangle2 Result = TextOp_(RenderGroup, Arena, Glyphs, TextOpText_Size, Scale, P, Text);
    return Result;
}

extern void
AppUpdateFrame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input)
{
    Platform = *PlatformAPI;
    
    
    if(!PlatformAPI->AppState)
    {
        PlatformAPI->AppState = PushStruct(Arena, app_state);
        app_state *AppState = PlatformAPI->AppState;
        
        AppState->TestGlyph = Platform.GetGlyphForCodePoint(Arena, 'K');
        SubArena(&AppState->TranArena, Arena, Kilobytes(512));
    }
    
    app_state *AppState = PlatformAPI->AppState;
    Assert(AppState->TestGlyph.Bitmap.Memory);
    
    render_group RenderGroup_;
    ZeroStruct(RenderGroup_);
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    RenderGroup->CurrentClipRectIndex = PushRenderCommandClipRectangle(RenderGroup, Rectangle2i(0, Commands->Width, 0, Commands->Height));
    
    if(Platform.DllReloaded)
    {
        PushRenderCommandClear(RenderGroup, 0.0f, V4(0.0f, 1.0f, 0.0f, 1.0f));
    }
    else
    {
        PushRenderCommandClear(RenderGroup, 0.0f, V4(1.0f, 0.5f, 0.0f, 1.0f));
    }
    
#if 1
    // NOTE(kstandbridge): Rectangle with clipping regions
    {    
        f32 Width = (f32)Commands->Width;
        f32 Height = (f32)Commands->Height;
        
        u16 OldClipRect = RenderGroup->CurrentClipRectIndex;
        RenderGroup->CurrentClipRectIndex =
            PushRenderCommandClipRectangle(RenderGroup, Rectangle2i((s32)(Width*0.25f), (s32)(Width*0.75f), 
                                                                    (s32)(Height*0.25f), (s32)(Height*0.75f)));
        
        PushRenderCommandRectangle(RenderGroup, V4(1.0f, 0.0f, 0.0f, 1.0f),
                                   Rectangle2(V2(0.0f, 0.0f), V2(Width*0.5f, Height*0.5f)), 1.0f);
        
        PushRenderCommandRectangle(RenderGroup, V4(0.0f, 1.0f, 0.0f, 1.0f),
                                   Rectangle2(V2(Width*0.5f, 0.0f), V2(Width, Height*0.5f)), 1.0f);
        
        PushRenderCommandRectangle(RenderGroup, V4(0.0f, 0.0f, 1.0f, 1.0f),
                                   Rectangle2(V2(0.0f, Height*0.5f), V2(Width*0.5f, Height)), 1.0f);
        
        PushRenderCommandRectangle(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f),
                                   Rectangle2(V2(Width*0.5f, Height*0.5f), V2(Width, Height)), 1.0f);
        
        RenderGroup->CurrentClipRectIndex = OldClipRect;
    }
#endif
    
#if 0
    // NOTE(kstandbridge): Bitmaps
    {    
        loaded_bitmap *Glyph = &AppState->TestGlyph;
        PushRenderCommandBitmap(RenderGroup, Glyph, (f32)Glyph->Height, V2(10.0f, 10.0f), V4(1.0f, 1.0f, 1.0f, 1.0f), 30.f);
    }
#endif
    
    
#if 1
    
    
    // NOTE(kstandbridge): Text rendering
    f32 Scale = 1.0f;
    v2 P = V2(10.0f, 100.0f);
    temporary_memory TempMem = BeginTemporaryMemory(&AppState->TranArena);
    string Text = FormatString(TempMem.Arena, "Mouse: %.03f / %.03f", Input->MouseX, Input->MouseY);
    rectangle2 Bounds = GetTextSize(RenderGroup, Arena, AppState->Glyphs, Scale, P, Text);
    DrawText(RenderGroup, Arena, AppState->Glyphs, Scale, P, Text);
    
    string SizeText = FormatString(TempMem.Arena, "Bounds: %.03f / %.03f | %.03f / %.03f", Bounds.Min.X, Bounds.Min.Y, Bounds.Max.X, Bounds.Max.Y);
    DrawText(RenderGroup, Arena, AppState->Glyphs, Scale, V2Add(P, V2(0, Platform.GetVerticleAdvance())), SizeText);
    
    EndTemporaryMemory(TempMem);
    
    CheckArena(&AppState->TranArena);
#endif
}
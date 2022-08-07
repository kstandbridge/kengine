#include "kengine.h"

global platform_api Platform;

#include "kengine_sort.c"
#include "kengine_render_group.c"

extern void
AppUpdateFrame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena)
{
    Platform = *PlatformAPI;
    
    
    if(!PlatformAPI->AppState)
    {
        PlatformAPI->AppState = PushStruct(Arena, app_state);
        PlatformAPI->AppState->TestGlyph = Platform.GetGlyphForCodePoint(Arena, 'K');
    }
    
    app_state *AppState = PlatformAPI->AppState;
    Assert(AppState->TestGlyph.Memory);
    
    render_group RenderGroup_;
    ZeroStruct(RenderGroup_);
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    RenderGroup->CurrentClipRectIndex = PushRenderCommandClipRectangle(RenderGroup, Rectangle2i(0, Commands->Width, 0, Commands->Height));
    
    PushRenderCommandClear(RenderGroup, 0.0f, V4(0.0f, 0.0f, 0.0f, 1.0f));
    
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
    {
        f32 AtX = P.X;
        f32 AtY = P.Y;
        u32 PrevCodePoint = 0;
        string Text = String("Hello, world!");
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
                    Assert(CodePoint < ArrayCount(AppState->Glyphs));
                    if(AppState->Glyphs[CodePoint].Memory == 0)
                    {
                        // TODO(kstandbridge): This should be threaded
                        AppState->Glyphs[CodePoint] = Platform.GetGlyphForCodePoint(Arena, CodePoint);
                    }
                    
                    loaded_bitmap *Glyph = AppState->Glyphs + CodePoint;
                    Assert(Glyph->Memory);
                    
                    PushRenderCommandBitmap(RenderGroup, Glyph, (f32)Glyph->Height, V2(AtX, AtY), V4(1.0f, 1.0f, 1.0f, 1.0f), 30.f);
                    
                    PrevCodePoint = CodePoint;
                }
                f32 AdvanceX = Scale*Platform.GetHorizontalAdvance(PrevCodePoint, CodePoint);
                
                AtX += AdvanceX;
                
            }
#endif
            
        }
    }
}
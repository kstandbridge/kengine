#include "kengine.h"

global platform_api Platform;

#include "kengine_sort.c"
#include "kengine_render_group.c"
#include "kengine_ui.c"

extern void
AppUpdateFrame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input)
{
    Platform = *PlatformAPI;
    
    
    if(!PlatformAPI->AppState)
    {
        PlatformAPI->AppState = PushStruct(Arena, app_state);
        app_state *AppState = PlatformAPI->AppState;
        
        AppState->TestGlyph = Platform.GetGlyphForCodePoint(Arena, 'K');
        // NOTE(kstandbridge): GetVerticleAdvance will return 0 if no glyphs have been loaded
        AppState->UIState.LineAdvance = Platform.GetVerticleAdvance();
        
        SubArena(&AppState->TranArena, Arena, Kilobytes(512));
    }
    
    app_state *AppState = PlatformAPI->AppState;
    Assert(AppState->TestGlyph.Bitmap.Memory);
    
    render_group RenderGroup_;
    ZeroStruct(RenderGroup_);
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    RenderGroup->CurrentClipRectIndex = PushRenderCommandClipRectangle(RenderGroup, Rectangle2i(0, Commands->Width, 0, Commands->Height));
    RenderGroup->Arena = Arena;
    RenderGroup->Glyphs = AppState->Glyphs;
    
    if(Platform.DllReloaded)
    {
        PushRenderCommandClear(RenderGroup, 0.0f, V4(0.0f, 1.0f, 0.0f, 1.0f));
    }
    else
    {
        PushRenderCommandClear(RenderGroup, 0.0f, RGBColor(255, 255, 255, 255));
    }
    
#if 0
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
    
    ui_state *UIState = &AppState->UIState;
    
    
    UIState->MouseDown = Input->MouseButtons[MouseButton_Left].EndedDown;
    UIState->LastMouseP = UIState->MouseP;
    UIState->MouseP = V2(Input->MouseX, Input->MouseY);
    UIState->dMouseP = V2Subtract(UIState->LastMouseP, UIState->MouseP);
    
    temporary_memory TempMem = BeginTemporaryMemory(&AppState->TranArena);
    
    ui_frame UIFrame = BeginUIFrame(UIState, V2(0.0f, (f32)Commands->Height));
    {    
        string Text = FormatString(TempMem.Arena, "Mouse: %.03f / %.03f", Input->MouseX, Input->MouseY);
        if(Button(&UIFrame, RenderGroup, 64.0f, InteractionIdFromPtr(AppState), AppState, String("Back")))
        {
            __debugbreak();
        }
        if(Button(&UIFrame, RenderGroup, 512.0f, InteractionIdFromPtr(AppState), AppState, Text))
        {
            __debugbreak();
        }
        
        BeginRow(&UIFrame);
        Button(&UIFrame, RenderGroup, 64.0f, InteractionIdFromPtr(AppState), AppState, String("Foo"));
        Button(&UIFrame, RenderGroup, 48.0f, InteractionIdFromPtr(AppState), AppState, String("Bar"));
        Button(&UIFrame, RenderGroup, 64.0f, InteractionIdFromPtr(AppState), AppState, String("Bas"));
        EndRow(&UIFrame);
        
        
        if(Button(&UIFrame, RenderGroup, 128.0f, InteractionIdFromPtr(AppState), AppState, String("Forward")))
        {
            __debugbreak();
        }
        
        f32 Width = (f32)Commands->Width;
        f32 Height = (f32)Commands->Height;
        
        PushRenderCommandRectangleOutline(RenderGroup, 10.0f, V4(0.0f, 0.5f, 0.5f, 1.0f),
                                          Rectangle2(V2(Width*0.25f, Height*0.25f), 
                                                     V2(Width*0.75f, Height*0.75f)),
                                          1000000.0f);
    }
    
    rectangle2 TotalBounds = EndUIFrame(&UIFrame);
    ui_frame RightPanel = BeginUIFrame(UIState, V2(TotalBounds.Max.X, TotalBounds.Max.Y));
    Button(&RightPanel, RenderGroup, 64.0f, InteractionIdFromPtr(AppState), AppState, String("One"));
    BeginRow(&RightPanel);
    Button(&RightPanel, RenderGroup, 48.0f, InteractionIdFromPtr(AppState), AppState, String("Two"));
    Button(&RightPanel, RenderGroup, 64.0f, InteractionIdFromPtr(AppState), AppState, String("Three"));
    Button(&RightPanel, RenderGroup, 96.0f, InteractionIdFromPtr(AppState), AppState, String("Four"));
    EndRow(&RightPanel);
    if(Button(&RightPanel, RenderGroup, 64.0f, InteractionIdFromPtr(AppState), AppState, String("Five")))
    {
        __debugbreak();
    }
    TotalBounds = EndUIFrame(&RightPanel);
    
    Interact(UIState, Input);
    UIState->ToExecute = UIState->NextToExecute;
    ClearInteraction(&UIState->NextToExecute);
    ClearInteraction(&UIState->NextHotInteraction);
    
    EndTemporaryMemory(TempMem);
    CheckArena(&AppState->TranArena);
}
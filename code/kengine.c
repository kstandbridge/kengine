#include "kengine.h"

global platform_api Platform;

#if KENGINE_INTERNAL
global debug_event_table *GlobalDebugEventTable;
#endif

#include "kengine_sort.c"
#include "kengine_render_group.c"
#include "kengine_ui.c"

extern void
AppUpdateFrame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input)
{
    Platform = *PlatformAPI;
#if KENGINE_INTERNAL
    GlobalDebugEventTable = PlatformAPI->DebugEventTable;
#endif
    
    
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
    
    
#if KENGINE_INTERNAL 
    if(Platform.DllReloaded)
    {
        PushRenderCommandClear(RenderGroup, 0.0f, V4(0.0f, 1.0f, 0.0f, 1.0f));
    }
    else
#endif
    {
        PushRenderCommandClear(RenderGroup, 0.0f, RGBColor(255, 255, 255, 255));
    }
    
    // NOTE(kstandbridge): Rectangle with clipping regions
    
#if 0
    f32 Width = (f32)Commands->Width;
    f32 Height = (f32)Commands->Height;
    
    u16 OldClipRect = RenderGroup->CurrentClipRectIndex;
    // TODO(kstandbridge): BeginClip/EndClip?
    RenderGroup->CurrentClipRectIndex =
        PushRenderCommandClipRectangle(RenderGroup, Rectangle2i((s32)(Width*0.1f), (s32)(Width*0.9f), 
                                                                (s32)(Height*0.1f), (s32)(Height*0.9f)));
    
    
    PushRenderCommandRectangle(RenderGroup, V4(1.0f, 0.0f, 0.0f, 1.0f),
                               Rectangle2(V2(0.0f, 0.0f), V2(Width*0.5f, Height*0.5f)), 1.0f);
    
    PushRenderCommandRectangle(RenderGroup, V4(0.0f, 1.0f, 0.0f, 1.0f),
                               Rectangle2(V2(Width*0.5f, 0.0f), V2(Width, Height*0.5f)), 1.0f);
    
    PushRenderCommandRectangle(RenderGroup, V4(0.0f, 0.0f, 1.0f, 1.0f),
                               Rectangle2(V2(0.0f, Height*0.5f), V2(Width*0.5f, Height)), 1.0f);
    
    PushRenderCommandRectangle(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f),
                               Rectangle2(V2(Width*0.5f, Height*0.5f), V2(Width, Height)), 1.0f);
    
    RenderGroup->CurrentClipRectIndex = OldClipRect;
#endif
    
    ui_state *UIState = &AppState->UIState;
    
    
    UIState->MouseDown = Input->MouseButtons[MouseButton_Left].EndedDown;
    UIState->LastMouseP = UIState->MouseP;
    UIState->MouseP = V2(Input->MouseX, Input->MouseY);
    UIState->dMouseP = V2Subtract(UIState->LastMouseP, UIState->MouseP);
    
    temporary_memory TempMem = BeginTemporaryMemory(&AppState->TranArena);
    
    ui_grid MainGrid = BeginGrid(UIState, TempMem.Arena, Rectangle2(V2Set1(0.0f), V2((f32)Commands->Width, (f32)Commands->Height)), 2, 1);
    {
        SetRowHeight(&MainGrid, 0, SIZE_AUTO);
        SetRowHeight(&MainGrid, 1, SIZE_AUTO);
        InitializeGridSize(&MainGrid);
        ui_grid Grid = BeginGrid(UIState, TempMem.Arena, GetCellBounds(&MainGrid, 0, 0), 3, 3);
        {
            SetColumnWidth(&Grid, 0, 0, 128.0f);
            SetColumnWidth(&Grid, 2, 2, 256.0f);
            
            SetRowHeight(&Grid, 1, SIZE_AUTO);
            
            
            if(Button(&Grid, RenderGroup, 0, 0, GenerateInteractionId(AppState), AppState, String("Top Left"))) { __debugbreak(); }
            if(Button(&Grid, RenderGroup, 1, 0, GenerateInteractionId(AppState), AppState, String("Top Middle"))) { __debugbreak(); }
            if(Button(&Grid, RenderGroup, 2, 0, GenerateInteractionId(AppState), AppState, String("Top Right"))) { __debugbreak(); }
            
            if(Button(&Grid, RenderGroup, 0, 1, GenerateInteractionId(AppState), AppState, String("Middle Left"))) { __debugbreak(); }
            
            ui_grid InnerGrid = BeginGrid(UIState, TempMem.Arena, GetCellBounds(&Grid, 1, 1), 3, 2);
            {
                SetRowHeight(&InnerGrid, 1, SIZE_AUTO);
                if(Button(&InnerGrid, RenderGroup, 0, 0, GenerateInteractionId(AppState), AppState, String("Inner NW"))) { __debugbreak(); }
                if(Button(&InnerGrid, RenderGroup, 1, 0, GenerateInteractionId(AppState), AppState, String("Inner NE"))) { __debugbreak(); }
                if(Button(&InnerGrid, RenderGroup, 0, 1, GenerateInteractionId(AppState), AppState, String("Inner W"))) { __debugbreak(); }
                if(Button(&InnerGrid, RenderGroup, 1, 1, GenerateInteractionId(AppState), AppState, String("Inner E"))) { __debugbreak(); }
                if(Button(&InnerGrid, RenderGroup, 0, 2, GenerateInteractionId(AppState), AppState, String("Inner SW"))) { __debugbreak(); }
                if(Button(&InnerGrid, RenderGroup, 1, 2, GenerateInteractionId(AppState), AppState, String("Inner SE"))) { __debugbreak(); }
            }
            EndGrid(&InnerGrid);
            
            if(Button(&Grid, RenderGroup, 2, 1, GenerateInteractionId(AppState), AppState, String("Middle Right"))) { __debugbreak(); }
            
            if(Button(&Grid, RenderGroup, 0, 2, GenerateInteractionId(AppState), AppState, String("Bottom Left"))) { __debugbreak(); }
            if(Button(&Grid, RenderGroup, 1, 2, GenerateInteractionId(AppState), AppState, String("Bottom Middle"))) { __debugbreak(); }
            if(Button(&Grid, RenderGroup, 2, 2, GenerateInteractionId(AppState), AppState, String("Bottom Right"))) { __debugbreak(); }
        }
        EndGrid(&Grid);
        
        debug_state *DebugState = PlatformAPI->DebugState;
        if(DebugState)
        {
            u16 FrameCount = ArrayCount(DebugState->Frames);
            
            ui_grid DebugGrid = BeginGrid(UIState, TempMem.Arena, GetCellBounds(&MainGrid, 0, 1), FrameCount, 1);
            {
                for(u16 FrameIndex = 0;
                    FrameIndex < FrameCount;
                    ++FrameIndex)
                {
                    debug_frame *Frame = DebugState->Frames + FrameIndex;
                    f32 MsPerFrame = Frame->SecondsElapsed;
                    f32 FramesPerSecond = 1.0f / MsPerFrame;
                    string FPS = FormatString(TempMem.Arena, "%.02f ms %.02f fps", MsPerFrame*1000.0f, FramesPerSecond);
                    Button(&DebugGrid, RenderGroup, 0, FrameIndex, GenerateInteractionId(AppState), AppState, FPS);
                }
            }
            EndGrid(&DebugGrid);
        }
        
        
    }
    EndGrid(&MainGrid);
    
    
    Interact(UIState, Input);
    UIState->ToExecute = UIState->NextToExecute;
    ClearInteraction(&UIState->NextToExecute);
    ClearInteraction(&UIState->NextHotInteraction);
    
    EndTemporaryMemory(TempMem);
    CheckArena(&AppState->TranArena);
}

#if KENGINE_INTERNAL
#include "kengine_debug.c"
#endif
#include "kengine.h"

global platform_api *Platform;

#if KENGINE_INTERNAL
global debug_event_table *GlobalDebugEventTable;
#endif

#include "kengine_sort.c"
#include "kengine_render_group.c"
#include "kengine_ui.c"

#if KENGINE_INTERNAL
#include "kengine_debug.c"
#endif

#include "main.c"

extern void
AppUpdateFrame(platform_api *PlatformAPI, render_commands *Commands, memory_arena *Arena, app_input *Input)
{
    Platform = PlatformAPI;
    
#if KENGINE_INTERNAL
    GlobalDebugEventTable = Platform->DebugEventTable;
#endif
    
    if(!Platform->AppState)
    {
        Platform->AppState = PushStruct(Arena, app_state);
    }
    
    if(!Platform->UIState)
    {
        Platform->UIState = PushStruct(Arena, ui_state);
        SubArena(&Platform->UIState->TranArena, Arena, Kilobytes(1024));
        
        // NOTE(kstandbridge): GetVerticleAdvance will return 0 if no glyphs have been loaded
        Platform->GetGlyphForCodePoint(Arena, 'K');
        Platform->UIState->LineAdvance = Platform->GetVerticleAdvance();
    }
    
    ui_state *UIState = Platform->UIState;
    
    render_group RenderGroup_;
    ZeroStruct(RenderGroup_);
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    RenderGroup->CurrentClipRectIndex = PushRenderCommandClipRectangle(RenderGroup, Rectangle2i(0, Commands->Width, 0, Commands->Height));
    RenderGroup->Arena = Arena;
    RenderGroup->Glyphs = UIState->Glyphs;
    
    
#if KENGINE_INTERNAL 
    if(Platform->DllReloaded)
    {
        PushRenderCommandClear(RenderGroup, 0.0f, V4(0.0f, 1.0f, 0.0f, 1.0f));
    }
    else
#endif
    {
        PushRenderCommandClear(RenderGroup, 0.0f, RGBColor(255, 255, 255, 255));
    }
    
    
#if 0
    // NOTE(kstandbridge): Rectangle with clipping regions
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
    
    UIState->MouseDown = Input->MouseButtons[MouseButton_Left].EndedDown;
    UIState->LastMouseP = UIState->MouseP;
    UIState->MouseP = V2(Input->MouseX, Input->MouseY);
    UIState->dMouseP = V2Subtract(UIState->LastMouseP, UIState->MouseP);
    
    temporary_memory TempMem = BeginTemporaryMemory(&UIState->TranArena);
    
#if KENGINE_INTERNAL
    if(UIState->ShowDebugTab)
    {
        ui_grid MainGrid = BeginGrid(UIState, TempMem.Arena, Rectangle2(V2Set1(0.0f), V2((f32)Commands->Width, (f32)Commands->Height)), 2, 1);
        {
            SetRowHeight(&MainGrid, 0, SIZE_AUTO);
            SetRowHeight(&MainGrid, 1, SIZE_AUTO);
            InitializeGridSize(&MainGrid);
            
            BEGIN_BLOCK("DrawAppGrid");
            {
                DrawAppGrid(Platform->AppState, UIState, RenderGroup, Arena, TempMem.Arena, Input, GetCellBounds(&MainGrid, 0, 0));
            }
            END_BLOCK();
            
            BEGIN_BLOCK("DrawDebugGrid");
            {
                DrawDebugGrid(Platform->DebugState, UIState, RenderGroup, Arena, TempMem.Arena, Input, GetCellBounds(&MainGrid, 0, 1));
            }
            END_BLOCK();
            
        }
        EndGrid(&MainGrid);
    }
    else
    {
        DrawAppGrid(Platform->AppState, UIState, RenderGroup, Arena, TempMem.Arena, Input, Rectangle2(V2Set1(0.0f), V2((f32)Commands->Width, (f32)Commands->Height)));
    }
#else
    DrawAppGrid(AppStatePlatform->, UIState, RenderGroup, Arena, TempMem.Arena, Input, Rectangle2(V2Set1(0.0f), V2((f32)Commands->Width, (f32)Commands->Height)));
#endif
    
    Interact(UIState, Input);
    UIState->ToExecute = UIState->NextToExecute;
    ClearInteraction(&UIState->NextToExecute);
    ClearInteraction(&UIState->NextHotInteraction);
    
    if(Input->FKeyPressed[11])
    {
        UIState->ShowDebugTab = !UIState->ShowDebugTab;
    }
    
    EndTemporaryMemory(TempMem);
    CheckArena(&UIState->TranArena);
    CheckArena(Arena);
}

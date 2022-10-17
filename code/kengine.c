#include "kengine.h"

typedef struct colors
{
    v4 Clear;
    v4 TabBackground;
    v4 TabHeaderBackground;
    v4 TabHeaderHotBackground;
    v4 TabBorder;
    
    v4 SelectedTextBackground;
    v4 SelectedText;
    v4 Text;
    
    v4 TextboxBackground;
    
    v4 ScrollButton;
    v4 ScrollButtonText;
    v4 ScrollButtonHot;
    v4 ScrollButtonHotText;
    v4 ScrollButtonClicked;
    v4 ScrollButtonClickedText;
    
    v4 ScrollBar;
    v4 ScrollBarBackground;
    v4 ScrollBarHot;
    v4 ScrollBarClicked;
    
} colors;

#if KENGINE_INTERNAL
platform_api Platform;
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
AppHandleArguments(app_memory *Memory)
{
    __debugbreak();
#if KENGINE_INTERNAL
    Platform = Memory->PlatformAPI;
#endif
    app_state *AppState = Memory->AppState;
    if(!AppState)
    {
        AppState = Memory->AppState = BootstrapPushStruct(app_state, TotalArena);
    }
    int x = 5;
}

extern void
AppUpdateFrame(app_memory *Memory, render_commands *Commands, memory_arena *Arena, app_input *Input)
{
#if KENGINE_INTERNAL
    Platform = Memory->PlatformAPI;
    GlobalDebugEventTable = Memory->DebugEventTable;
#endif
    
    if(!Memory->AppState)
    {
        Memory->AppState = PushStruct(Arena, app_state);
        GlobalDebugEventTable->Settings.ShowDebugTab = true;
        
#if 0        
        b32 HttpInit = Platform.InitializeHttpServer();
        Assert(HttpInit);
#endif
        
    }
    
    if(!Memory->UIState)
    {
        Memory->UIState = PushStruct(Arena, ui_state);
        Memory->UIState->PermArena = Arena;
        
        // NOTE(kstandbridge): GetVerticleAdvance will return 0 if no glyphs have been loaded
        Platform.GetGlyphForCodePoint(Arena, 'K');
        Memory->UIState->LineAdvance = Platform.GetVerticleAdvance();
    }
    
    if(Input->dtForFrame > 0.1f)
    {
        Input->dtForFrame = 0.1f;
    }
    else if(Input->dtForFrame < 0.001f)
    {
        Input->dtForFrame = 0.001f;
    }
    
    app_state *AppState = Memory->AppState;
    ui_state *UIState = Memory->UIState;
    debug_state *DebugState = Memory->DebugState;
    
    colors Colors;
    Colors.Clear = RGBColor(240, 240, 240, 255);
    Colors.TabBackground = RGBColor(255, 255, 255, 255);
    Colors.TabHeaderBackground = RGBColor(240, 240, 240, 255);
    Colors.TabHeaderHotBackground = RGBColor(216, 234, 249, 255);
    Colors.TabBorder = RGBColor(217, 217, 217, 255);
    
    
    Colors.SelectedTextBackground = RGBColor(0, 120, 215, 255);
    Colors.SelectedText = RGBColor(255, 255, 255, 255);
    Colors.Text = RGBColor(0, 0, 0, 255);
    
    
    Colors.TextboxBackground = RGBColor(255, 255, 255, 255);
    
    Colors.ScrollButton = RGBColor(240, 240, 240, 255);
    Colors.ScrollButtonText = RGBColor(96, 96, 96, 255);
    Colors.ScrollButtonHot = RGBColor(218, 218, 218, 255);
    Colors.ScrollButtonHotText = RGBColor(0, 0, 0, 255);
    Colors.ScrollButtonClicked = RGBColor(96, 96, 96, 255);
    Colors.ScrollButtonClickedText = RGBColor(255, 255, 255, 255);
    
    Colors.ScrollBar = RGBColor(205, 205, 205, 255);
    Colors.ScrollBarBackground = RGBColor(240, 240, 240, 255);
    Colors.ScrollBarHot = RGBColor(116, 116, 116, 255);
    Colors.ScrollBarClicked = RGBColor(96, 96, 96, 255);
    
    render_group RenderGroup_;
    ZeroStruct(RenderGroup_);
    render_group *RenderGroup = &RenderGroup_;
    RenderGroup->Commands = Commands;
    RenderGroup->CurrentClipRectIndex = PushRenderCommandClipRectangle(RenderGroup, Rectangle2i(0, Commands->Width, 0, Commands->Height));
    RenderGroup->Arena = Arena;
    RenderGroup->Glyphs = UIState->Glyphs;
    RenderGroup->Colors = &Colors;
    
    PushRenderCommandClear(RenderGroup, 0.0f, Colors.Clear);
    
#if 0
    // NOTE(kstandbridge): Rectangle with clipping regions
    f32 Width = (f32)Commands->Width;
    f32 Height = (f32)Commands->Height;
    
    BeginClipRect(RenderGroup, Rectangle2(V2(Width*0.1f, Height*0.1f), V2(Width*0.9f,  Height*0.9f)));
    {    
        PushRenderCommandRectangle(RenderGroup, V4(1.0f, 0.0f, 0.0f, 1.0f),
                                   Rectangle2(V2(0.0f, 0.0f), V2(Width*0.5f, Height*0.5f)), 1.0f);
        
        PushRenderCommandRectangle(RenderGroup, V4(0.0f, 1.0f, 0.0f, 1.0f),
                                   Rectangle2(V2(Width*0.5f, 0.0f), V2(Width, Height*0.5f)), 1.0f);
        
        PushRenderCommandRectangle(RenderGroup, V4(0.0f, 0.0f, 1.0f, 1.0f),
                                   Rectangle2(V2(0.0f, Height*0.5f), V2(Width*0.5f, Height)), 1.0f);
        
        PushRenderCommandRectangle(RenderGroup, V4(1.0f, 1.0f, 0.0f, 1.0f),
                                   Rectangle2(V2(Width*0.5f, Height*0.5f), V2(Width, Height)), 1.0f);
    }
    
    EndClipRect(RenderGroup);
#endif
    
    UIState->MouseDown = Input->MouseButtons[MouseButton_Left].EndedDown;
    UIState->LastMouseP = UIState->MouseP;
    UIState->MouseP = V2(Input->MouseX, Input->MouseY);
    UIState->dMouseP = V2Subtract(UIState->LastMouseP, UIState->MouseP);
    
    temporary_memory TempMem = BeginTemporaryMemory(&UIState->TranArena);
    
    rectangle2 Bounds = Rectangle2(V2Set1(0.0f), V2((f32)Commands->Width, (f32)Commands->Height));
#if KENGINE_INTERNAL
    DEBUG_IF(ShowDebugTab)
    {
        DrawDebugGrid(DebugState, UIState, RenderGroup, Arena, TempMem.Arena, Input, Bounds);
    }
    else
    {
        //DrawAppGrid(AppState, UIState, RenderGroup, Arena, TempMem.Arena, Input, Bounds);
    }
#else
    DrawAppGrid(AppState, UIState, RenderGroup, Arena, TempMem.Arena, Input, Bounds);
#endif
    
    Interact(UIState, Input);
    UIState->ToExecute = UIState->NextToExecute;
    ClearInteraction(&UIState->NextToExecute);
    ClearInteraction(&UIState->NextHotInteraction);
    
#if KENGINE_INTERNAL
    if(Input->FKeyPressed[10])
    {
        GlobalDebugEventTable->Settings.ShowDebugTab = !GlobalDebugEventTable->Settings.ShowDebugTab;
    }
#endif
    
    EndTemporaryMemory(TempMem);
    CheckArena(&UIState->TranArena);
    CheckArena(Arena);
}

#include "kengine.h"

global platform_api Platform;

#include "kengine_render_group.c"
#include "kengine_ui.c"
#include "kengine_assets.c"

extern void
AppUpdateAndRender(app_memory *Memory, app_input *Input, app_offscreen_buffer *Buffer)
{
    Platform = Memory->PlatformAPI;
    
    app_state *AppState = (app_state *)Memory->Storage;
    
    if(!AppState->IsInitialized)
    {
        InitializeArena(&AppState->PermanentArena, Memory->StorageSize - sizeof(app_state), (u8 *)Memory->Storage + sizeof(app_state));
        
        AppState->TestBMP = LoadBMP(&AppState->PermanentArena, "test_tree.bmp");
        AppState->TestFont = Platform.DEBUGGetGlyphForCodePoint(&AppState->PermanentArena, 'K');
        AppState->TestP = V2(500.0f, 500.0f);
        
        SubArena(&AppState->Assets.Arena, &AppState->PermanentArena, Megabytes(32));
        
        SubArena(&AppState->TransientArena, &AppState->PermanentArena, Megabytes(32));
        
        AppState->UiState.Assets = &AppState->Assets;
        
        AppState->UiScale = V2(0.5f, 0.0f);
        
        AppState->TestString.Length = 1;
        AppState->TestString.SelectionStart = 1;
        AppState->TestString.SelectionEnd = 1;
        AppState->TestString.Size = 32;
        AppState->TestString.Data = PushSize(&AppState->PermanentArena, AppState->TestString.Size);
        AppState->TestString.Data[0] = ':';
        
        AppState->IsInitialized = true;
    }
    
    AppState->Time += Input->dtForFrame;
    
    temporary_memory RenderMem = BeginTemporaryMemory(&AppState->TransientArena);
    
    loaded_bitmap DrawBufferInteral;
    loaded_bitmap *DrawBuffer = &DrawBufferInteral;
    DrawBuffer->Memory = Buffer->Memory;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    
    render_group *RenderGroup = AllocateRenderGroup(RenderMem.Arena, Megabytes(4), DrawBuffer);
    
    if(Memory->ExecutableReloaded)
    {
        PushClear(RenderGroup, V4(0.6f, 0.0f, 0.6f, 1.0f));
    }
    else
    {
        PushClear(RenderGroup, RGBColor(56, 56, 56, 255));
    }
    
    ui_layout Layout = BeginUIFrame(&AppState->UiState, RenderMem.Arena, RenderGroup, Input, 8.0f, AppState->UiScale.X);
    
    BeginRow(&Layout, LayoutType_Auto);
    PushTextInputElement(&Layout, __COUNTER__, &AppState->TestString);
    PushSpacerElement(&Layout);
    if(PushButtonElement(&Layout, __COUNTER__, String("Click Me")))
    {
        AppState->TestP = V2Set1(0.0f);
    }
    PushButtonElement(&Layout, __COUNTER__, String("Top Right"));
    EndRow(&Layout);
    
    BeginRow(&Layout, LayoutType_Fill);
    PushSpacerElement(&Layout);
    PushScrollElement(&Layout, __COUNTER__, 
                      FormatString(RenderMem.Arena, "PadLeft %.2f %.2f PadRight", AppState->TestP.X, AppState->TestP.Y), 
                      &AppState->TestP);
    PushSpacerElement(&Layout);
    EndRow(&Layout);
    
    BeginRow(&Layout, LayoutType_Fill);
    PushScrollElement(&Layout, __COUNTER__, 
                      FormatString(RenderMem.Arena, "NoPad %.2f %.2f NoPad", AppState->TestP.X, AppState->TestP.Y), 
                      &AppState->TestP);
    EndRow(&Layout);
    
    BeginRow(&Layout, LayoutType_Fill);
    PushScrollElement(&Layout, __COUNTER__, 
                      FormatString(RenderMem.Arena, "NoPad %.2f %.2f PadRight", AppState->TestP.X, AppState->TestP.Y), 
                      &AppState->TestP);
    PushSpacerElement(&Layout);
    EndRow(&Layout);
    
    BeginRow(&Layout, LayoutType_Auto);
    PushSpacerElement(&Layout);
    PushButtonElement(&Layout, __COUNTER__, String("Foo"));
    PushSpacerElement(&Layout);
    PushStaticElement(&Layout, __COUNTER__, String("Bar"));
    PushSpacerElement(&Layout);
    PushButtonElement(&Layout, __COUNTER__, String("Bas"));
    PushSpacerElement(&Layout);
    EndRow(&Layout);
    
    BeginRow(&Layout, LayoutType_Fill);
    PushSpacerElement(&Layout);
    PushScrollElement(&Layout, __COUNTER__, 
                      FormatString(RenderMem.Arena, "PadLeft %.2f %.2f NoPad", AppState->TestP.X, AppState->TestP.Y), 
                      &AppState->TestP);
    EndRow(&Layout);
    
    BeginRow(&Layout, LayoutType_Auto);
    PushButtonElement(&Layout, __COUNTER__, String("Bottom Left"));
    PushSpacerElement(&Layout);
    PushScrollElement(&Layout, __COUNTER__, 
                      FormatString(RenderMem.Arena, "UI Scale: %.2f", AppState->UiScale.X),
                      &AppState->UiScale);
    PushSpacerElement(&Layout);
    PushButtonElement(&Layout, __COUNTER__, String("Bottom Right"));
    EndRow(&Layout);
    
    EndUIFrame(&Layout, Input);
    
#if 0
    v2 P = V2(500.0f, 400.0f);
    f32 Angle = 0.1f*AppState->Time;
    PushBitmap(RenderGroup, &AppState->TestBMP, (f32)AppState->TestBMP.Height, P, V4(1, 1, 1, 1), Angle);
#endif
    
#if 0
    v2 RectP = V2((f32)Buffer->Width / 2, (f32)Buffer->Height / 2);
    // TODO(kstandbridge): push circle?
    DrawCircle(DrawBuffer, RectP, V2Add(RectP, V2(50, 50)), V4(1, 1, 0, 1), Rectangle2i(0, Buffer->Width, 0, Buffer->Height));
#endif
    
    RenderGroupToOutput(RenderGroup);
    EndTemporaryMemory(RenderMem);
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
    CheckArena(&AppState->Assets.Arena);
}
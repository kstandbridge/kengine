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
    
    PushClear(RenderGroup, V4(0.3f, 0.0f, 0.3f, 1.0f));
    
    v2 MouseP = V2(Input->MouseX, Input->MouseY);
    ui_state *UiState = &AppState->UiState;
    UIUpdateAndRender(AppState, UiState, RenderMem.Arena, RenderGroup, MouseP);
    UIInteract(UiState, Input);
    ClearInteraction(&UiState->NextHotInteraction);
    UiState->LastMouseP = MouseP;
    
#if 0
    
    v2 P = V2(100.0f, 75.0f);
    
    v2 RectSize = V2((f32)AppState->TestBMP.Width, (f32)AppState->TestBMP.Height);
    
    PushRect(RenderGroup, P, RectSize, V4(1, 1, 0, 1));
    PushBitmap(RenderGroup, &AppState->TestBMP, (f32)AppState->TestBMP.Height, P, V4(1, 1, 1, 1), 0.0f);
    
    P.X += 250;
    
    rectangle2 TextBounds = GetTextSize(AppState, RenderGroup, P, 1.0f, String("Hello"));
    v2 MouseP = V2(Input->MouseX, Input->MouseY);
    
    b32 IsHot = IsInRectangle(TextBounds, MouseP);
    v4 BackColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 1, 1, 1);
    PushRect(RenderGroup, TextBounds.Min, V2Subtract(TextBounds.Max, TextBounds.Min), BackColor);
    WriteLine(AppState, RenderGroup, P, 1.0f, String("Hello"));
    
#if 0
    f32 Angle = 0.1f*AppState->Time;
    PushBitmap(RenderGroup, &AppState->TestBMP, (f32)AppState->TestBMP.Height, P, V4(1, 1, 1, 1), Angle);
#endif
    
#endif
    
    RenderGroupToOutput(RenderGroup);
    
#if 0    
    v2 RectP = V2((f32)Buffer->Width / 2, (f32)Buffer->Height / 2);
    DrawCircle(DrawBuffer, RectP, V2Add(RectP, V2(50, 50)), V4(1, 1, 0, 1), Rectangle2i(0, Buffer->Width, 0, Buffer->Height));
#endif
    
    EndTemporaryMemory(RenderMem);
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
}
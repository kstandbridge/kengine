#include "kengine.h"

global platform_api Platform;




#if KENGINE_INTERNAL
global app_memory *GlobalDebugMemory;
#endif

#include "kengine_render_group.c"
#include "kengine_ui.c"
#include "kengine_assets.c"
#include "kengine_debug.c"

extern void
AppUpdateAndRender(app_memory *Memory, app_input *Input, app_offscreen_buffer *Buffer)
{
    Platform = Memory->PlatformAPI;
    
#if KENGINE_INTERNAL
    GlobalDebugMemory = Memory;
#endif
    
    SetColors();
    
    app_state *AppState = (app_state *)Memory->Storage;
    
    if(!AppState->IsInitialized)
    {
        InitializeArena(&AppState->PermanentArena, Memory->StorageSize - sizeof(app_state), (u8 *)Memory->Storage + sizeof(app_state));
        
        AppState->TestBMP = LoadBMP(&AppState->PermanentArena, "test_tree.bmp");
        AppState->TestFont = Platform.DEBUGGetGlyphForCodePoint(&AppState->PermanentArena, 'K');
        AppState->TestP = V2(500.0f, 500.0f);
        
        SubArena(&AppState->Assets.Arena, &AppState->PermanentArena, Megabytes(512));
        
        SubArena(&AppState->TransientArena, &AppState->PermanentArena, Megabytes(256));
        
        //AppState->UiState.Assets = &AppState->Assets;
        
        AppState->UiScale = V2(0.2f, 0.0f);
        
        AppState->ShowLocalWorlds = true;
        AppState->TestString.Length = 1;
        AppState->TestString.SelectionStart = 1;
        AppState->TestString.SelectionEnd = 1;
        AppState->TestString.Size = 32;
        AppState->TestString.Data = PushSize(&AppState->PermanentArena, AppState->TestString.Size);
        AppState->TestString.Data[0] = ':';
        
        string TheString = String("Lorem ipsum dolor sit amet, consectetur adipiscing elit. \nDuis mattis iaculis nunc, vitae laoreet dolor. Sed condimentum,\n nulla venenatis interdum gravida, metus magna vestibulum urna,\n nec euismod lectus dui at mauris. Aenean venenatis ut ligula\n sit amet ullamcorper. Vivamus in magna tristique, sodales\n magna ac, sodales purus. Proin ut est ante. Quisque et \n sollicitudin velit. Fusce id elementum augue, non maximus\n magna. Aliquam finibus erat sit amet nibh pharetra, eget pharetra\n est convallis. Nam sodales tellus imperdiet ante hendrerit, ut\ntristique ex euismod. Morbi gravida elit orci, at ultrices\n turpis efficitur ac. Fusce dapibus auctor lorem quis tempor.\nSuspendisse at egestas justo. Nam bibendum ultricies molestie.\n Aenean lobortis vehicula ante, elementum eleifend eros congue\n eget. Phasellus placerat varius nunc non faucibus.");
        AppState->LongString.Length = (u32)TheString.Size;
        AppState->LongString.Size = TheString.Size;
        AppState->LongString.Data = TheString.Data;
        AppState->LongString.SelectionStart = 10;
        AppState->LongString.SelectionEnd = 5;
        
        AppState->IsInitialized = true;
    }
    
#if KENGINE_INTERNAL
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    
    if(!DebugState->IsInitialized)
    {
        InitializeArena(&DebugState->Arena, Memory->DebugStorageSize - sizeof(debug_state), (u8 *)Memory->DebugStorage + sizeof(debug_state));
        
        DebugState->Assets = &AppState->Assets;
        
        DebugState->LeftEdge = 20.0f;
        DebugState->AtY = 0.0f;
        DebugState->FontScale = 0.2f;
        
        DebugState->IsInitialized = true;
    }
    
#endif
    
    AppState->Time += Input->dtForFrame;
    
#if 1
    loaded_bitmap DrawBufferInteral;
    loaded_bitmap *DrawBuffer = &DrawBufferInteral;
    DrawBuffer->Memory = Buffer->Memory;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
#else
    loaded_bitmap DrawBufferInteral;
    loaded_bitmap *DrawBuffer = &DrawBufferInteral;
    s32 Y = 320;
    s32 X = 240;
    DrawBuffer->Memory = (u8 *)Buffer->Memory + X*BITMAP_BYTES_PER_PIXEL + Y*Buffer->Pitch;
    DrawBuffer->Width = 1024;
    DrawBuffer->Height = 768;
    DrawBuffer->Pitch = Buffer->Pitch;
#endif
    
#if KENGINE_INTERNAL
    DEBUGStart(DrawBuffer);
#endif
    
    {
        temporary_memory TempMem = BeginTemporaryMemory(&AppState->TransientArena);
        
        render_group *RenderGroup = AllocateRenderGroup(TempMem.Arena, Megabytes(4), DrawBuffer);
        
        if(Memory->ExecutableReloaded)
        {
            PushClear(RenderGroup, V4(0.3f, 0.0f, 0.3f, 1.0f));
        }
        else
        {
            PushClear(RenderGroup, V4Set1(1.0f));
        }
        
        
#if 0
        v2 P = V2(Buffer->Width*0.5f, Buffer->Height*0.5f);
        f32 Angle = 0.1f*AppState->Time;
        PushBitmap(RenderGroup, &AppState->TestBMP, (f32)AppState->TestBMP.Height, P, V4(1, 1, 1, 1), Angle);
#endif
        
#if 0
        v2 RectP = V2((f32)Buffer->Width / 2, (f32)Buffer->Height / 2);
        // TODO(kstandbridge): push circle?
        DrawCircle(DrawBuffer, RectP, V2Add(RectP, V2(50, 50)), V4(1, 1, 0, 1.0f), Rectangle2i(0, Buffer->Width, 0, Buffer->Height));
#endif
        
        
        tile_render_work Work;
        ZeroStruct(Work);
        Work.Group = RenderGroup;
        Work.ClipRect.MinX = 0;
        Work.ClipRect.MaxX = DrawBuffer->Width;
        Work.ClipRect.MinY = 0;
        Work.ClipRect.MaxY = DrawBuffer->Height;
        TileRenderWorkThread(&Work);
        EndTemporaryMemory(TempMem);
    }
    
    
    temporary_memory TempMem = BeginTemporaryMemory(&AppState->TransientArena);
    
    ui_layout *Layout = BeginUIFrame(TempMem.Arena, &AppState->UiState, Input, &AppState->Assets, 0.2f, 4.0f, DrawBuffer);
    
#if 0
    
    BeginRow(Layout);
    {    
        Spacer(Layout);
        Spacer(Layout);
        Spacer(Layout);
        SetControlWidth(Layout, 24.0f);
        
        BeginRow(Layout);Spacer(Layout);EndRow(Layout);
    }
    EndRow(Layout);
    
#else
    
    
    BeginRow(Layout);
    {
        SetRowHeight(Layout, 28.0f);
        Label(Layout, String("Show:"));
        Checkbox(Layout, String("Empty Worlds"), &AppState->ShowEmptyWorlds); // Editable bool
        Checkbox(Layout, String("Local"), &AppState->ShowLocalWorlds); // Editable bool
        Checkbox(Layout, String("Available"), &AppState->ShowAvailableWorlds); // Editable bool
        Spacer(Layout);
        Label(Layout, String("Filter:"));
        Textbox(Layout, &AppState->FilterText);
    }
    EndRow(Layout);
    
    BeginRow(Layout);
    {
        // NOTE(kstandbridge): Listview Worlds
        Spacer(Layout);
    }
    EndRow(Layout);
    
    BeginRow(Layout);
    {
        BeginRow(Layout);
        {
            // NOTE(kstandbridge): Listview Builds
            Spacer(Layout);
        }
        EndRow(Layout);
        
        Splitter(Layout, &UserSettings->BuildRunSplitSize);
        SetControlWidth(Layout, 12.0f);
        
        BeginRow(Layout);
        Checkbox(Layout, String("Edit run params"), &AppState->EditRunParams);
        EndRow(Layout);
        
        BeginRow(Layout);
        DropDown(Layout, "default");
        EndRow(Layout);
        
        BeginRow(Layout);
        {
            Textbox(Layout, "thing.exe");
            Button(Layout, "Copy");
            SetControlWidth(Layout, 24.0f);
        }
        EndRow(Layout);
        
        BeginRow(Layout);
        {
            Label(Layout, String("Sync command"));
            Button(Layout, "Copy");
            SetControlWidth(Layout, 24.0f);
        }
        EndRow(Layout);
        
        BeginRow(Layout);
        {
            Button(Layout, "Run");
            Button(Layout, "Sync");
            Button(Layout, "Cancel");
            Button(Layout, "Clean");
            Button(Layout, "Open Folder");
        }
        EndRow(Layout);
        
        BeginRow(Layout);
        {
            Label(Layout, String("DownloadPath"));
            Button(Layout, "Copy");
            SetControlWidth(Layout, 24.0f);
        }
        EndRow(Layout);
        
    }
    EndRow(Layout);
    
    
    BeginRow(Layout);
    SetRowHeight(Layout, 124.0f);
    Textbox(Layout, &AppState->LogMessages, &AppSate->SelectedMessage);
    EndRow(Layout);
    
    BeginRow(Layout);
    {
        SetRowHeight(Layout, 28.0f);
        Button(Layout, "Settings");
        Button(Layout, "Settings");
        Button(Layout, "Settings");
        Spacer(Layout);
        Button(Layout, "Open Remote Config");
        Button(Layout, "Open Log");
    }
    EndRow(Layout);
    
    
    
    
#endif
    
    EndUIFrame(&AppState->UiState, Layout, Input);
    
    EndTemporaryMemory(TempMem);
    
#if KENGINE_INTERNAL
    DEBUGEnd();
#endif
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
    CheckArena(&AppState->Assets.Arena);
}

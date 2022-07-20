#include "kengine.h"

global platform_api Platform;




#if KENGINE_INTERNAL
global app_memory *GlobalDebugMemory;
#endif

#include "kengine_render_group.c"
#include "kengine_interface.c"
#include "kengine_assets.c"
#include "kengine_debug.c"

extern void
AppUpdateFrame(app_memory *Memory, app_input *Input, app_render_commands *RenderCommands)
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
        
        //AppState->TestBMP = LoadBMP(&AppState->PermanentArena, "test_tree.bmp");
        AppState->TestFont = Platform.DEBUGGetGlyphForCodePoint(&AppState->PermanentArena, 'K');
        AppState->TestP = V2(500.0f, 500.0f);
        
        SubArena(&AppState->Assets.Arena, &AppState->PermanentArena, Megabytes(512));
        
        SubArena(&AppState->TransientArena, &AppState->PermanentArena, Megabytes(256));
        
        AppState->UiScale = V2(0.2f, 0.0f);
        
        AppState->ShowLocalWorlds = true;
        
        AppState->FilterText.Length = 1;
        AppState->FilterText.SelectionStart = 1;
        AppState->FilterText.SelectionEnd = 1;
        AppState->FilterText.Size = 32;
        AppState->FilterText.Data = PushSize(&AppState->PermanentArena, AppState->FilterText.Size);
        
        AppState->LaunchParams.SelectionStart = 10;
        AppState->LaunchParams.SelectionEnd = 5;
        
        AppState->IsInitialized = true;
    }
    
    {        
        string TheString = String("Lorem ipsum dolor sit amet, consectetur adipiscing elit. \nDuis mattis iaculis nunc, vitae laoreet dolor. Sed condimentum,\n nulla venenatis interdum gravida, metus magna vestibulum urna,\n nec euismod lectus dui at mauris. Aenean venenatis ut ligula\n sit amet ullamcorper. Vivamus in magna tristique, sodales\n magna ac, sodales purus. Proin ut est ante. Quisque et \n sollicitudin velit. Fusce id elementum augue, non maximus\n magna. Aliquam finibus erat sit amet nibh pharetra, eget pharetra\n est convallis. Nam sodales tellus imperdiet ante hendrerit, ut\ntristique ex euismod. Morbi gravida elit orci, at ultrices\n turpis efficitur ac. Fusce dapibus auctor lorem quis tempor.\nSuspendisse at egestas justo. Nam bibendum ultricies molestie.\n Aenean lobortis vehicula ante, elementum eleifend eros congue\n eget. Phasellus placerat varius nunc non faucibus.");
        
        AppState->LaunchParams.Length = (u32)TheString.Size;
        AppState->LaunchParams.Size = TheString.Size;
        AppState->LaunchParams.Data = TheString.Data;
    }
    
    
#if KENGINE_INTERNAL
    debug_state *DebugState = (debug_state *)Memory->DebugStorage;
    
    if(!DebugState->IsInitialized)
    {
        InitializeArena(&DebugState->Arena, Memory->DebugStorageSize - sizeof(debug_state), (u8 *)Memory->DebugStorage + sizeof(debug_state));
        
        DebugState->Assets = &AppState->Assets;
        DebugState->RenderCommands = RenderCommands;
        DebugState->LeftEdge = 20.0f;
        
        DebugState->IsInitialized = true;
    }
    
#endif
    
    AppState->Time += Input->dtForFrame;
    
#if KENGINE_INTERNAL
    DEBUGStart();
#endif
    
    temporary_memory TempMem = BeginTemporaryMemory(&AppState->TransientArena);
    
    interface_state *InterfaceState = &AppState->InterfaceState;
    
    layout Layout = BeginUIFrame(TempMem.Arena, InterfaceState, Input, &AppState->Assets, 1.0f, 2.0f, RenderCommands);
    
    // TODO(kstandbridge): should be able to hide this somewhere
    Layout.CurrentElement = &Layout.SentinalElement;
    
#if 1
    
    DEBUGTextLine("kstandbridge VAWAV whatever");
    
    
    
#if 0
    BeginRow(&Layout);
    SetRowFill(&Layout);
    Textbox(&Layout, &AppState->LaunchParams);
    SetControlPadding(&Layout, Layout.DefaultPadding, 0.0f, Layout.DefaultPadding, Layout.DefaultPadding);
    
    f32 LineAdvanceForFrame = Layout.DefaultRowHeight*3.0f*Input->dtForFrame;
    BeginRow(&Layout);
    SetRowWidth(&Layout, Layout.DefaultRowHeight);
    if(ContinousButton(InterfaceState, &Layout, String("U")))
    {
        AppState->LaunchParams.Offset.Y += LineAdvanceForFrame;
    }
    SetControlPadding(&Layout, Layout.DefaultPadding, Layout.DefaultPadding, 0.0f, 0.0f);
    EndRow(&Layout);
    
    BeginRow(&Layout);
    SetRowFill(&Layout);
    VerticleSlider(&Layout, &AppState->LaunchParams.Offset.Y);
    SetControlPadding(&Layout, 0.0f, Layout.DefaultPadding, 0.0f, 0.0f);
    EndRow(&Layout);
    
    BeginRow(&Layout);
    if(ContinousButton(InterfaceState, &Layout, String("D")))
    {
        AppState->LaunchParams.Offset.Y -= LineAdvanceForFrame;
    }
    SetControlPadding(&Layout, 0.0f, Layout.DefaultPadding, Layout.DefaultPadding, 0.0f);
    EndRow(&Layout);
    
    EndRow(&Layout);
#endif
    
#else
    
    BeginRow(&Layout);
    {
        Label(&Layout, String("Show:"));
        SetControlWidth(&Layout, 64.0f);
        Checkbox(&Layout, String("Empty Worlds"), &AppState->ShowEmptyWorlds); // Editable bool
        SetControlWidth(&Layout, 158.0f);
        Checkbox(&Layout, String("Local"), &AppState->ShowLocalWorlds); // Editable bool
        SetControlWidth(&Layout, 80.0f);
        Checkbox(&Layout, String("Available"), &AppState->ShowAvailableWorlds); // Editable bool
        SetControlWidth(&Layout, 104.0f);
        Spacer(&Layout);
        Label(&Layout, String("Filter:"));
        SetControlWidth(&Layout, 48.0f);
        Textbox(&Layout, &AppState->FilterText);
        SetControlWidth(&Layout, 256.0f);
    }
    EndRow(&Layout);
    
    BeginRow(&Layout);
    {
        //SetRowFill(&Layout);
        // NOTE(kstandbridge): Listview Worlds
        Spacer(&Layout);
    }
    EndRow(&Layout);
    
    BeginRow(&Layout);
    {
        SetRowFill(&Layout);
        
        BeginRow(&Layout);
        {
            SetRowFill(&Layout);
            // NOTE(kstandbridge): Listview Builds
            Spacer(&Layout);
        }
        EndRow(&Layout);
        
        Splitter(&Layout, &UserSettings->BuildRunSplitSize);
        SetControlWidth(&Layout, 12.0f + Layout.DefaultPadding*2.0f);
        
        BeginRow(&Layout);
        Checkbox(&Layout, String("Edit run params"), &AppState->EditRunParams);
        EndRow(&Layout);
        
        BeginRow(&Layout);
        DropDown(&Layout, "default");
        EndRow(&Layout);
        
        BeginRow(&Layout);
        {
            SetRowFill(&Layout);
            Textbox(&Layout, &AppState->LaunchParams);
        }
        EndRow(&Layout);
        
        BeginRow(&Layout);
        {
            Label(&Layout, String("Sync command"));
            Button(InterfaceState, &Layout, String("Copy"));
            SetControlWidth(&Layout, 24.0f);
        }
        EndRow(&Layout);
        
        BeginRow(&Layout);
        {
            if(Button(InterfaceState, &Layout, String("Run")))
            {
                AppState->LaunchParams.Offset = V2Set1(0);
                DEBUGTextLine("Pressed");
            }
            Button(InterfaceState, &Layout, String("Sync"));
            Button(InterfaceState, &Layout, String("Cancel"));
            Button(InterfaceState, &Layout, String("Clean"));
            Button(InterfaceState, &Layout, String("Open Folder"));
        }
        EndRow(&Layout);
        
        BeginRow(&Layout);
        {
            Label(&Layout, String("DownloadPath"));
            Button(InterfaceState, &Layout, String("Copy"));
            SetControlWidth(&Layout, 24.0f);
        }
        EndRow(&Layout);
        
    }
    EndRow(&Layout);
    
    
    BeginRow(&Layout);
    {
        // NOTE(kstandbridge): List of logs?
        Spacer(&Layout);
    }
    EndRow(&Layout);
    
    BeginRow(&Layout);
    {
        Button(InterfaceState, &Layout, String("Settings"));
        Button(InterfaceState, &Layout, String("Feedback"));
        Button(InterfaceState, &Layout, String("Delete 1 Expired Build"));
        Spacer(&Layout);
        Button(InterfaceState, &Layout, String("Open Remote Config"));
        Button(InterfaceState, &Layout, String("Open Log"));
    }
    EndRow(&Layout);
    
#endif
    
    EndUIFrame(InterfaceState, &Layout, Input);
    
    EndTemporaryMemory(TempMem);
    
#if KENGINE_INTERNAL
    DEBUGEnd();
#endif
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
    CheckArena(&AppState->Assets.Arena);
}

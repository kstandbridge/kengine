#include "kengine.h"

/* NOTE(kstandbridge): 

*/

#ifndef VERSION
#define VERSION 0
#endif // VERSION

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
#include "kengine_telemetry.c"
#include "kengine_render_group.c"
//#include "kengine_ui.c"
#include "kengine_html_parser.c"
#include "kengine_json_parser.c"
#include "kengine_sha512.c"
#include "kengine_eddsa.c"
#include "kengine_random.c"

#if KENGINE_INTERNAL
//#include "kengine_debug_ui.c"
#endif

#include "main.c"

extern void
AppTick_(app_memory *AppMemory, render_group *Group, app_input Input)
{
#if KENGINE_INTERNAL
    Platform = AppMemory->PlatformAPI;
    GlobalTelemetryState = AppMemory->TelemetryState;
    GlobalDebugEventTable = AppMemory->DebugEventTable;
#endif
    
#if 0
    // NOTE(kstandbridge): Populate rects
    {
#define BOX_WIDTH 15
#define BOX_HEIGHT 15
#define BOX_PADDING 3
        u32 Columns = Group->Width / (BOX_WIDTH + BOX_PADDING);
        u32 Rows = Group->Height / (BOX_HEIGHT + BOX_PADDING);
        u32 AtX = BOX_PADDING;
        u32 AtY = BOX_PADDING;
        
        for(u32 Row = 0;
            Row < Rows;
            ++Row)
        {
            for(u32 Column = 0;
                Column < Columns;
                ++Column)
            {
                render_command *Command = PushRenderCommand(Group, RenderCommand_Rect);
                Command->Rect.Offset = V3(AtX, AtY, 1.0f);
                Command->Rect.Size = V2(BOX_WIDTH, BOX_HEIGHT);
                Command->Rect.Color = V4(0.3f, 0.5f, 0.2f, 1.0f);
                
                AtX += BOX_WIDTH + BOX_PADDING;
            }
            AtX = BOX_PADDING;
            AtY += BOX_HEIGHT + BOX_PADDING;
        }
    }
#endif
    
#if 0
    // NOTE(kstandbridge): Populate Glyphs
    {
        string LoremIpsum = String("Lorem Ipsum is simply dummy text of the printing and typesetting\nindustry. Lorem Ipsum has been the industry's standard dummy\ntext ever since the 1500s, when an unknown printer took a galley\nof type and scrambled it to make a type specimen book. It has\nsurvived not only five centuries, but also the leap into electronic\ntypesetting, remaining essentially unchanged. It was popularised in\nthe 1960s with the release of Letraset sheets containing Lorem\nIpsum passages, and more recently with desktop publishing\nsoftware like Aldus PageMaker including versions of Lorem Ipsum.");
        
        render_command *Command = PushRenderCommand(Group, RenderCommand_Text);
        Command->Text.Offset = V3(2.0f, 2.0f, 1.0f);
        Command->Text.Size = V2(1.0f, 1.0f);
        Command->Text.Color = V4(0.0f, 0.0f, 0.0f, 1.0f);
        Command->Text.Text = LoremIpsum;
        
        Command = PushRenderCommand(Group, RenderCommand_Text);
        Command->Text.Offset = V3(0.0f, 0.0f, 1.0f);
        Command->Text.Size = V2(1.0f, 1.0f);
        Command->Text.Color = V4(1.0f, 1.0f, 1.0f, 1.0f);
        Command->Text.Text = LoremIpsum;
        
    }
#endif
    
    v2 MouseP = V2(Input.MouseX, Input.MouseY);
    rectangle2 Rect = Rectangle2(V2(10, 10), V2(100, 100));
    {
        b32 Hot = Rectangle2IsIn(Rect, MouseP);
        v4 Color = Hot ? V4(1, 0, 0, 1) : V4(0, 1, 0, 1);
        render_command *Command = PushRenderCommand(Group, RenderCommand_Rect);
        Command->Rect.Offset = V3(Rect.Min.X, Rect.Min.Y, 4.0f);
        Command->Rect.Size = V2Subtract(Rect.Max, Rect.Min);
        Command->Rect.Color = Color;
    }
    
    {
        render_command *Command = PushRenderCommand(Group, RenderCommand_Rect);
        Command->Rect.Offset = V3(Input.MouseX, Input.MouseY, 5.0f);
        Command->Rect.Size = V2(10, 10);
        Command->Rect.Color = V4(1.0f, 1.0f, 1.0f, 1.0f);
        
    }
    
    
}

#if 0
extern void
#if defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
AppTick_(app_memory *AppMemory, f32 dtForFrame)
#else 
AppTick_(app_memory *AppMemory, render_group *Group)
#endif 
{
#if KENGINE_INTERNAL
    Platform = AppMemory->PlatformAPI;
    GlobalTelemetryState = AppMemory->TelemetryState;
    GlobalDebugEventTable = AppMemory->DebugEventTable;
#endif
    app_state *AppState = AppMemory->AppState;
    if(!AppState)
    {
        AppState = AppMemory->AppState = BootstrapPushStruct(app_state, Arena);
#if KENGINE_INTERNAL
        GlobalDebugEventTable->Settings.ShowDebugTab = true;
#endif
        AppInit(AppMemory);
        
        u32 ArgCount = AppMemory->ArgCount;
        string *Args = AppMemory->Args;
        
        if(ArgCount > 1)
        {
            string Command = Args[1];
            if(ArgCount == 2)
            {
                Args = 0;
            }
            else
            {
                Args += 2;
            }
            ArgCount -= 2;
            AppHandleCommand(AppMemory, Command, ArgCount, Args);
        }
    }
    
#if defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
    AppTick(AppMemory, dtForFrame);
#else
    
    ui_state *UIState = AppMemory->UIState;
    if(!UIState)
    {
        UIState = AppMemory->UIState = BootstrapPushStruct(ui_state, PermArena);
        
        // NOTE(kstandbridge): GetVerticleAdvance will return 0 if no glyphs have been loaded
        Platform.GetGlyphForCodePoint(&UIState->TranArena, 'K');
        AppMemory->UIState->LineAdvance = Platform.GetVerticleAdvance();
    }
    
#if KENGINE_INTERNAL
    debug_state *DebugState = AppMemory->DebugState;
#endif
    
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
    RenderGroup->Arena = &AppState->Arena;
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
    
#if KENGINE_INTERNAL
    DEBUG_IF(ShowDebugTab)
    {
        rectangle2 Bounds = Rectangle2(V2Set1(0.0f), V2((f32)Commands->Width, (f32)Commands->Height));
        DrawDebugGrid(DebugState, UIState, RenderGroup, &AppState->Arena, TempMem.Arena, Input, Bounds);
    }
    else
    {
        AppTick(AppMemory, dtForFrame);
    }
#else
    AppTick(AppMemory, dtForFrame);
#endif
    
    Interact(UIState, Input, dtForFrame);
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
    CheckArena(&AppState->Arena);
    
#endif // defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
}
#endif

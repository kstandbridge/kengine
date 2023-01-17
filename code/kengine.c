#include "kengine.h"

#ifndef VERSION
#define VERSION 0
#endif // VERSION

#if KENGINE_INTERNAL
platform_api Platform;
global debug_event_table *GlobalDebugEventTable;
#endif

#include "kengine_sort.c"
#include "kengine_telemetry.c"
#include "kengine_render_group.c"
#include "kengine_html_parser.c"
#include "kengine_json_parser.c"
#include "kengine_sha512.c"
#include "kengine_eddsa.c"
#include "kengine_random.c"

#if KENGINE_INTERNAL
//#include "kengine_debug_ui.c"
#endif

#include "main.c"

inline b32
ElementIsHot(control_element Element, v2 MouseP)
{
    b32 Result = false;
    
    v2 Offset = V2(Element.Offset.X, Element.Offset.Y);
    
    rectangle2 Bounds = Rectangle2(Offset, V2Add(Offset, Element.Size));
    Result = Rectangle2IsIn(Bounds, MouseP);
    
    return Result;
}

internal void
BeginGrid(ui_state *UiState, rectangle2 Bounds, u32 Columns, u32 Rows)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    
    ui_grid *Grid = PushStruct(Arena, ui_grid);
    Grid->Prev= Frame->CurrentGrid;
    Frame->CurrentGrid = Grid;
    
    Grid->Bounds = Bounds;
    Grid->Columns = Columns;
    Grid->Rows = Rows;
    
    Grid->ColumnWidth = (Bounds.Max.X - Bounds.Min.X) / Grid->Columns;
    Grid->RowHeight = (Bounds.Max.Y - Bounds.Min.Y) / Grid->Rows;
}

internal void
EndGrid(ui_state *UiState)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    Frame->CurrentGrid = Frame->CurrentGrid->Prev;
}

internal void
GridSetRowHeight(ui_state *UiState, u32 Row, f32 Height)
{
}

internal void
GridSetColumnWidth(ui_state *UiState, u32 Column, f32 Width)
{
}

internal rectangle2
GridGetColumnBounds(ui_state *UiState, u32 Column, u32 Row)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    v2 Min = Grid->Bounds.Min;
    Min.X += Column * Grid->ColumnWidth;
    Min.Y += Row * Grid->RowHeight;
    
    v2 Max = V2Add(Min, V2(Grid->ColumnWidth, Grid->RowHeight));
    
    rectangle2 Result = Rectangle2(Min, Max);
    return Result;
}

internal b32
Button(ui_state *UiState, u32 Column, u32 Row, ui_id Id, char *Format, ...)
{
    b32 Result = false;
    
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_ImmediateButton,
        .Target = 0
    };
    
    Result = InteractionsAreEqual(Interaction, UiState->ToExecute);
    
    rectangle2 Bounds = GridGetColumnBounds(UiState, Column, Row);
    
    v4 ButtonColor = V4(1.0f, 0.5f, 0.0f, 1.0f);
    if(Rectangle2IsIn(Bounds, UiState->MouseP))
    {
        UiState->NextHotInteraction = Interaction;
        ButtonColor = V4(0.5f, 1.0f, 0.0f, 1.0f);
    }
    
    PushRenderCommandRect(RenderGroup, Bounds, 1.0f, ButtonColor);
    
    PushRenderCommandText(RenderGroup, 1.0f, Bounds.Min, 2.0f, V4(0.0f, 0.0f, 0.0f, 1.0f),
                          FormatString(Arena, Format));
    
    return Result;
}

internal void
Label(ui_state *UiState, u32 Column, u32 Row, ui_id Id, char *Format, ...)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    rectangle2 Bounds = GridGetColumnBounds(UiState, Column, Row);
    
    format_string_state StringState = BeginFormatString();
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    string Message = EndFormatString(&StringState, &UiState->Arena);
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_NOP,
        .Target = 0
    };
    
    
    v4 ButtonColor = V4(1.0f, 0.5f, 0.0f, 1.0f);
    if(Rectangle2IsIn(Bounds, UiState->MouseP))
    {
        UiState->NextHotInteraction = Interaction;
        ButtonColor = V4(0.5f, 1.0f, 0.0f, 1.0f);
    }
    
    PushRenderCommandRect(RenderGroup, Bounds, 1.0f, ButtonColor);
    
    
    PushRenderCommandText(RenderGroup, 1.0f, Bounds.Min, 2.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Message);
}

internal void
Slider(ui_state *UiState, u32 Column, u32 Row, ui_id Id, f32 *Target, char *Format, ...)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    rectangle2 Bounds = GridGetColumnBounds(UiState, Column, Row);
    
    format_string_state StringState = BeginFormatString();
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    string Message = EndFormatString(&StringState, &UiState->Arena);
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_Draggable,
        .Target = Target
    };
    
    
    v4 ButtonColor = V4(1.0f, 0.5f, 0.0f, 1.0f);
    if(Rectangle2IsIn(Bounds, UiState->MouseP))
    {
        UiState->NextHotInteraction = Interaction;
        ButtonColor = V4(0.5f, 1.0f, 0.0f, 1.0f);
    }
    
    if(InteractionsAreEqual(Interaction, UiState->Interaction))
    {
        *Target += 0.01f*UiState->dMouseP.X;
    }
    
    PushRenderCommandRect(RenderGroup, Bounds, 1.0f, ButtonColor);
    
    
    PushRenderCommandText(RenderGroup, 1.0f, Bounds.Min, 2.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Message);
}

extern void
AppTick_(app_memory *AppMemory, render_group *RenderGroup, app_input *Input)
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
    }
    
    ui_state *UiState = AppMemory->UiState;
    if(!UiState)
    {
        UiState = AppMemory->UiState = BootstrapPushStruct(ui_state, Arena);
    }
    
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
    
    temporary_memory MemoryFlush = BeginTemporaryMemory(&AppState->Arena);
    
    ui_frame _UiFrame = 
    {
        .Arena = MemoryFlush.Arena,
        .RenderGroup = RenderGroup,
        .Input = Input,
        .CurrentGrid = 0,
    };
    
    UiState->LastMouseP = UiState->MouseP;
    UiState->MouseP = Input->MouseP;
    UiState->dMouseP = V2Subtract(UiState->MouseP, UiState->LastMouseP);
    
    UiState->Frame = &_UiFrame;
    // NOTE(kstandbridge): UpdateAndRender
    {    
        rectangle2 Bounds = Rectangle2(V2Set1(0.0f), V2(RenderGroup->Width, RenderGroup->Height));
        BeginGrid(UiState, Bounds, 2, 2);
        {
            GridSetRowHeight(UiState, 0, 24.0f);
            GridSetColumnWidth(UiState, 0, 128.0f);
            
            if(Button(UiState, 0, 0, GenerateUIId(&AppState->SomeValue), "Add"))
            {
                AppState->SomeValue += 10.0f;
                LogInfo("Added");
            }
            
            if(Button(UiState, 1, 0, GenerateUIId(&AppState->SomeValue), "Subtract"))
            {
                AppState->SomeValue -= 10.0f;
                LogInfo("Subtracted");
            }
            
            Label(UiState, 0, 1, GenerateUIId(&AppState->SomeValue), "Value %f", AppState->SomeValue);
            Slider(UiState, 1, 1, GenerateUIId(&AppState->SomeValue), &AppState->SomeValue, "Slide");
        }
        EndGrid(UiState);
    }
    
    UiState->ToExecute = UiState->NextToExecute;
    ClearInteraction(&UiState->NextToExecute);
    
    // NOTE(kstandbridge): Interact
    {
        u32 TransistionCount = Input->MouseButtons[MouseButton_Left].HalfTransitionCount;
        b32 MouseButton = Input->MouseButtons[MouseButton_Left].EndedDown;
        if(TransistionCount % 2)
        {
            MouseButton = !MouseButton;
        }
        
        for(u32 TransitionIndex = 0;
            TransitionIndex <= TransistionCount;
            ++TransitionIndex)
        {
            b32 MouseMove = false;
            b32 MouseDown = false;
            b32 MouseUp = false;
            if(TransitionIndex == 0)
            {
                MouseMove = true;
            }
            else
            {
                MouseDown = MouseButton;
                MouseUp = !MouseButton;
            }
            
            b32 EndInteraction = false;
            
            switch(UiState->Interaction.Type)
            {
                case UI_Interaction_ImmediateButton:
                {
                    if(MouseUp)
                    {
                        UiState->NextToExecute = UiState->Interaction;
                        EndInteraction = true;
                    }
                } break;
                
                case UI_Interaction_None:
                {
                    UiState->HotInteraction = UiState->NextHotInteraction;
                    if(MouseDown)
                    {
                        UiState->Interaction = UiState->HotInteraction;
                    }
                } break;
                
                default:
                {
                    if(MouseUp)
                    {
                        EndInteraction = true;
                    }
                } break;
            }
            
            if(EndInteraction)
            {
                ClearInteraction(&UiState->Interaction);
            }
            
            MouseButton = !MouseButton;
        }
    }
    
    ClearInteraction(&UiState->NextHotInteraction);
    
    EndTemporaryMemory(MemoryFlush);
    
}
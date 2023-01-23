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

typedef enum text_op
{
    TextOp_Draw,
    TextOp_Size,
} text_op;

internal rectangle2
TextOp_(ui_state *UiState, v2 Offset, f32 Depth, f32 Scale, v4 Color, string Text, text_op Op)
{
    Scale = 0.5f;
    rectangle2 Result = Rectangle2(Offset, Offset);
    
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    
    f32 AtX = Offset.X;
    f32 AtY = Offset.Y + UiState->FontScale*UiState->FontAscent*Scale;
    
    for(umm Index = 0;
        Index < Text.Size;
        ++Index)
    {
        u32 CodePoint = Text.Data[Index];
        
        if(CodePoint == '\n')
        {
            AtY += UiState->FontScale*UiState->FontAscent*Scale;
            AtX = 0.0f;
        }
        else
        {
            glyph_info *Info = UiState->GlyphInfos + CodePoint;
            
            Assert(Info->CodePoint == CodePoint);
            
            v2 Size = V2(Scale*Info->Width, Scale*Info->Height);
            
            v2 GlyphOffset = V2(AtX, AtY + Info->YOffset*Scale);
            if(Op == TextOp_Draw)
            {
                PushRenderCommandGlyph(RenderGroup, GlyphOffset, Depth, Size, Color, Info->UV);
            }
            else
            {
                Assert(Op == TextOp_Size);
                Result = Rectangle2Union(Result, Rectangle2(GlyphOffset, V2Add(GlyphOffset, Size)));
            }
            
            AtX += UiState->FontScale*Info->AdvanceWidth*Scale;
            
            if(Index < Text.Size)
            {
                s32 Kerning = stbtt_GetCodepointKernAdvance(&UiState->FontInfo, CodePoint, Text.Data[Index + 1]);
                AtX += UiState->FontScale*Kerning*Scale;
            }
        }
    }
    
    return Result;
}

internal void
DrawTextAt(ui_state *UiState, v2 Offset, f32 Depth, f32 Scale, v4 Color, string Text)
{
    TextOp_(UiState, Offset, Depth, Scale, Color, Text, TextOp_Draw);
}

internal rectangle2
GetTextBounds(ui_state *UiState, v2 Offset, f32 Depth, f32 Scale, v4 Color, string Text)
{
    rectangle2 Result = TextOp_(UiState, Offset, Depth, Scale, Color, Text, TextOp_Size);
    return Result;
}

#include "main.c"

internal void
BeginGrid(ui_state *UiState, rectangle2 Bounds, u32 Columns, u32 Rows)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    
    ui_grid *Grid = PushStruct(Arena, ui_grid);
    Grid->Prev = Frame->CurrentGrid;
    Frame->CurrentGrid = Grid;
    
    Grid->Bounds = Bounds;
    Grid->Columns = Columns;
    Grid->Rows = Rows;
    
    Grid->GridSizeCalculated = false;
    Grid->ColumnWidths = PushArray(Arena, Columns, f32);
    for(u32 ColumnIndex = 0;
        ColumnIndex < Columns;
        ++ColumnIndex)
    {
        Grid->ColumnWidths[ColumnIndex] = 0.0f;
    }
    
    Grid->DefaultRowHeight = UiState->FontScale*UiState->FontAscent*2.0f;
    Grid->RowHeights = PushArray(Arena, Rows, f32);
    for(u32 RowIndex = 0;
        RowIndex < Rows;
        ++RowIndex)
    {
        Grid->RowHeights[RowIndex] = 0.0f;
    }
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
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    Assert(Row <= Grid->Rows);
    
    // NOTE(kstandbridge): Sizes must be set before a control is drawn
    Assert(!Grid->GridSizeCalculated);
    
    Grid->RowHeights[Row] = Height;
}

internal void
GridSetColumnWidth(ui_state *UiState, u32 Column, f32 Width)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    Assert(Column <= Grid->Columns);
    
    // NOTE(kstandbridge): Sizes must be set before a control is drawn
    Assert(!Grid->GridSizeCalculated);
    
    Grid->ColumnWidths[Column] = Width;
}

internal rectangle2
GridGetCellBounds(ui_state *UiState, u32 Column, u32 Row)
{
    ui_frame *Frame = UiState->Frame;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    if(!Grid->GridSizeCalculated)
    {
        
        v2 Dim = Rectangle2GetDim(Grid->Bounds);
#if 0        
        f32 ColumnWidth = Dim.X / Grid->Columns;
        for(u32 ColumnIndex = 0;
            ColumnIndex < Grid->Columns;
            ++ColumnIndex)
        {
            Grid->ColumnWidths[ColumnIndex] = ColumnWidth;
        }
        
        f32 RowHeight = Dim.Y / Grid->Rows;
        for(u32 RowIndex = 0;
            RowIndex < Grid->Rows;
            ++RowIndex)
        {
            Grid->RowHeights[RowIndex] = RowHeight;
        }
#else
        u32 ColumnsFilled = 0;
        f32 WidthUsed = 0.0f;
        for(u32 ColumnIndex = 0;
            ColumnIndex < Grid->Columns;
            ++ColumnIndex)
        {
            if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
            {
                ++ColumnsFilled;
            }
            else
            {
                WidthUsed += Grid->ColumnWidths[ColumnIndex];
            }
        }
        if(ColumnsFilled > 0)
        {
            f32 WidthRemaining = Dim.X - WidthUsed;
            f32 ColumnWidth = WidthRemaining / ColumnsFilled;
            
            for(u32 ColumnIndex = 0;
                ColumnIndex < Grid->Columns;
                ++ColumnIndex)
            {
                if(Grid->ColumnWidths[ColumnIndex] == 0.0f)
                {
                    Grid->ColumnWidths[ColumnIndex] = ColumnWidth;
                }
            }
        }
        
        u32 RowsFilled = 0;
        f32 HeightUsed = 0.0f;
        for(u32 RowIndex = 0;
            RowIndex < Grid->Rows;
            ++RowIndex)
        {
            if(Grid->RowHeights[RowIndex] == 0.0f)
            {
                ++RowsFilled;
            }
            else
            {
                HeightUsed += Grid->RowHeights[RowIndex];
            }
        }
        if(RowsFilled > 0)
        {
            f32 HeightRemaining = Dim.Y - HeightUsed;
            f32 RowHeight = HeightRemaining / RowsFilled;
            
            for(u32 RowIndex = 0;
                RowIndex < Grid->Rows;
                ++RowIndex)
            {
                if(Grid->RowHeights[RowIndex] == 0.0f)
                {
                    Grid->RowHeights[RowIndex] = RowHeight;
                }
            }
        }
#endif
        
        Grid->GridSizeCalculated = true;
    }
    
    v2 Min = Grid->Bounds.Min;
    for(u32 ColumnIndex = 0;
        ColumnIndex < Column;
        ++ColumnIndex)
    {
        Min.X += Grid->ColumnWidths[ColumnIndex];
    }
    
    for(u32 RowIndex = 0;
        RowIndex < Row;
        ++RowIndex)
    {
        Min.Y += Grid->RowHeights[RowIndex];
    }
    
    v2 Max = V2Add(Min, V2(Grid->ColumnWidths[Column], Grid->RowHeights[Row]));
    
    rectangle2 Result = Rectangle2(Min, Max);
    return Result;
}

typedef enum ui_interaction_state
{
    UIInteractionState_None,
    UIInteractionState_HotClicked,
    UIInteractionState_Hot,
    UIInteractionState_Selected
} ui_interaction_state;

internal ui_interaction_state
AddUIInteraction(ui_state *UiState, rectangle2 Bounds, ui_interaction Interaction)
{
    ui_interaction_state Result = UIInteractionState_None;
    
    Assert(Interaction.Type);
    
    if(Rectangle2IsIn(Bounds, UiState->MouseP))
    {
        UiState->NextHotInteraction = Interaction;
    }
    
    if(InteractionsAreEqual(Interaction, UiState->Interaction) &&
       InteractionsAreEqual(Interaction, UiState->NextHotInteraction))
    {
        Result = UIInteractionState_HotClicked;
    }
    else if(InteractionsAreEqual(Interaction, UiState->HotInteraction))
    {
        Result = UIInteractionState_Hot;
    }
    else if(InteractionsAreEqual(Interaction, UiState->SelectedInteration))
    {
        Result = UIInteractionState_Selected;
    }
    else
    {
        Result = UIInteractionState_None;
    }
    
    return Result;
}

#define Button(UiState, Column, Row, Text) Button_(UiState, Column, Row, GenerateUIId(0), Text)
internal b32
Button_(ui_state *UiState, u32 Column, u32 Row, ui_id Id, string Text)
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
    
    rectangle2 Bounds = GridGetCellBounds(UiState, Column, Row);
    Bounds.Min = V2Add(Bounds.Min, V2Set1(GlobalMargin));
    
    v2 Dim = Rectangle2GetDim(Bounds);
    
    ui_interaction_state InteractionState = AddUIInteraction(UiState, Bounds, Interaction);
    switch(InteractionState)
    {
        case UIInteractionState_HotClicked:
        {
            PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonClickedBackColor);
            PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonClickedBorderColor, 1);
        } break;
        
        case UIInteractionState_Hot:
        {
            PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonHotBackColor);
            PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonHotBorderColor, 1);
        } break;
        
        case UIInteractionState_Selected:
        {
            PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonBackColor);
            PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonHotBorderColor, 2);
        } break;
        
        default:
        {
            PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalButtonBackColor);
            PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, GlobalButtonBorderColor, 1);
        } break;
    }
    
    rectangle2 TextBounds = GetTextBounds(UiState, Bounds.Min, 2.0f, 1.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
    
    v2 TextDim = Rectangle2GetDim(TextBounds);
    
    v2 TextOffset = V2Add(Bounds.Min,
                          V2Subtract(V2Multiply(Dim, V2Set1(0.5f)),
                                     V2Multiply(TextDim, V2Set1(0.5f))));
    DrawTextAt(UiState, TextOffset, 2.0f, 1.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
    
    return Result;
}

#define Label(UiState, Column, Row, Text) Label_(UiState, Column, Row, GenerateUIId(0), Text);
internal void
Label_(ui_state *UiState, u32 Column, u32 Row, ui_id Id, string Text)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    rectangle2 Bounds = GridGetCellBounds(UiState, Column, Row);
    
    ui_interaction Interaction =
    {
        .Id = Id,
        .Type = UI_Interaction_NOP,
        .Target = 0
    };
    
    
    if(Rectangle2IsIn(Bounds, UiState->MouseP))
    {
        UiState->NextHotInteraction = Interaction;
        PushRenderCommandRect(RenderGroup, Bounds, 1.0f, V4(0.5f, 0.5f, 0.0f, 1.0f));
    }
    else
    {
        PushRenderCommandRect(RenderGroup, Bounds, 1.0f, GlobalFormColor);
    }
    
    
    PushRenderCommandRectOutline(RenderGroup, Bounds, 1.0f, V4(1.0f, 0.5f, 0.0f, 1.0f), 1.0f);
    
    v2 Dim = Rectangle2GetDim(Bounds);
    
    rectangle2 TextBounds = GetTextBounds(UiState, Bounds.Min, 2.0f, 1.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
    
    v2 TextDim = Rectangle2GetDim(TextBounds);
    
    v2 TextOffset = V2Add(Bounds.Min,
                          V2Subtract(V2Multiply(Dim, V2Set1(0.5f)),
                                     V2Multiply(TextDim, V2Set1(0.5f))));
    
#if 0    
    PushRenderCommandRect(RenderGroup, Rectangle2(TextOffset, V2Add(TextOffset, TextDim)), 
                          1.0f, V4(1, 0, 0, 1));
    PushRenderCommandRectOutline(RenderGroup, Rectangle2(TextOffset, V2Add(TextOffset, TextDim)), 
                                 2.0f, V4(0, 1, 0, 1), 3.0f);
#endif
    
    DrawTextAt(UiState, TextOffset, 2.0f, 1.0f, GlobalFormTextColor, Text);
    
}

internal void
Slider(ui_state *UiState, u32 Column, u32 Row, ui_id Id, f32 *Target, string Text)
{
    ui_frame *Frame = UiState->Frame;
    memory_arena *Arena = Frame->Arena;
    render_group *RenderGroup = Frame->RenderGroup;
    Assert(Frame->CurrentGrid);
    ui_grid *Grid = Frame->CurrentGrid;
    
    rectangle2 Bounds = GridGetCellBounds(UiState, Column, Row);
    
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
    
    DrawTextAt(UiState, Bounds.Min, 2.0f, 1.0f, V4(0.0f, 0.0f, 0.0f, 1.0f), Text);
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
        
        
        // NOTE(kstandbridge): Loading a glyph sprite sheet
        {
            LogDebug("Loading a glyph sprite sheet");
            
            string FontData = Platform.ReadEntireFile(&UiState->Arena, String("C:\\Windows\\Fonts\\segoeui.ttf"));
            stbtt_InitFont(&UiState->FontInfo, FontData.Data, 0);
            
            f32 MaxFontHeightInPixels = 32.0f;
            UiState->FontScale = stbtt_ScaleForPixelHeight(&UiState->FontInfo, MaxFontHeightInPixels);
            stbtt_GetFontVMetrics(&UiState->FontInfo, &UiState->FontAscent, &UiState->FontDescent, &UiState->FontLineGap);
            
#if 0
            s32 Padding = (s32)(MaxFontHeightInPixels / 3.0f);
            u8 OnEdgeValue = (u8)(0.8f*255.0f);
            f32 PixelDistanceScale = (f32)OnEdgeValue/(f32)Padding;
#else
            s32 Padding = 4;
            u8 OnEdgeValue = 128;
            f32 PixelDistanceScale = 100.0f;
#endif
            
#if 1
            u32 FirstChar = 0;
            u32 LastChar = 256;
#else
            u32 FirstChar = 0x0400;
            u32 LastChar = FirstChar + 255;
#endif
            
            s32 MaxWidth = 0;
            s32 MaxHeight = 0;
            s32 TotalWidth = 0;
            s32 TotalHeight = 0;
            u32 ColumnAt = 0;
            u32 RowCount = 1;
            
            glyph_info *GlyphInfo = UiState->GlyphInfos;
            
            for(u32 CodePoint = FirstChar;
                CodePoint < LastChar;
                ++CodePoint)
            {                
                GlyphInfo->Data = stbtt_GetCodepointSDF(&UiState->FontInfo, UiState->FontScale, CodePoint, Padding, OnEdgeValue, PixelDistanceScale, 
                                                        &GlyphInfo->Width, &GlyphInfo->Height, 
                                                        &GlyphInfo->XOffset, &GlyphInfo->YOffset);
                stbtt_GetCodepointHMetrics(&UiState->FontInfo, CodePoint, &GlyphInfo->AdvanceWidth, &GlyphInfo->LeftSideBearing);
                
                GlyphInfo->CodePoint = CodePoint;
                
                if(GlyphInfo->Data)
                {
                    TotalWidth += GlyphInfo->Width;
                    ++ColumnAt;
                    
                    if(GlyphInfo->Height > MaxHeight)
                    {
                        MaxHeight = GlyphInfo->Height;
                    }
                }
                
                if((ColumnAt % 16) == 0)
                {
                    ++RowCount;
                    ColumnAt = 0;
                    if(TotalWidth > MaxWidth)
                    {
                        MaxWidth = TotalWidth;
                    }
                    TotalWidth = 0;
                }
                
                ++GlyphInfo;
            }
            
            TotalWidth = MaxWidth;
            TotalHeight = MaxHeight*RowCount;
            
            umm TextureSize = TotalWidth*TotalHeight*sizeof(u32);
            // TODO(kstandbridge): Temp memory here
            u32 *TextureBytes = PushSize(&UiState->Arena, TextureSize);
            
            u32 AtX = 0;
            u32 AtY = 0;
            
            ColumnAt = 0;
            
            for(u32 Index = 0;
                Index < ArrayCount(UiState->GlyphInfos);
                ++Index)
            {
                GlyphInfo = UiState->GlyphInfos + Index;
                
                GlyphInfo->UV = V4((f32)AtX / (f32)TotalWidth, (f32)AtY / (f32)TotalHeight,
                                   ((f32)AtX + (f32)GlyphInfo->Width) / (f32)TotalWidth, 
                                   ((f32)AtY + (f32)GlyphInfo->Height) / (f32)TotalHeight);
                
                for(s32 Y = 0;
                    Y < GlyphInfo->Height;
                    ++Y)
                {
                    for(s32 X = 0;
                        X < GlyphInfo->Width;
                        ++X)
                    {
                        u32 Alpha = (u32)GlyphInfo->Data[(Y*GlyphInfo->Width) + X];
                        TextureBytes[(Y + AtY)*TotalWidth + (X + AtX)] = 0x00000000 | (u32)((Alpha) << 24);
                    }
                }
                
                AtX += GlyphInfo->Width;
                
                ++ColumnAt;
                
                if((ColumnAt % 16) == 0)
                {
                    AtY += MaxHeight;
                    AtX = 0;
                }
                
                stbtt_FreeSDF(GlyphInfo->Data, 0);
            }
            
            Platform.LoadTexture(TotalWidth, TotalHeight, TextureBytes);
            
        }
        
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
        RenderGroup->ClearColor = GlobalBackColor;
#if 1
        
#if 0        
        PushRenderCommandGlyph(RenderGroup, V2Set1(0.0f), 1.0f, V2(RenderGroup->Width, RenderGroup->Height), V4Set1(1.0f), V4(0.0f, 0.0f, 1.0f, 1.0f));
#else
        rectangle2 ScreenBounds = Rectangle2(V2Set1(0.0f), V2(RenderGroup->Width, RenderGroup->Height));
        ScreenBounds = Rectangle2AddRadiusTo(ScreenBounds, -GlobalMargin);
        
        BeginGrid(UiState, ScreenBounds, 1, 2);
        {
            GridSetRowHeight(UiState, 1, 32.0f);
            
            Label(UiState, 0, 0, String("Tab"));
            
            BeginGrid(UiState, GridGetCellBounds(UiState, 0, 1), 4, 1);
            {
                //Label(UiState, 0, 0, String("Space"));
                
                if(Button(UiState, 1, 0, String("OK")))
                {
                    LogInfo("OK");
                }
                if(Button(UiState, 2, 0, String("Cancel")))
                {
                    LogInfo("Cancel");
                }
                if(Button(UiState, 3, 0, String("Apply")))
                {
                    LogInfo("Apply");
                }
            }
            EndGrid(UiState);
            
        }
        EndGrid(UiState);
#endif
        
        
#else
        f32 LabelWidth = 96.0f;
        BeginGrid(UiState, Rectangle2(V2Set1(0.0f), V2(RenderGroup->Width, RenderGroup->Height)), 1, 11);
        {       
            GridSetRowHeight(UiState, 0, 24.0f);
            GridSetRowHeight(UiState, 1, 48.0f);
            GridSetRowHeight(UiState, 2, 4.0f);
            GridSetRowHeight(UiState, 3, 128.0f);
            GridSetRowHeight(UiState, 4, 4.0f);
            GridSetRowHeight(UiState, 5, 24.0f);
            GridSetRowHeight(UiState, 6, 4.0f);
            GridSetRowHeight(UiState, 7, 24.0f);
            GridSetRowHeight(UiState, 8, 24.0f);
            GridSetRowHeight(UiState, 10, 24.0f);
            
            BeginGrid(UiState, GridGetColumnBounds(UiState, 0, 0), 5, 1);
            {
                Label(UiState, 0, 0, String("General"));
                Label(UiState, 1, 0, String("Sharing"));
                Label(UiState, 2, 0, String("Security"));
                Label(UiState, 3, 0, String("Previous Versions"));
                //Label(UiState, 4, 0, String(""));
            }
            EndGrid(UiState);
            
            BeginGrid(UiState, GridGetColumnBounds(UiState, 0, 1), 2, 1);
            {
                GridSetColumnWidth(UiState, 0, LabelWidth);
                
                Label(UiState, 0, 0, String("<Folder>"));
                Label(UiState, 1, 0, String("Windows"));
            }
            EndGrid(UiState);
            
            Label(UiState, 0, 2, String(""));
            
            BeginGrid(UiState, GridGetColumnBounds(UiState, 0, 3), 2, 5);
            {
                GridSetColumnWidth(UiState, 0, LabelWidth);
                
                Label(UiState, 0, 0, String("Type:"));
                Label(UiState, 1, 0, String("File Folder"));
                Label(UiState, 0, 1, String("Location:"));
                Label(UiState, 1, 1, String("C:\\"));
                Label(UiState, 0, 2, String("Size:"));
                Label(UiState, 1, 2, String("30.4 GB (32,731,587,083 bytes)"));
                Label(UiState, 0, 3, String("Size on disk:"));
                Label(UiState, 1, 3, String("30.3 GB (32,640,794,624 bytes)"));
                Label(UiState, 0, 4, String("Contains:"));
                Label(UiState, 1, 4, String("311,762 Files, 137,072 Folders"));
            }
            EndGrid(UiState);
            
            Label(UiState, 0, 4, String(""));
            
            BeginGrid(UiState, GridGetColumnBounds(UiState, 0, 5), 2, 1);
            {
                GridSetColumnWidth(UiState, 0, LabelWidth);
                
                Label(UiState, 0, 0, String("Created:"));
                Label(UiState, 1, 0, String("07 December 2019, 09:03:44"));
            }
            EndGrid(UiState);
            
            Label(UiState, 0, 6, String(""));
            
            BeginGrid(UiState, GridGetColumnBounds(UiState, 0, 7), 2, 1);
            {
                GridSetColumnWidth(UiState, 0, LabelWidth);
                
                Label(UiState, 0, 0, String("Attributes:"));
                Label(UiState, 1, 0, String("Read-only (Only applies to files in folder)"));
            }
            EndGrid(UiState);
            
            BeginGrid(UiState, GridGetColumnBounds(UiState, 0, 8), 3, 1);
            {
                GridSetColumnWidth(UiState, 0, LabelWidth);
                
                Label(UiState, 0, 0, String(""));
                Label(UiState, 1, 0, String("Hidden"));
                Label(UiState, 2, 0, String("Advanced..."));
            }
            EndGrid(UiState);
            
            Label(UiState, 0, 9, String("Apply Apply AWAV"));
            
            BeginGrid(UiState, GridGetColumnBounds(UiState, 0, 10), 4, 1);
            {
                //Label(UiState, 0, 0, String(""));
                Label(UiState, 1, 0, String("OK"));
                Label(UiState, 2, 0, String("Cancel"));
                Label(UiState, 3, 0, String("Apply"));
            }
            EndGrid(UiState);
        }
        EndGrid(UiState);
#endif
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
            
            if(MouseDown)
            {
                ClearInteraction(&UiState->SelectedInteration);
            }
            
            if(MouseUp)
            {
                UiState->SelectedInteration = UiState->Interaction;
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
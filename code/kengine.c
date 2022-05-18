#include "kengine.h"

global platform_api Platform;

#include "kengine_render_group.c"

internal loaded_bitmap
LoadBMP(memory_arena *Arena, char *FileName)
{
    loaded_bitmap Result;
    ZeroStruct(Result);
    
    string ReadResult = Platform.DEBUGReadEntireFile(Arena, FileName);
    if(ReadResult.Size != 0)
    {
        bitmap_header *Header = (bitmap_header *)ReadResult.Data;
        u32 *Pixels = (u32 *)((u8 *)ReadResult.Data + Header->BitmapOffset);
        
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        Result.WidthOverHeight = SafeRatio1((f32)Result.Width, (f32)Result.Height);
        Result.AlignPercentage = V2(0.5f, 0.5f);
        
        Assert(Result.Height >= 0);
        Assert(Header->Compression == 3);
        
        // NOTE(kstandbridge): If you are using this generically for some reason,
        // please remember that BMP files CAN GO IN EITHER DIRECTION and
        // the height will be negative for top-down.
        // (Also, there can be compression, etc., etc... DON'T think this
        // is complete BMP loading code because it isn't!!)
        
        // NOTE(kstandbridge): Byte order in memory is determined by the Header itself,
        // so we have to read out the masks and convert the pixels ourselves.
        u32 RedMask = Header->RedMask;
        u32 GreenMask = Header->GreenMask;
        u32 BlueMask = Header->BlueMask;
        u32 AlphaMask = ~(RedMask | GreenMask | BlueMask);        
        
        bit_scan_result RedScan = FindLeastSignificantSetBit(RedMask);
        bit_scan_result GreenScan = FindLeastSignificantSetBit(GreenMask);
        bit_scan_result BlueScan = FindLeastSignificantSetBit(BlueMask);
        bit_scan_result AlphaScan = FindLeastSignificantSetBit(AlphaMask);
        
        Assert(RedScan.Found);
        Assert(GreenScan.Found);
        Assert(BlueScan.Found);
        Assert(AlphaScan.Found);
        
        s32 RedShiftDown = (s32)RedScan.Index;
        s32 GreenShiftDown = (s32)GreenScan.Index;
        s32 BlueShiftDown = (s32)BlueScan.Index;
        s32 AlphaShiftDown = (s32)AlphaScan.Index;
        
        u32 *SourceDest = Pixels;
        for(s32 Y = 0;
            Y < Header->Height;
            ++Y)
        {
            for(s32 X = 0;
                X < Header->Width;
                ++X)
            {
                u32 C = *SourceDest;
                
                v4 Texel = {(f32)((C & RedMask) >> RedShiftDown),
                    (f32)((C & GreenMask) >> GreenShiftDown),
                    (f32)((C & BlueMask) >> BlueShiftDown),
                    (f32)((C & AlphaMask) >> AlphaShiftDown)};
                
                Texel = SRGB255ToLinear1(Texel);
                Texel.R *= Texel.A;
                Texel.G *= Texel.A;
                Texel.B *= Texel.A;
                Texel = Linear1ToSRGB255(Texel);
                
                *SourceDest++ = (((u32)(Texel.A + 0.5f) << 24) |
                                 ((u32)(Texel.R + 0.5f) << 16) |
                                 ((u32)(Texel.G + 0.5f) << 8) |
                                 ((u32)(Texel.B + 0.5f) << 0));
            }
        }
    }
    
    Result.Pitch = Result.Width*BITMAP_BYTES_PER_PIXEL;
    
    return(Result);
}

typedef enum text_op_type
{
    TextOp_DrawText,
    TextOp_SizeText,
} text_op_type;
internal rectangle2
TextOpInternal(text_op_type Op, app_state *AppState, render_group *RenderGroup, v2 P, f32 Scale, string Str)
{
    rectangle2 Result = InvertedInfinityRectangle2();
    
    f32 AtX = P.X;
    f32 AtY = P.Y;
    u32 PrevCodePoint = 0;
    for(u32 Index = 0;
        Index < Str.Size;
        ++Index)
    {
        u32 CodePoint = Str.Data[Index];
        
        if(CodePoint != ' ')
        {        
            if(AppState->Glyphs[CodePoint].Memory == 0)
            {
                AppState->Glyphs[CodePoint] = Platform.DEBUGGetGlyphForCodePoint(&AppState->PermanentArena, CodePoint);
            }
            
            loaded_bitmap *Bitmap = AppState->Glyphs + CodePoint;
            f32 Height = Scale*Bitmap->Height;
            v2 Offset = V2(AtX, AtY);
            if(Op == TextOp_DrawText)
            {
                PushBitmap(RenderGroup, Bitmap, Height, Offset, V4(1, 1, 1, 1), 0.0f);
            }
            else
            {
                Assert(Op == TextOp_SizeText);
                v2 Dim = V2(Height*Bitmap->WidthOverHeight, Height);
                v2 Align = Hadamard(Bitmap->AlignPercentage, Dim);
                v2 GlyphP = V2Subtract(Offset, Align);
                rectangle2 GlyphDim = RectMinDim(GlyphP, Dim);
                Result = Union(Result, GlyphDim);
            }
            
            PrevCodePoint = CodePoint;
        }
        
        f32 AdvanceX = Scale*Platform.DEBUGGetHorizontalAdvanceForPair(PrevCodePoint, CodePoint);
        AtX += AdvanceX;
    }
    
    return Result;
}

inline void
WriteLine(app_state *AppState, render_group *RenderGroup, v2 P, f32 Scale, string Str)
{
    TextOpInternal(TextOp_DrawText, AppState, RenderGroup, P, Scale, Str);
}

inline rectangle2
GetTextSize(app_state *AppState, render_group *RenderGroup, v2 P, f32 Scale, string Str)
{
    rectangle2 Result = TextOpInternal(TextOp_SizeText, AppState, RenderGroup, P, Scale, Str);
    
    return Result;
}

internal b32 
PushButton(app_state *AppState, render_group *RenderGroup, memory_arena *Arena, u32 ID, v2 MouseP, v2 BoundsP, string Str)
{
    f32 Scale = 1.0f;
    
    ui_interaction Interaction;
    ZeroStruct(Interaction);
    Interaction.ID = ID;
    Interaction.Type = UiInteraction_ImmediateButton;
    
    b32 Result = InteractionsAreEqual(Interaction, AppState->ToExecute);
    
    rectangle2 TextBounds = GetTextSize(AppState, RenderGroup, BoundsP, Scale, Str);
    
    b32 IsHot = InteractionIsHot(AppState, Interaction);
    
    v4 ButtonColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 0, 0, 1);
    PushRect(RenderGroup, TextBounds.Min, V2Subtract(TextBounds.Max, TextBounds.Min), ButtonColor);
    WriteLine(AppState, RenderGroup, BoundsP, Scale, Str);
    
    if(IsInRectangle(TextBounds, MouseP))
    {
        AppState->NextHotInteraction = Interaction;
    }
    
    return Result;
}

internal void
UIStuffs(app_state *AppState, render_group *RenderGroup, memory_arena *Arena, app_input *Input)
{
    v2 MouseP = V2(Input->MouseX, Input->MouseY);
    
    // NOTE(kstandbridge): Update and render
    {
        
        // NOTE(kstandbridge): Draggable
        {        
            f32 Scale = 1.0f;
            
            ui_interaction Interaction;
            ZeroStruct(Interaction);
            Interaction.ID = 1111;
            Interaction.Type = UiInteraction_Draggable;
            Interaction.P = &AppState->TestP;
            
            string Str = FormatString(Arena, "before %d after", AppState->TestCounter);
            rectangle2 TextBounds = GetTextSize(AppState, RenderGroup, AppState->TestP, Scale, Str);
            
            b32 IsHot = InteractionIsHot(AppState, Interaction);
            
            v4 ButtonColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 0, 0, 1);
            PushRect(RenderGroup, TextBounds.Min, V2Subtract(TextBounds.Max, TextBounds.Min), ButtonColor);
            WriteLine(AppState, RenderGroup, AppState->TestP, Scale, Str);
            
            if(IsInRectangle(TextBounds, MouseP))
            {
                AppState->NextHotInteraction = Interaction;
            }
            
            if(InteractionsAreEqual(Interaction, AppState->Interaction))
            {
                v2 dMouseP = V2Subtract(MouseP, AppState->LastMouseP);
                v2 *P = AppState->Interaction.P;
                *P = V2Add(*P, dMouseP);
            }
            
        }
        
        
        if(PushButton(AppState, RenderGroup, Arena, 2222, MouseP, V2(750, 150), String("Decrement")))
        {
            --AppState->TestCounter;
        }
        if(PushButton(AppState, RenderGroup, Arena, 3333, MouseP, V2(150, 150), String("Increment")))
        {
            ++AppState->TestCounter;
        }
        
        
        
        AppState->ToExecute = AppState->NextToExecute;
        ClearInteraction(&AppState->NextToExecute);
    }
    
    // NOTE(kstandbridge): Interact
    {    
        u32 TransitionCount = Input->MouseButtons[AppInputMouseButton_Left].HalfTransitionCount;
        b32 MouseButton = Input->MouseButtons[AppInputMouseButton_Left].EndedDown;
        if(TransitionCount % 2)
        {
            MouseButton = !MouseButton;
        }
        
        for(u32 TransitionIndex = 0;
            TransitionIndex <= TransitionCount;
            ++TransitionIndex)
        {
            b32 MouseDown = false;
            b32 MouseUp = false;
            if(TransitionIndex != 0)
            {
                MouseDown = MouseButton;
                MouseUp = !MouseButton;
            }
            
            b32 EndInteraction = false;
            
            switch(AppState->Interaction.Type)
            {
                case UiInteraction_ImmediateButton:
                case UiInteraction_Draggable:
                {
                    if(MouseUp)
                    {
                        AppState->NextToExecute = AppState->Interaction;
                        EndInteraction = true;
                    }
                } break;
                
                // TODO(kstandbridge): InteractionSelect that could choose a textbox or row in list
                
                case UiInteraction_None:
                {
                    AppState->HotInteraction = AppState->NextHotInteraction;
                    if(MouseDown)
                    {
                        AppState->Interaction = AppState->NextHotInteraction;
                    }
                } break;
                
                default:
                {
                    if(MouseUp)
                    {
                        EndInteraction = true;
                    }
                }
            }
            
            if(EndInteraction)
            {
                ClearInteraction(&AppState->Interaction);
            }
            
            MouseButton = !MouseButton;
        }
    }
    
    ClearInteraction(&AppState->NextHotInteraction);
    AppState->LastMouseP = MouseP;
    
}

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
        
        SubArena(&AppState->TransientArena, &AppState->PermanentArena, Megabytes(64));
        
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
    
    
    UIStuffs(AppState, RenderGroup, RenderMem.Arena, Input);
    
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
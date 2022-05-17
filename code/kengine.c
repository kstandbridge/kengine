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

internal void
UiBeginInteract(app_state *AppState, app_input *Input, v2 MouseP)
{
    if(AppState->HotInteraction.Type)
    {
        // TODO(kstandbridge): We could potentionally create a new control and begin an interaction with it
        
        AppState->Interaction = AppState->HotInteraction;
    }
    else
    {
        AppState->Interaction.Type = UiInteraction_NOP;
        AppState->Interaction.Generic = 0;
    }
}

internal void
UiEndInteract(app_state *AppState, app_input *Input, v2 MouseP)
{
    switch(AppState->Interaction.Type)
    {
        case UiInteraction_Invoke:
        {
            click_event *Handler = AppState->Interaction.Handler;
            Assert(Handler);
            if(Handler)
            {
                Handler(AppState);
            }
        } break;
    }
    
    AppState->Interaction.Type = UiInteraction_None;
    AppState->Interaction.Generic = 0;
}

internal void
UiInteract(app_state *AppState, app_input *Input, v2 MouseP)
{
    if(AppState->Interaction.Type)
    {
        v2 *P = AppState->Interaction.P;
        v2 dMouseP = V2Subtract(MouseP, AppState->LastMouseP);
        switch(AppState->Interaction.Type)
        {
            case UiInteraction_Move:
            {
                *P = V2Add(*P, dMouseP);
            } break;
        }
        
        for(u32 TransitionIndex = Input->MouseButtons[AppInputMouseButton_Left].HalfTransitionCount;
            TransitionIndex > 1;
            --TransitionIndex)
        {
            UiEndInteract(AppState, Input, MouseP);
            UiBeginInteract(AppState, Input, MouseP);
        }
        
        if(!Input->MouseButtons[AppInputMouseButton_Left].EndedDown)
        {
            UiEndInteract(AppState, Input, MouseP);
        }
    }
    else
    {
        AppState->HotInteraction = AppState->NextHotInteraction;
        
        for(u32 TransitionIndex = Input->MouseButtons[AppInputMouseButton_Left].HalfTransitionCount;
            TransitionIndex > 1;
            --TransitionIndex)
        {
            UiBeginInteract(AppState, Input, MouseP);
            UiEndInteract(AppState, Input, MouseP);
        }
        
        if(Input->MouseButtons[AppInputMouseButton_Left].EndedDown)
        {
            UiBeginInteract(AppState, Input, MouseP);
        }
    }
    
    AppState->LastMouseP = MouseP;
}

internal void
IncrementHandler(app_state *AppState)
{
    ++AppState->TestCounter;
}

internal void
DecrementHandler(app_state *AppState)
{
    --AppState->TestCounter;
}

internal void
DrawUI(app_state *AppState, render_group *RenderGroup, memory_arena *Arena, app_input *Input, v2 MouseP)
{
    f32 Scale = 1.0f;
    
    // NOTE(kstandbridge): Counter Display
    {    
        v2 BoundsP = AppState->TestP;
        string Str = FormatString(Arena, "before %d after", AppState->TestCounter);
        rectangle2 TextBounds = GetTextSize(AppState, RenderGroup, BoundsP, Scale, Str);
        
        ui_interaction ButtonInteraction;
        ButtonInteraction.Type = UiInteraction_Move;
        ButtonInteraction.P = &AppState->TestP;
        //ButtonInteraction.Handler = TestHandler;
        
        b32 IsHot = InteractionIsHot(AppState, ButtonInteraction);
        v4 ButtonColor = IsHot ? V4(1, 0, 0, 1) : V4(1, 0, 1, 1);
        PushRect(RenderGroup, TextBounds.Min, V2Subtract(TextBounds.Max, TextBounds.Min), ButtonColor);
        WriteLine(AppState, RenderGroup, BoundsP, Scale, Str);
        
        if(IsInRectangle(TextBounds, MouseP))
        {
            AppState->NextHotInteraction = ButtonInteraction;
        }
    }
    
    // NOTE(kstandbridge): Increment button
    {
        v2 BoundsP = V2(150, 150);
        
        string Str = String("Decrement");
        rectangle2 TextBounds = GetTextSize(AppState, RenderGroup, BoundsP, Scale, Str);
        
        ui_interaction Interaction;
        Interaction.Type = UiInteraction_Invoke;
        Interaction.Handler = DecrementHandler;
        
        b32 IsHot = InteractionIsHot(AppState, Interaction);
        v4 ButtonColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 0, 0, 1);
        PushRect(RenderGroup, TextBounds.Min, V2Subtract(TextBounds.Max, TextBounds.Min), ButtonColor);
        WriteLine(AppState, RenderGroup, BoundsP, Scale, Str);
        
        if(IsInRectangle(TextBounds, MouseP))
        {
            AppState->NextHotInteraction = Interaction;
        }
    }
    
    // NOTE(kstandbridge): Increment button
    {
        v2 BoundsP = V2(750, 150);
        
        string Str = String("Increment");
        rectangle2 TextBounds = GetTextSize(AppState, RenderGroup, BoundsP, Scale, Str);
        
        ui_interaction Interaction;
        Interaction.Type = UiInteraction_Invoke;
        Interaction.Handler = IncrementHandler;
        
        b32 IsHot = InteractionIsHot(AppState, Interaction);
        v4 ButtonColor = IsHot ? V4(0, 0, 1, 1) : V4(1, 0, 0, 1);
        PushRect(RenderGroup, TextBounds.Min, V2Subtract(TextBounds.Max, TextBounds.Min), ButtonColor);
        WriteLine(AppState, RenderGroup, BoundsP, Scale, Str);
        
        if(IsInRectangle(TextBounds, MouseP))
        {
            AppState->NextHotInteraction = Interaction;
        }
    }
    
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
    
    
    AppState->Time += Input->dtForFrame;;
    
    temporary_memory RenderMem = BeginTemporaryMemory(&AppState->TransientArena);
    
    loaded_bitmap DrawBufferInteral;
    loaded_bitmap *DrawBuffer = &DrawBufferInteral;
    DrawBuffer->Memory = Buffer->Memory;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    
    render_group *RenderGroup = AllocateRenderGroup(RenderMem.Arena, Megabytes(4), DrawBuffer);
    
    PushClear(RenderGroup, V4(0.3f, 0.0f, 0.3f, 1.0f));
    
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
    
#else
    
    AppState->NextHotInteraction.Type = UiInteraction_None;
    AppState->NextHotInteraction.Generic = 0;
    
    v2 MouseP = V2(Input->MouseX, Input->MouseY);
    DrawUI(AppState, RenderGroup, RenderMem.Arena, Input, MouseP);
    UiInteract(AppState, Input, MouseP);
    
    // NOTE(kstandbridge): Cursor
    {
        PushRect(RenderGroup, MouseP, V2(10, 10), V4(1, 1, 0, 1));
    }
#endif
    
    RenderGroupToOutput(RenderGroup);
    EndTemporaryMemory(RenderMem);
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
}
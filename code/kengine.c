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
        }
    }
    
    AppState->Interaction.Type = UiInteraction_None;
    AppState->Interaction.Generic = 0;
}

internal void
TestHandler(app_state *AppState)
{
    ++AppState->TestCounter;
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
    f32 Angle = 0.1f*AppState->Time;
    PushBitmap(RenderGroup, &AppState->TestBMP, (f32)AppState->TestBMP.Height, MouseP, V4(1, 1, 1, 1), Angle);
#endif
    
    f32 Scale = 1.0f;
    v2 MouseP = Unproject(RenderGroup, V2(Input->MouseX, Input->MouseY));
    
    AppState->NextHotInteraction.Type = UiInteraction_None;
    AppState->NextHotInteraction.Generic = 0;
    
    // NOTE(kstandbridge): Draw Button
    {    
        v2 BoundsP = V2(-250.0f, 125.0f);
        string Str = FormatString(RenderMem.Arena, "before %d after", AppState->TestCounter);
        rectangle2 TextBounds = GetTextSize(AppState, RenderGroup, BoundsP, Scale, Str);
        v2 Dim = GetDim(TextBounds);
        v2 TotalDim = V2Add(Dim, V2Set1(10.0f));
        
        v2 TotalMinCorner = V2(BoundsP.X, BoundsP.Y - TotalDim.Y);
        v2 InteriorMinCorner = TotalMinCorner;
        v2 InteriorMaxCorner = V2Add(InteriorMinCorner, Dim);
        rectangle2 TotalBounds = Rectangle2(InteriorMinCorner, InteriorMaxCorner);
        
        ui_interaction ButtonInteraction;
        ButtonInteraction.Type = UiInteraction_Invoke;
        ButtonInteraction.Handler = TestHandler;
        
        b32 IsHot = InteractionIsHot(AppState, ButtonInteraction);
        v4 ButtonColor = IsHot ? V4(1, 0, 0, 1) : V4(1, 0, 1, 1);
        PushRect(RenderGroup, BoundsP, TotalDim, ButtonColor);
        v2 TextP = V2Subtract(BoundsP, V2Multiply(V2Set1(0.5f), Dim));
        WriteLine(AppState, RenderGroup, TextP, Scale, Str);
        
        if(IsInRectangle(TotalBounds, MouseP))
        {
            AppState->NextHotInteraction = ButtonInteraction;
        }
    }
    
    // NOTE(kstandbridge): Interact button
    {
        if(AppState->Interaction.Type)
        {
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
    
    // NOTE(kstandbridge): Cursor
    {
        PushRect(RenderGroup, MouseP, V2(10, 10), V4(1, 1, 0, 1));
    }
    
    RenderGroupToOutput(RenderGroup);
    EndTemporaryMemory(RenderMem);
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
}
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

internal void
DEBUGWriteLine(app_state *AppState, render_group *RenderGroup, v2 P, f32 Scale, string Str)
{
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
            PushBitmap(RenderGroup, Bitmap, Scale*Bitmap->Height, V2(AtX, AtY), V4(1, 1, 1, 1), 0.0f);
            
            PrevCodePoint = CodePoint;
        }
        
        f32 AdvanceX = Scale*Platform.DEBUGGetHorizontalAdvanceForPair(PrevCodePoint, CodePoint);
        AtX += AdvanceX;
    }
    
    AtY += 100.0f;
}

extern void
AppUpdateAndRender(app_memory *Memory, app_offscreen_buffer *Buffer, f32 DeltaTime)
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
    
    
    AppState->Time += DeltaTime;
    
    temporary_memory RenderMem = BeginTemporaryMemory(&AppState->TransientArena);
    
    loaded_bitmap DrawBufferInteral;
    loaded_bitmap *DrawBuffer = &DrawBufferInteral;
    DrawBuffer->Memory = Buffer->Memory;
    DrawBuffer->Width = Buffer->Width;
    DrawBuffer->Height = Buffer->Height;
    DrawBuffer->Pitch = Buffer->Pitch;
    
    render_group *RenderGroup = AllocateRenderGroup(RenderMem.Arena, Megabytes(4), DrawBuffer);
    
    PushClear(RenderGroup, V4(0.3f, 0.0f, 0.3f, 1.0f));
    
    f32 Angle = 0.1f*AppState->Time;
    PushBitmap(RenderGroup, &AppState->TestBMP, (f32)AppState->TestBMP.Height, V2(0.0f, 0.0f), V4(1, 1, 1, 1), Angle);
    
    DEBUGWriteLine(AppState, RenderGroup, V2(-500.0f, 0.0f), 0.5f, String("AWA VA AV"));
    DEBUGWriteLine(AppState, RenderGroup, V2(-500.0f, 120.0f), 0.5f, String("The quick brown fox jumps over the lazy dog"));
    DEBUGWriteLine(AppState, RenderGroup, V2(-500.0f, -120.0f), 0.5f, String("Waltz, bad nymph, for quick jigs vex."));
    
    RenderGroupToOutput(RenderGroup);
    EndTemporaryMemory(RenderMem);
    
    CheckArena(&AppState->PermanentArena);
    CheckArena(&AppState->TransientArena);
}
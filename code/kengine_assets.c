
#if 0
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
                
                v4 Texel = V4((f32)((C & RedMask) >> RedShiftDown),
                              (f32)((C & GreenMask) >> GreenShiftDown),
                              (f32)((C & BlueMask) >> BlueShiftDown),
                              (f32)((C & AlphaMask) >> AlphaShiftDown));
                
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
#endif


#pragma pack(push, 1)
typedef struct bitmap_header
{
    u16 FileType;
    u32 FileSize;
    u16 Reserved1;
    u16 Reserved2;
    u32 BitmapOffset;
    u32 Size;
    s32 Width;
    s32 Height;
    u16 Planes;
    u16 BitsPerPixel;
    u32 Compression;
    u32 SizeOfBitmap;
    s32 HorzResolution;
    s32 VertResolution;
    u32 ColorsUsed;
    u32 ColorsImportant;

    u32 RedMask;
    u32 GreenMask;
    u32 BlueMask;
} bitmap_header;
#pragma pack(pop)

typedef struct loaded_bitmap
{
    s32 Width;
    s32 Height;
    s32 Pitch;
    void *Memory;
    
    u32 Handle;
} loaded_bitmap;

internal loaded_bitmap
LoadBMP(string File)
{
    loaded_bitmap Result = {0};
    
    if(File.Size != 0)
    {
        bitmap_header *Header = (bitmap_header *)File.Data;
        u32 *Pixels = (u32 *)((u8 *)File.Data + Header->BitmapOffset);
        Result.Memory = Pixels;
        Result.Width = Header->Width;
        Result.Height = Header->Height;
        
        Assert(Result.Height >= 0);
        Assert(Header->Compression == 3);

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
#if 1
                Texel.R *= Texel.A;
                Texel.G *= Texel.A;
                Texel.B *= Texel.A;
#endif
                Texel = Linear1ToSRGB255(Texel);
                
                *SourceDest++ = (((u32)(Texel.A + 0.5f) << 24) |
                                 ((u32)(Texel.R + 0.5f) << 16) |
                                 ((u32)(Texel.G + 0.5f) << 8) |
                                 ((u32)(Texel.B + 0.5f) << 0));
            }
        }
    }

    Result.Pitch = Result.Width*4;
    
    return Result;
}
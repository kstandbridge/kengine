#ifndef KENGINE_ASSETS_H

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

typedef struct assets
{
    memory_arena Arena;
    
    loaded_bitmap Glyphs[256];
    
} assets;

#define KENGINE_ASSETS_H
#endif //KENGINE_ASSETS_H

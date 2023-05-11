#ifndef KENGINE_ASSETS_H

typedef enum asset_state_type
{
    AssetState_Unloaded,
    AssetState_Queued,
    AssetState_Processing,
    AssetState_Loaded,
} asset_state_type;

typedef struct sprite_sheet
{
    asset_state_type AssetState;
    
    s32 Width;
    s32 Height;
    s32 Comp;
    void *Handle;
} sprite_sheet;

typedef struct glyph_info
{
    u8 *Data;
    
    u32 CodePoint;
    
    s32 Width;
    s32 Height;
    s32 XOffset;
    s32 YOffset;
    
    s32 AdvanceWidth;
    s32 LeftSideBearing;
    
    v4 UV;
    
} glyph_info;

typedef struct font_asset
{
    asset_state_type AssetState;
    
    string Name;
    
    f32 MaxHeightInPixels;
    f32 Scale;
    s32 Ascent;
    s32 Descent;
    s32 LineGap;
    
    stbtt_fontinfo Info;
} font_asset;

typedef struct glyph_sprite_sheet
{
    asset_state_type AssetState;
    
    font_asset *Font;
    
    u32 CodePointStartAt;
    glyph_info GlyphInfos[256];
    v2 Size;
    void *Handle;
    
    struct glyph_sprite_sheet *Next;
} glyph_sprite_sheet;

typedef struct app_assets
{
    memory_arena Arena;
    platform_work_queue *BackgroundQueue;
    ticket_mutex AssetLock;
    
#if KENGINE_INTERNAL
    b32 IsInitialized;
#endif
    font_asset *Font;
    
    glyph_sprite_sheet *GlyphSpriteSheets;
    
} app_assets;

inline void
InitAssets(app_assets **Assets, platform_work_queue *BackgroundQueue);

#define KENGINE_ASSETS_H
#endif //KENGINE_ASSETS_H

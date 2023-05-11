
internal void
LoadFontInfoThread(memory_arena *TransientArena, app_assets *Assets)
{
    font_asset *Font = Assets->Font;
    LogDebug("Loading font info for %S", Font->Name);
    
    BeginTicketMutex(&Assets->AssetLock);
    string File = PlatformReadEntireFile(&Assets->Arena, Font->Name);
    EndTicketMutex(&Assets->AssetLock);
    
    stbtt_InitFont(&Font->Info, File.Data, 0);
    
    TransientArena;
    
    Font->AssetState = AssetState_Processing;
    
    Font->MaxHeightInPixels = 12.0f;
    Font->Scale = stbtt_ScaleForPixelHeight(&Font->Info, Font->MaxHeightInPixels);
    stbtt_GetFontVMetrics(&Font->Info, &Font->Ascent, &Font->Descent, &Font->LineGap);
    
    Font->AssetState = AssetState_Loaded;
}

internal void
GenerateGlyphSpriteSheetThread(memory_arena *TransientArena, glyph_sprite_sheet *Sheet)
{
    LogDebug("Loading a glyph sprite sheet starting at %u", Sheet->CodePointStartAt);
    
    Sheet->AssetState = AssetState_Processing;
    
    font_asset *Font = Sheet->Font;
    
#if 0
    u32 FirstChar = 0x0400;
#else
    u32 FirstChar = Sheet->CodePointStartAt;
#endif
    u32 LastChar = FirstChar + ArrayCount(Sheet->GlyphInfos);
    
    s32 MaxWidth = 0;
    s32 MaxHeight = 0;
    s32 TotalWidth = 0;
    s32 TotalHeight = 0;
    u32 ColumnAt = 0;
    u32 RowCount = 1;
    
    glyph_info *GlyphInfo = Sheet->GlyphInfos;
    
    for(u32 CodePoint = FirstChar;
        CodePoint < LastChar;
        ++CodePoint)
    {                
        s32 Padding = (s32)(Font->MaxHeightInPixels / 3.0f);
        u8 OnEdgeValue = (u8)(0.8f*255.0f);
        f32 PixelDistanceScale = (f32)OnEdgeValue/(f32)Padding;
        GlyphInfo->Data = stbtt_GetCodepointSDF(&Font->Info, Font->Scale, CodePoint, Padding, OnEdgeValue, PixelDistanceScale, 
                                                &GlyphInfo->Width, &GlyphInfo->Height, 
                                                &GlyphInfo->XOffset, &GlyphInfo->YOffset);
        
        stbtt_GetCodepointHMetrics(&Font->Info, CodePoint, &GlyphInfo->AdvanceWidth, &GlyphInfo->LeftSideBearing);
        
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
    
    u32 *TextureBytes = PushSize(TransientArena, TextureSize);
    
    u32 AtX = 0;
    u32 AtY = 0;
    
    ColumnAt = 0;
    
    for(u32 Index = 0;
        Index < ArrayCount(Sheet->GlyphInfos);
        ++Index)
    {
        GlyphInfo = Sheet->GlyphInfos + Index;
        
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
                TextureBytes[(Y + AtY)*TotalWidth + (X + AtX)] = 0x00FFFFFF | (u32)((Alpha) << 24);
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
    
    Sheet->Size = V2(TotalWidth, TotalHeight);
    Sheet->Handle = DirectXLoadTexture(TotalWidth, TotalHeight, TextureBytes);
    
    Sheet->AssetState = AssetState_Loaded;
}

inline font_asset *
GetFontInfo(app_assets *Assets, string Name)
{
    font_asset *Result = Assets->Font;
    
#if KENGINE_INTERNAL
    Assert(Assets->IsInitialized);
#endif
    
    BeginTicketMutex(&Assets->AssetLock);
    
    if(!Result)
    {
        font_asset *Font = Assets->Font = PushStruct(&Assets->Arena, font_asset);
        Font->AssetState = AssetState_Queued;
        Font->Name = PushString_(&Assets->Arena, Name.Size, Name.Data);
        
        PlatformAddWorkEntry(Assets->BackgroundQueue, LoadFontInfoThread, Assets);
    }
    else
    {
        if(Result->AssetState != AssetState_Loaded)
        {
            Result = 0;
        }
    }
    
    EndTicketMutex(&Assets->AssetLock);
    
    return Result;
}

inline glyph_sprite_sheet *
GetSpriteSheetForCodepoint(app_assets *Assets, u32 CodePoint)
{
    glyph_sprite_sheet *Result = 0;
    
#if KENGINE_INTERNAL
    Assert(Assets->IsInitialized);
#endif
    
    BeginTicketMutex(&Assets->AssetLock);
    
    if((Assets->Font) &&
       (Assets->Font->AssetState == AssetState_Loaded))
    {        
        u32 StartingCodePoint = (CodePoint / 256) * 256;
        
        for(glyph_sprite_sheet *Sheet = Assets->GlyphSpriteSheets;
            Sheet;
            Sheet = Sheet->Next)
        {
            if(Sheet->CodePointStartAt == StartingCodePoint)
            {
                Result = Sheet;
                break;
            }
        }
        
        if(!Result)
        {
            glyph_sprite_sheet *Sheet = PushStruct(&Assets->Arena, glyph_sprite_sheet);
            Sheet->Font = Assets->Font;
            Sheet->AssetState = AssetState_Queued;
            Sheet->CodePointStartAt = StartingCodePoint;
            Sheet->Next = Assets->GlyphSpriteSheets;
            Assets->GlyphSpriteSheets = Sheet;
            
            PlatformAddWorkEntry(Assets->BackgroundQueue, GenerateGlyphSpriteSheetThread, Sheet);
        }
        else
        {
            if(Result->AssetState != AssetState_Loaded)
            {
                Result = 0;
            }
        }
    }
    
    EndTicketMutex(&Assets->AssetLock);
    
    return Result;
}


inline void
InitAssets(app_assets **Assets, platform_work_queue *BackgroundQueue)
{
    *Assets = BootstrapPushStruct(app_assets, Arena);
#if KENGINE_INTERNAL
    (*Assets)->IsInitialized = true;
#endif
    (*Assets)->BackgroundQueue = BackgroundQueue;
}

internal void
DrawRectangle(loaded_bitmap *Buffer, v2 MinP, v2 MaxP, v4 Color, rectangle2i ClipRect)
{
    rectangle2i FillRect = Rectangle2i(RoundF32ToS32(MinP.X), RoundF32ToS32(MaxP.X),
                                       RoundF32ToS32(MinP.Y), RoundF32ToS32(MaxP.Y));
    
    FillRect = Intersect(FillRect, ClipRect);
    
    Color.R *= Color.A;
    Color.G *= Color.A;
    Color.B *= Color.A;
    Color = V4Multiply(Color, V4Set1(255.0f));
    
    if(HasArea(FillRect))
    {
        __m128i StartClipMask = _mm_set1_epi8(-1);
        __m128i EndClipMask = _mm_set1_epi8(-1);
        
        __m128i StartClipMasks[] =
        {
            _mm_slli_si128(StartClipMask, 0*4),
            _mm_slli_si128(StartClipMask, 1*4),
            _mm_slli_si128(StartClipMask, 2*4),
            _mm_slli_si128(StartClipMask, 3*4),            
        };
        
        __m128i EndClipMasks[] =
        {
            _mm_srli_si128(EndClipMask, 0*4),
            _mm_srli_si128(EndClipMask, 3*4),
            _mm_srli_si128(EndClipMask, 2*4),
            _mm_srli_si128(EndClipMask, 1*4),            
        };
        
        if(FillRect.MinX & 3)
        {
            StartClipMask = StartClipMasks[FillRect.MinX & 3];
            FillRect.MinX = FillRect.MinX & ~3;
        }
        
        if(FillRect.MaxX & 3)
        {
            EndClipMask = EndClipMasks[FillRect.MaxX & 3];
            FillRect.MaxX = (FillRect.MaxX & ~3) + 4;
        }
        
        f32 Inv255 = 1.0f / 255.0f;
        __m128 Inv255_4x = _mm_set1_ps(Inv255);
        
        __m128 One = _mm_set1_ps(1.0f);
        __m128 Zero = _mm_set1_ps(0.0f);
        __m128i MaskFF = _mm_set1_epi32(0xFF);
        __m128 Colorr_4x = _mm_set1_ps(Color.R);
        __m128 Colorg_4x = _mm_set1_ps(Color.G);
        __m128 Colorb_4x = _mm_set1_ps(Color.B);
        __m128 Colora_4x = _mm_set1_ps(Color.A);
        __m128 MaxColorValue = _mm_set1_ps(255.0f*255.0f);
        
        u8 *Row = ((u8 *)Buffer->Memory +
                   FillRect.MinX*BITMAP_BYTES_PER_PIXEL +
                   FillRect.MinY*Buffer->Pitch);
        s32 RowAdvance = Buffer->Pitch;
        
        s32 MinY = FillRect.MinY;
        s32 MaxY = FillRect.MaxY;
        s32 MinX = FillRect.MinX;
        s32 MaxX = FillRect.MaxX;
        
        for(int Y = MinY;
            Y < MaxY;
            ++Y)
        {
            __m128i ClipMask = StartClipMask;
            
            u32 *Pixel = (u32 *)Row;
            for(int XI = MinX;
                XI < MaxX;
                XI += 4)
            {            
                
                __m128i WriteMask = ClipMask;
                
                __m128i OriginalDest = _mm_load_si128((__m128i *)Pixel);
                
                // NOTE(kstandbridge): Load destination
                __m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));
                __m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF));
                __m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
                __m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));
                
                // NOTE(kstandbridge): Modulate by incoming color
                __m128 Texelr = _mm_mul_ps(Colorr_4x, Colorr_4x);
                __m128 Texelg = _mm_mul_ps(Colorg_4x, Colorg_4x);
                __m128 Texelb = _mm_mul_ps(Colorb_4x, Colorb_4x);
                __m128 Texela = Colora_4x;
                
                Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), MaxColorValue);
                Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), MaxColorValue);
                Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), MaxColorValue);
                
                // NOTE(kstandbridge): Go from sRGB to "linear" brightness space
                Destr = _mm_mul_ps(Destr, Destr);
                Destg = _mm_mul_ps(Destr, Destr);
                Destb = _mm_mul_ps(Destr, Destr);
                
                // NOTE(kstandbridge): Destination blend
                __m128 InvTexelA = _mm_sub_ps(One, _mm_mul_ps(Inv255_4x, Texela));
                __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
                __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
                __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
                __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);
                
                // NOTE(kstandbridge): Go from "linear" 0-1 brightness space to sRGB 0-255
                Blendedr = _mm_mul_ps(Blendedr, _mm_rsqrt_ps(Blendedr));
                Blendedg = _mm_mul_ps(Blendedg, _mm_rsqrt_ps(Blendedg));
                Blendedb = _mm_mul_ps(Blendedb, _mm_rsqrt_ps(Blendedb));
                __m128i Intr = _mm_cvtps_epi32(Blendedr);
                __m128i Intg = _mm_cvtps_epi32(Blendedg);
                __m128i Intb = _mm_cvtps_epi32(Blendedb);
                __m128i Inta = _mm_cvtps_epi32(Blendeda);
                
                __m128i Sr = _mm_slli_epi32(Intr, 16);
                __m128i Sg = _mm_slli_epi32(Intg, 8);
                __m128i Sb = Intb;
                __m128i Sa = _mm_slli_epi32(Inta, 24);
                
                __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
                
                __m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                                                 _mm_andnot_si128(WriteMask, OriginalDest));
                _mm_store_si128((__m128i *)Pixel, MaskedOut);
                
                
                Pixel += 4;
                
                if((XI + 8) < MaxX)
                {
                    ClipMask = _mm_set1_epi8(-1);
                }
                else
                {
                    ClipMask = EndClipMask;
                }
            }
            
            Row += RowAdvance;
        }
    }
}

internal void
DrawBitmap(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color, loaded_bitmap *Texture, rectangle2i ClipRect)
{
    // NOTE(kstandbridge): Premultiply color up front   
    Color.R *= Color.A;
    Color.G *= Color.A;
    Color.B *= Color.A;
    
    f32 InvXAxisLengthSq = 1.0f / LengthSq(XAxis);
    f32 InvYAxisLengthSq = 1.0f / LengthSq(YAxis);
    
    rectangle2i FillRect = InvertedInfinityRectangle2i();
    
    v2 P[4];
    P[0] = Origin;
    P[1] = V2Add(Origin, XAxis);
    P[2] = V2Add(Origin, V2Add(XAxis, YAxis));
    P[3] = V2Add(Origin, YAxis);
    
    for(s32 PIndex = 0;
        PIndex < ArrayCount(P);
        ++PIndex)
    {
        v2 TestP = P[PIndex];
        s32 FloorX = FloorF32ToS32(TestP.X);
        s32 CeilX = CeilF32ToS32(TestP.X) + 1;
        s32 FloorY = FloorF32ToS32(TestP.Y);
        s32 CeilY = CeilF32ToS32(TestP.Y) + 1;
        
        if(FillRect.MinX > FloorX) {FillRect.MinX = FloorX;}
        if(FillRect.MinY > FloorY) {FillRect.MinY = FloorY;}
        if(FillRect.MaxX < CeilX) {FillRect.MaxX = CeilX;}
        if(FillRect.MaxY < CeilY) {FillRect.MaxY = CeilY;}
    }
    
    FillRect = Intersect(ClipRect, FillRect);
    
    if(HasArea(FillRect))
    {
        __m128i StartClipMask = _mm_set1_epi8(-1);
        __m128i EndClipMask = _mm_set1_epi8(-1);
        
        __m128i StartClipMasks[] =
        {
            _mm_slli_si128(StartClipMask, 0*4),
            _mm_slli_si128(StartClipMask, 1*4),
            _mm_slli_si128(StartClipMask, 2*4),
            _mm_slli_si128(StartClipMask, 3*4),            
        };
        
        __m128i EndClipMasks[] =
        {
            _mm_srli_si128(EndClipMask, 0*4),
            _mm_srli_si128(EndClipMask, 3*4),
            _mm_srli_si128(EndClipMask, 2*4),
            _mm_srli_si128(EndClipMask, 1*4),            
        };
        
        if(FillRect.MinX & 3)
        {
            StartClipMask = StartClipMasks[FillRect.MinX & 3];
            FillRect.MinX = FillRect.MinX & ~3;
        }
        
        if(FillRect.MaxX & 3)
        {
            EndClipMask = EndClipMasks[FillRect.MaxX & 3];
            FillRect.MaxX = (FillRect.MaxX & ~3) + 4;
        }
        
        v2 nXAxis = V2Multiply(V2Set1(InvXAxisLengthSq), XAxis);
        v2 nYAxis = V2Multiply(V2Set1(InvYAxisLengthSq), YAxis);
        
        f32 Inv255 = 1.0f / 255.0f;
        __m128 Inv255_4x = _mm_set1_ps(Inv255);
        
        __m128 One = _mm_set1_ps(1.0f);
        __m128 Half = _mm_set1_ps(0.5f);
        __m128 Four_4x = _mm_set1_ps(4.0f);
        __m128 Zero = _mm_set1_ps(0.0f);
        __m128i MaskFF = _mm_set1_epi32(0xFF);
        __m128i MaskFFFF = _mm_set1_epi32(0xFFFF);
        __m128i MaskFF00FF = _mm_set1_epi32(0x00FF00FF);
        __m128 Colorr_4x = _mm_set1_ps(Color.R);
        __m128 Colorg_4x = _mm_set1_ps(Color.G);
        __m128 Colorb_4x = _mm_set1_ps(Color.B);
        __m128 Colora_4x = _mm_set1_ps(Color.A);
        __m128 nXAxisx_4x = _mm_set1_ps(nXAxis.X);
        __m128 nXAxisy_4x = _mm_set1_ps(nXAxis.Y);
        __m128 nYAxisx_4x = _mm_set1_ps(nYAxis.X);
        __m128 nYAxisy_4x = _mm_set1_ps(nYAxis.Y);
        __m128 Originx_4x = _mm_set1_ps(Origin.X);
        __m128 Originy_4x = _mm_set1_ps(Origin.Y);
        __m128 MaxColorValue = _mm_set1_ps(255.0f*255.0f);
        __m128i TexturePitch_4x = _mm_set1_epi32(Texture->Pitch);
        
        __m128 WidthM2 = _mm_set1_ps((f32)(Texture->Width - 2));
        __m128 HeightM2 = _mm_set1_ps((f32)(Texture->Height - 2));
        
        u8 *Row = ((u8 *)Buffer->Memory +
                   FillRect.MinX*BITMAP_BYTES_PER_PIXEL +
                   FillRect.MinY*Buffer->Pitch);
        s32 RowAdvance = Buffer->Pitch;
        
        void *TextureMemory = Texture->Memory;
        s32 TexturePitch = Texture->Pitch;
        
        s32 MinY = FillRect.MinY;
        s32 MaxY = FillRect.MaxY;
        s32 MinX = FillRect.MinX;
        s32 MaxX = FillRect.MaxX;
        for(s32 Y = MinY;
            Y < MaxY;
            ++Y)
        {
            __m128 PixelPy = _mm_set1_ps((f32)Y);
            PixelPy = _mm_sub_ps(PixelPy, Originy_4x);
            __m128 PynX = _mm_mul_ps(PixelPy, nXAxisy_4x);
            __m128 PynY = _mm_mul_ps(PixelPy, nYAxisy_4x);
            
            __m128 PixelPx = _mm_set_ps((f32)(MinX + 3),
                                        (f32)(MinX + 2),
                                        (f32)(MinX + 1),
                                        (f32)(MinX + 0));
            PixelPx = _mm_sub_ps(PixelPx, Originx_4x);
            
            __m128i ClipMask = StartClipMask;
            
            u32 *Pixel = (u32 *)Row;
            for(s32 XI = MinX;
                XI < MaxX;
                XI += 4)
            {            
#define mmSquare(a) _mm_mul_ps(a, a)    
#define M(a, i) ((float *)&(a))[i]
#define Mi(a, i) ((u32 *)&(a))[i]
                
                
                __m128 U = _mm_add_ps(_mm_mul_ps(PixelPx, nXAxisx_4x), PynX);
                __m128 V = _mm_add_ps(_mm_mul_ps(PixelPx, nYAxisx_4x), PynY);
                
                __m128i WriteMask = _mm_castps_si128(_mm_and_ps(_mm_and_ps(_mm_cmpge_ps(U, Zero),
                                                                           _mm_cmple_ps(U, One)),
                                                                _mm_and_ps(_mm_cmpge_ps(V, Zero),
                                                                           _mm_cmple_ps(V, One))));
                WriteMask = _mm_and_si128(WriteMask, ClipMask);
                
                __m128i OriginalDest = _mm_load_si128((__m128i *)Pixel);
                
                U = _mm_min_ps(_mm_max_ps(U, Zero), One);
                V = _mm_min_ps(_mm_max_ps(V, Zero), One);
                
                // NOTE(kstandbridge): Bias texture coordinates to start
                // on the boundary between the 0,0 and 1,1 pixels.
                __m128 tX = _mm_add_ps(_mm_mul_ps(U, WidthM2), Half);
                __m128 tY = _mm_add_ps(_mm_mul_ps(V, HeightM2), Half);
                
                __m128i FetchX_4x = _mm_cvttps_epi32(tX);
                __m128i FetchY_4x = _mm_cvttps_epi32(tY);
                
                __m128 fX = _mm_sub_ps(tX, _mm_cvtepi32_ps(FetchX_4x));
                __m128 fY = _mm_sub_ps(tY, _mm_cvtepi32_ps(FetchY_4x));
                
                FetchX_4x = _mm_slli_epi32(FetchX_4x, 2);
                FetchY_4x = _mm_or_si128(_mm_mullo_epi16(FetchY_4x, TexturePitch_4x),
                                         _mm_slli_epi32(_mm_mulhi_epi16(FetchY_4x, TexturePitch_4x), 16));
                __m128i Fetch_4x = _mm_add_epi32(FetchX_4x, FetchY_4x);
                
                s32 Fetch0 = Mi(Fetch_4x, 0);
                s32 Fetch1 = Mi(Fetch_4x, 1);
                s32 Fetch2 = Mi(Fetch_4x, 2);
                s32 Fetch3 = Mi(Fetch_4x, 3);
                
                u8 *TexelPtr0 = ((u8 *)TextureMemory) + Fetch0;
                u8 *TexelPtr1 = ((u8 *)TextureMemory) + Fetch1;
                u8 *TexelPtr2 = ((u8 *)TextureMemory) + Fetch2;
                u8 *TexelPtr3 = ((u8 *)TextureMemory) + Fetch3;
                
                __m128i SampleA = _mm_setr_epi32(*(u32 *)(TexelPtr0),
                                                 *(u32 *)(TexelPtr1),
                                                 *(u32 *)(TexelPtr2),
                                                 *(u32 *)(TexelPtr3));
                
                __m128i SampleB = _mm_setr_epi32(*(u32 *)(TexelPtr0 + sizeof(u32)),
                                                 *(u32 *)(TexelPtr1 + sizeof(u32)),
                                                 *(u32 *)(TexelPtr2 + sizeof(u32)),
                                                 *(u32 *)(TexelPtr3 + sizeof(u32)));
                
                __m128i SampleC = _mm_setr_epi32(*(u32 *)(TexelPtr0 + TexturePitch),
                                                 *(u32 *)(TexelPtr1 + TexturePitch),
                                                 *(u32 *)(TexelPtr2 + TexturePitch),
                                                 *(u32 *)(TexelPtr3 + TexturePitch));
                
                __m128i SampleD = _mm_setr_epi32(*(u32 *)(TexelPtr0 + TexturePitch + sizeof(u32)),
                                                 *(u32 *)(TexelPtr1 + TexturePitch + sizeof(u32)),
                                                 *(u32 *)(TexelPtr2 + TexturePitch + sizeof(u32)),
                                                 *(u32 *)(TexelPtr3 + TexturePitch + sizeof(u32)));
                
                // NOTE(kstandbridge): Unpack bilinear samples
                __m128i TexelArb = _mm_and_si128(SampleA, MaskFF00FF);
                __m128i TexelAag = _mm_and_si128(_mm_srli_epi32(SampleA, 8), MaskFF00FF);
                TexelArb = _mm_mullo_epi16(TexelArb, TexelArb);
                __m128 TexelAa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelAag, 16));
                TexelAag = _mm_mullo_epi16(TexelAag, TexelAag);
                
                __m128i TexelBrb = _mm_and_si128(SampleB, MaskFF00FF);
                __m128i TexelBag = _mm_and_si128(_mm_srli_epi32(SampleB, 8), MaskFF00FF);
                TexelBrb = _mm_mullo_epi16(TexelBrb, TexelBrb);
                __m128 TexelBa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBag, 16));
                TexelBag = _mm_mullo_epi16(TexelBag, TexelBag);
                
                __m128i TexelCrb = _mm_and_si128(SampleC, MaskFF00FF);
                __m128i TexelCag = _mm_and_si128(_mm_srli_epi32(SampleC, 8), MaskFF00FF);
                TexelCrb = _mm_mullo_epi16(TexelCrb, TexelCrb);
                __m128 TexelCa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCag, 16));
                TexelCag = _mm_mullo_epi16(TexelCag, TexelCag);
                
                __m128i TexelDrb = _mm_and_si128(SampleD, MaskFF00FF);
                __m128i TexelDag = _mm_and_si128(_mm_srli_epi32(SampleD, 8), MaskFF00FF);
                TexelDrb = _mm_mullo_epi16(TexelDrb, TexelDrb);
                __m128 TexelDa = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDag, 16));
                TexelDag = _mm_mullo_epi16(TexelDag, TexelDag);
                
                // NOTE(kstandbridge): Load destination
                __m128 Destb = _mm_cvtepi32_ps(_mm_and_si128(OriginalDest, MaskFF));
                __m128 Destg = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 8), MaskFF));
                __m128 Destr = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 16), MaskFF));
                __m128 Desta = _mm_cvtepi32_ps(_mm_and_si128(_mm_srli_epi32(OriginalDest, 24), MaskFF));
                
                // NOTE(kstandbridge): Convert texture from 0-255 sRGB to "linear" 0-1 brightness space
                __m128 TexelAr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelArb, 16));
                __m128 TexelAg = _mm_cvtepi32_ps(_mm_and_si128(TexelAag, MaskFFFF));
                __m128 TexelAb = _mm_cvtepi32_ps(_mm_and_si128(TexelArb, MaskFFFF));
                
                __m128 TexelBr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelBrb, 16));
                __m128 TexelBg = _mm_cvtepi32_ps(_mm_and_si128(TexelBag, MaskFFFF));
                __m128 TexelBb = _mm_cvtepi32_ps(_mm_and_si128(TexelBrb, MaskFFFF));
                
                __m128 TexelCr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelCrb, 16));
                __m128 TexelCg = _mm_cvtepi32_ps(_mm_and_si128(TexelCag, MaskFFFF));
                __m128 TexelCb = _mm_cvtepi32_ps(_mm_and_si128(TexelCrb, MaskFFFF));
                
                __m128 TexelDr = _mm_cvtepi32_ps(_mm_srli_epi32(TexelDrb, 16));
                __m128 TexelDg = _mm_cvtepi32_ps(_mm_and_si128(TexelDag, MaskFFFF));
                __m128 TexelDb = _mm_cvtepi32_ps(_mm_and_si128(TexelDrb, MaskFFFF));
                
                // NOTE(kstandbridge): Bilinear texture blend
                __m128 ifX = _mm_sub_ps(One, fX);
                __m128 ifY = _mm_sub_ps(One, fY);
                
                __m128 l0 = _mm_mul_ps(ifY, ifX);
                __m128 l1 = _mm_mul_ps(ifY, fX);
                __m128 l2 = _mm_mul_ps(fY, ifX);
                __m128 l3 = _mm_mul_ps(fY, fX);
                
                __m128 Texelr = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAr), _mm_mul_ps(l1, TexelBr)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCr), _mm_mul_ps(l3, TexelDr)));
                __m128 Texelg = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAg), _mm_mul_ps(l1, TexelBg)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCg), _mm_mul_ps(l3, TexelDg)));
                __m128 Texelb = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAb), _mm_mul_ps(l1, TexelBb)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCb), _mm_mul_ps(l3, TexelDb)));
                __m128 Texela = _mm_add_ps(_mm_add_ps(_mm_mul_ps(l0, TexelAa), _mm_mul_ps(l1, TexelBa)),
                                           _mm_add_ps(_mm_mul_ps(l2, TexelCa), _mm_mul_ps(l3, TexelDa)));
                
                // NOTE(kstandbridge): Modulate by incoming color
                Texelr = _mm_mul_ps(Texelr, Colorr_4x);
                Texelg = _mm_mul_ps(Texelg, Colorg_4x);
                Texelb = _mm_mul_ps(Texelb, Colorb_4x);
                Texela = _mm_mul_ps(Texela, Colora_4x);
                
                Texelr = _mm_min_ps(_mm_max_ps(Texelr, Zero), MaxColorValue);
                Texelg = _mm_min_ps(_mm_max_ps(Texelg, Zero), MaxColorValue);
                Texelb = _mm_min_ps(_mm_max_ps(Texelb, Zero), MaxColorValue);
                
                // NOTE(kstandbridge): Go from sRGB to "linear" brightness space
                Destr = mmSquare(Destr);
                Destg = mmSquare(Destg);
                Destb = mmSquare(Destb);
                
                // NOTE(kstandbridge): Destination blend
                __m128 InvTexelA = _mm_sub_ps(One, _mm_mul_ps(Inv255_4x, Texela));
                __m128 Blendedr = _mm_add_ps(_mm_mul_ps(InvTexelA, Destr), Texelr);
                __m128 Blendedg = _mm_add_ps(_mm_mul_ps(InvTexelA, Destg), Texelg);
                __m128 Blendedb = _mm_add_ps(_mm_mul_ps(InvTexelA, Destb), Texelb);
                __m128 Blendeda = _mm_add_ps(_mm_mul_ps(InvTexelA, Desta), Texela);
                
                // NOTE(kstandbridge): Go from "linear" 0-1 brightness space to sRGB 0-255
                Blendedr = _mm_mul_ps(Blendedr, _mm_rsqrt_ps(Blendedr));
                Blendedg = _mm_mul_ps(Blendedg, _mm_rsqrt_ps(Blendedg));
                Blendedb = _mm_mul_ps(Blendedb, _mm_rsqrt_ps(Blendedb));
                
                Blendeda = Blendeda;
                
                __m128i Intr = _mm_cvtps_epi32(Blendedr);
                __m128i Intg = _mm_cvtps_epi32(Blendedg);
                __m128i Intb = _mm_cvtps_epi32(Blendedb);
                __m128i Inta = _mm_cvtps_epi32(Blendeda);
                
                __m128i Sr = _mm_slli_epi32(Intr, 16);
                __m128i Sg = _mm_slli_epi32(Intg, 8);
                __m128i Sb = Intb;
                __m128i Sa = _mm_slli_epi32(Inta, 24);
                
                __m128i Out = _mm_or_si128(_mm_or_si128(Sr, Sg), _mm_or_si128(Sb, Sa));
                
                __m128i MaskedOut = _mm_or_si128(_mm_and_si128(WriteMask, Out),
                                                 _mm_andnot_si128(WriteMask, OriginalDest));
                _mm_store_si128((__m128i *)Pixel, MaskedOut);
                
                
                PixelPx = _mm_add_ps(PixelPx, Four_4x);            
                Pixel += 4;
                
                if((XI + 8) < MaxX)
                {
                    ClipMask = _mm_set1_epi8(-1);
                }
                else
                {
                    ClipMask = EndClipMask;
                }
                
            }
            Row += RowAdvance;
        }
    }
}

typedef struct
{
    render_commands *Commands;
    loaded_bitmap *OutputTarget;
    rectangle2i ClipRect;
} software_render_commands_work;
internal void
SoftwareRenderCommandsThread(software_render_commands_work *Work)
{
    render_commands *Commands = Work->Commands;
    loaded_bitmap *OutputTarget = Work->OutputTarget;
    
    u32 ClipRectIndex = 0XFFFFFFFF;
    rectangle2i ClipRect = Work->ClipRect;
    
    sort_entry *SortEntries = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
    sort_entry *SortEntry = SortEntries;
    
    for(u32 SortEntryIndex = 0;
        SortEntryIndex < Commands->PushBufferElementCount;
        ++SortEntryIndex, ++SortEntry)
    {
        render_group_command_header *Header = (render_group_command_header *)(Commands->PushBufferBase + SortEntry->Index);
        if(ClipRectIndex != Header->ClipRectIndex)
        {
            ClipRectIndex = Header->ClipRectIndex;
            Assert(ClipRectIndex < Commands->ClipRectCount);
            
            render_command_cliprect *Clip = Commands->ClipRects + ClipRectIndex;
            ClipRect = Intersect(Work->ClipRect, Clip->Bounds);
        }
        
        void *Data = (u8 *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderGroupCommand_Clear:
            {
                render_group_command_clear *Command = (render_group_command_clear *)Data;
                DrawRectangle(OutputTarget, V2(0.0f, 0.0f), V2((f32)OutputTarget->Width, (f32)OutputTarget->Height), Command->Color, ClipRect);
            } break;
            
            case RenderGroupCommand_Rectangle:
            {
                render_group_command_rectangle *Command = (render_group_command_rectangle *)Data;
                DrawRectangle(OutputTarget, Command->Bounds.Min, Command->Bounds.Max, Command->Color, ClipRect);
            } break;
            
            case RenderGroupCommand_Bitmap:
            {
                render_group_command_bitmap *Command = (render_group_command_bitmap *)Data;
                
                // TODO(kstandbridge): When bitmap loading is on background thread only this can be an if
                Assert(Command->Bitmap);
                {
                    // TODO(kstandbridge): Rotation?
                    v2 XAxis = V2(1, 0);
                    v2 YAxis = V2(0, 1);
                    v2 ScaledXAxis = V2Multiply(V2Set1(Command->Dim.X), XAxis);
                    v2 ScaledYAxis = V2Multiply(V2Set1(Command->Dim.Y), YAxis);
                    DrawBitmap(OutputTarget, Command->P, ScaledXAxis, ScaledYAxis, Command->Color, Command->Bitmap, ClipRect);
                }
                
                
            } break;
            
            InvalidDefaultCase;
        }
    }
}

internal void
SoftwareRenderCommands(platform_api *Platform, platform_work_queue *Queue, render_commands *Commands, loaded_bitmap *OutputTarget)
{
#if 0
    Queue;
    software_render_commands_work Work;
    Work.Commands = Commands;
    Work.OutputTarget = OutputTarget;
    Work.ClipRect = Rectangle2i(0, OutputTarget->Width, 0, OutputTarget->Height);;
    SoftwareRenderCommandsThread(&Work);
#else
    
#define TileCountX 4
#define TileCountY 4
    
    software_render_commands_work WorkArray[TileCountX*TileCountY];
    
    Assert(((umm)OutputTarget->Memory & 15) == 0);    
    s32 TileWidth = OutputTarget->Width / TileCountX;
    s32 TileHeight = OutputTarget->Height / TileCountY;
    TileWidth = ((TileWidth + 3) / 4) * 4;
    
    s32 WorkCount = 0;
    for(s32 TileY = 0;
        TileY < TileCountY;
        ++TileY)
    {
        for(s32 TileX = 0;
            TileX < TileCountX;
            ++TileX)
        {
            software_render_commands_work *Work = WorkArray + WorkCount++;
            
            rectangle2i ClipRect;
            ClipRect.MinX = TileX*TileWidth;
            ClipRect.MaxX = ClipRect.MinX + TileWidth;
            ClipRect.MinY = TileY*TileHeight;
            ClipRect.MaxY = ClipRect.MinY + TileHeight;
            
            if(TileX == (TileCountX - 1))
            {
                ClipRect.MaxX = OutputTarget->Width;
            }
            if(TileY == (TileCountY - 1))
            {
                ClipRect.MaxY = OutputTarget->Height;
            }
            
            Work->Commands = Commands;
            Work->OutputTarget = OutputTarget;
            Work->ClipRect = ClipRect;
            
            Platform->AddWorkEntry(Queue, SoftwareRenderCommandsThread, Work);
        }
    }
    
    Platform->CompleteAllWork(Queue);
#endif
    
}
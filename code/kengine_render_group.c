
internal void
DrawRectangle(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v4 Color, v4 AltColor, rectangle2i ClipRect)
{
    rectangle2i FillRect;
    FillRect.MinX = RoundReal32ToInt32(vMin.X);
    FillRect.MinY = RoundReal32ToInt32(vMin.Y);
    FillRect.MaxX = RoundReal32ToInt32(vMax.X);
    FillRect.MaxY = RoundReal32ToInt32(vMax.Y);
    
    FillRect = Intersect(FillRect, ClipRect);
    
    u32 Color32 = ((RoundReal32ToUInt32(Color.A * 255.0f) << 24) |
                   (RoundReal32ToUInt32(Color.R * 255.0f) << 16) |
                   (RoundReal32ToUInt32(Color.G * 255.0f) << 8) |
                   (RoundReal32ToUInt32(Color.B * 255.0f) << 0));
    
    u32 AltColor32 = ((RoundReal32ToUInt32(AltColor.A * 255.0f) << 24) |
                      (RoundReal32ToUInt32(AltColor.R * 255.0f) << 16) |
                      (RoundReal32ToUInt32(AltColor.G * 255.0f) << 8) |
                      (RoundReal32ToUInt32(AltColor.B * 255.0f) << 0));
    
    u8 *Row = ((u8 *)Buffer->Memory +
               FillRect.MinX*BITMAP_BYTES_PER_PIXEL +
               FillRect.MinY*Buffer->Pitch);
    b32 InvertColor = false;
    for(s32 Y = FillRect.MinY;
        Y < FillRect.MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(s32 X = FillRect.MinX;
            X < FillRect.MaxX;
            ++X)
        {            
            u32 ColorToUse;
            
            if(InvertColor)
            {
                ColorToUse = (X % 2 == 1) ? Color32 : AltColor32;
            }
            else
            {
                ColorToUse = (X % 2 == 1) ? AltColor32 : Color32;
            }
            
            *Pixel++ = ColorToUse;
        }
        InvertColor = !InvertColor;
        Row += Buffer->Pitch;
    }
}

internal void
DrawCircle(loaded_bitmap *Buffer, v2 vMin, v2 vMax, v4 Color, rectangle2i ClipRect)
{
    f32 R = Color.R;
    f32 G = Color.G;
    f32 B = Color.B;
    f32 A = Color.A;
    
    rectangle2i FillRect;
    FillRect.MinX = RoundReal32ToInt32(vMin.X);
    FillRect.MinY = RoundReal32ToInt32(vMin.Y);
    FillRect.MaxX = RoundReal32ToInt32(vMax.X);
    FillRect.MaxY = RoundReal32ToInt32(vMax.Y);
    
    FillRect = Intersect(FillRect, ClipRect);
    
    s32 RadiusX = (FillRect.MaxX - FillRect.MinX) / 2;
    s32 RadiusY = (FillRect.MaxY - FillRect.MinY) / 2;
    s32 CenterX = FillRect.MinX + (RadiusX);
    s32 CenterY = FillRect.MinY + (RadiusY);
    
    u32 Color32 = ((RoundReal32ToUInt32(A * 255.0f) << 24) |
                   (RoundReal32ToUInt32(R * 255.0f) << 16) |
                   (RoundReal32ToUInt32(G * 255.0f) << 8) |
                   (RoundReal32ToUInt32(B * 255.0f) << 0));
    
    u8 *Row = ((u8 *)Buffer->Memory +
               FillRect.MinX*BITMAP_BYTES_PER_PIXEL +
               FillRect.MinY*Buffer->Pitch);
    for(s32 Y = FillRect.MinY;
        Y < FillRect.MaxY;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(s32 X = FillRect.MinX;
            X < FillRect.MaxX;
            ++X)
        {   
            s32 dX = X - CenterX;
            s32 dY = Y - CenterY;
            
            dX *= dX;
            dY *= dY;
            
            f32 Val = SquareRoot((f32)dX + (f32)dY);
            if(Val < RadiusX)
            {
                *Pixel = Color32;
            }
            ++Pixel;
        }
        
        Row += Buffer->Pitch;
    }
}

internal void
DrawBitmap(loaded_bitmap *Buffer, v2 Origin, v2 XAxis, v2 YAxis, v4 Color, loaded_bitmap *Texture, rectangle2i ClipRect)
{
    // NOTE(kstandbridge): Premultiply color up front   
    Color.R *= Color.A;
    Color.G *= Color.A;
    Color.B *= Color.A;
    
    f32 XAxisLength = LengthV2(XAxis);
    f32 YAxisLength = LengthV2(YAxis);
    
    //v2 NyAxix = V2Multiply(V2Set1(YAxisLength / XAxisLength), XAxis);
    //v2 NyAxis = V2Multiply(V2Set1(XAxisLength / YAxisLength), YAxis);
    
    // NOTE(kstandbridge): NzScale could be a parameter if we want people to
    // have control over the amount of scaling in the Z direction
    // that the normals appear to have.
    f32 NzScale = 0.5f*(XAxisLength + YAxisLength);
    
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
        s32 FloorX = FloorReal32ToInt32(TestP.X);
        s32 CeilX = CeilReal32ToInt32(TestP.X) + 1;
        s32 FloorY = FloorReal32ToInt32(TestP.Y);
        s32 CeilY = CeilReal32ToInt32(TestP.Y) + 1;
        
        if(FillRect.MinX > FloorX) {FillRect.MinX = FloorX;}
        if(FillRect.MinY > FloorY) {FillRect.MinY = FloorY;}
        if(FillRect.MaxX < CeilX) {FillRect.MaxX = CeilX;}
        if(FillRect.MaxY < CeilY) {FillRect.MaxY = CeilY;}
    }
    
    //    rectangle2i ClipRect = {0, 0, WidthMax, HeightMax};
    //    rectangle2i ClipRect = {128, 128, 256, 256};
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
        f32 One255 = 255.0f;
        
        __m128 One = _mm_set1_ps(1.0f);
        __m128 Half = _mm_set1_ps(0.5f);
        __m128 Four_4x = _mm_set1_ps(4.0f);
        //__m128 One255_4x = _mm_set1_ps(255.0f);
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

#define PushRenderElement(Group, Type) (Type *)PushRenderElementInternal(Group, sizeof(Type), RenderGroupEntry_##Type)
inline void *
PushRenderElementInternal(render_group *Group, umm Size, render_group_entry_type Type)
{
    void *Result = 0;
    
    Size += sizeof(render_group_entry_header);
    
    if((Group->PushBufferSize + Size) < Group->MaxPushBufferSize)
    {
        render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + Group->PushBufferSize);
        Header->Type = Type;
        Result = (u8 *)Header + sizeof(*Header);
        Group->PushBufferSize += Size;
    }
    else
    {
        InvalidCodePath;
    }
    
    return Result;
}

inline void
PushClear(render_group *Group, v4 Color)
{
    render_entry_clear *Clear = PushRenderElement(Group, render_entry_clear);
    if(Clear)
    {
        Clear->Color = Color;
    }
}

inline void
PushBitmap(render_group *Group, loaded_bitmap *Bitmap, f32 Height, v2 Offset, v4 Color, f32 Angle)
{
    v2 Dim = V2(Height*Bitmap->WidthOverHeight, Height);
    v2 Align = Hadamard(Bitmap->AlignPercentage, Dim);
    v2 P = V2Subtract(Offset, Align);
    render_entry_bitmap *Entry = PushRenderElement(Group, render_entry_bitmap);
    if(Entry)
    {
        Entry->Bitmap = Bitmap;
        Entry->P = P;
        Entry->Color = Color;
        Entry->Dim = Dim;
        Entry->Angle = Angle;
    }
}

inline void
PushRect(render_group *Group, v2 Offset, v2 Dim, v4 Color, v4 AltColor)
{
    render_entry_rectangle *Entry = PushRenderElement(Group, render_entry_rectangle);
    if(Entry)
    {
        Entry->P = Offset;
        Entry->Color = Color;
        Entry->AltColor = AltColor;
        Entry->Dim = Dim;
    }
}

inline void
PushRectOutline(render_group *Group, v2 Offset, v2 Dim, v4 Color, v4 AltColor, f32 Thickness)
{
    PushRect(Group, Offset, V2(Dim.X, Thickness), Color, AltColor);
    PushRect(Group, V2Add(Offset, V2(0.0f, Dim.Y - Thickness)), V2(Dim.X, Thickness), Color, AltColor);
    
    PushRect(Group, Offset, V2(Thickness, Dim.Y), Color, AltColor);
    PushRect(Group, V2Add(Offset, V2(Dim.X - Thickness, 0.0f)), V2(Thickness, Dim.Y), Color, AltColor);
}

internal render_group *
AllocateRenderGroup(memory_arena *Arena, memory_index MaxPushBufferSize, loaded_bitmap *OutputTarget)
{
    render_group *Result = PushStruct(Arena, render_group);
    
    Result->OutputTarget = OutputTarget;
    
    Result->PushBufferBase = (u8 *)PushSize(Arena, MaxPushBufferSize);
    Result->MaxPushBufferSize = MaxPushBufferSize;
    Result->PushBufferSize = 0;
    
    Result->ScreenCenter = V2(0.5f*(f32)OutputTarget->Width, 0.5f*(f32)OutputTarget->Height);
    
    return Result;
}

typedef struct tile_render_work
{
    render_group *Group;
    rectangle2i ClipRect;
} tile_render_work;
internal void
TileRenderWorkThread(void *Data)
{
    tile_render_work *Work = (tile_render_work *)Data;
    render_group *Group = Work->Group;
    loaded_bitmap *OutputTarget = Group->OutputTarget;
    rectangle2i ClipRect = Work->ClipRect;
    
    for(u32 BaseAddress = 0;
        BaseAddress < Group->PushBufferSize;
        )
    {
        render_group_entry_header *Header = (render_group_entry_header *)(Group->PushBufferBase + BaseAddress);
        BaseAddress += sizeof(*Header);
        
        void *EntryData = (u8 *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderGroupEntry_render_entry_clear:
            {
                render_entry_clear *Entry = (render_entry_clear *)EntryData;
                
                v2 vMin = V2(0.0f, 0.0f);
                v2 vMax = V2((f32)OutputTarget->Width, (f32)OutputTarget->Height);
                DrawRectangle(OutputTarget, vMin, vMax, Entry->Color, Entry->Color, ClipRect);
                
                BaseAddress += sizeof(*Entry);
            } break;
            
            case RenderGroupEntry_render_entry_bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)EntryData;
                Assert(Entry->Bitmap);
                
                v2 XAxis = V2(Cos(10.0f*Entry->Angle), Sin(10.0f*Entry->Angle));
                v2 YAxis = Perp(XAxis);
                
                v2 ScaledXAxis = V2Multiply(V2Set1(Entry->Dim.X), XAxis);
                v2 ScaledYAxis = V2Multiply(V2Set1(Entry->Dim.Y), YAxis);
                DrawBitmap(OutputTarget, Entry->P, ScaledXAxis, ScaledYAxis, Entry->Color, Entry->Bitmap, ClipRect);
                
                BaseAddress += sizeof(*Entry);
                
            } break;
            case RenderGroupEntry_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)EntryData;
                
                DrawRectangle(OutputTarget, Entry->P, V2Add(Entry->P, Entry->Dim), Entry->Color, Entry->AltColor, ClipRect);
                
                BaseAddress += sizeof(*Entry);
            } break;
            
            
            InvalidDefaultCase;
        }
    }
}


inline void
RenderGroupToOutput(render_group *Group)
{
    loaded_bitmap *OutputTarget = Group->OutputTarget;
    
#define TileCountX 4
#define TileCountY 4
    
    tile_render_work WorkArray[TileCountX*TileCountY];
    
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
            tile_render_work *Work = WorkArray + WorkCount++;
            
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
            
            Work->Group = Group;
            Work->ClipRect = ClipRect;
            
            Platform.AddWorkEntry(Platform.PerFrameWorkQueue, TileRenderWorkThread, Work);
        }
    }
    
    Platform.CompleteAllWork(Platform.PerFrameWorkQueue);
}


typedef enum text_op_type
{
    TextOp_DrawText,
    TextOp_SizeText,
} text_op_type;
internal rectangle2
TextOpInternal(text_op_type Op, render_group *RenderGroup, assets *Assets, v2 P, f32 Scale, string Str, v4 Color)
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
        
        if(CodePoint && CodePoint != ' ')
        {        
            if(Assets->Glyphs[CodePoint].Memory == 0)
            {
                // TODO(kstandbridge): This should be threaded
                Assets->Glyphs[CodePoint] = Platform.DEBUGGetGlyphForCodePoint(&Assets->Arena, CodePoint);
            }
            
            loaded_bitmap *Bitmap = Assets->Glyphs + CodePoint;
            Assert(Bitmap->Memory);
            f32 Height = Scale*Bitmap->Height;
            v2 Offset = V2(AtX, AtY);
            if(Op == TextOp_DrawText)
            {
                PushBitmap(RenderGroup, Bitmap, Height, Offset, Color, 0.0f);
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
        
        if(CodePoint == ' ')
        {
            Result.Max.X += AdvanceX;
        }
        
        AtX += AdvanceX;
    }
    
    return Result;
}

inline void
WriteLine(render_group *RenderGroup, assets *Assets, v2 P, f32 Scale, string Str, v4 Color)
{
    TextOpInternal(TextOp_DrawText, RenderGroup, Assets, P, Scale, Str, Color);
}

inline rectangle2
GetTextSize(render_group *RenderGroup, assets *Assets, v2 P, f32 Scale, string Str, v4 Color)
{
    rectangle2 Result = TextOpInternal(TextOp_SizeText, RenderGroup, Assets, P, Scale, Str, Color);
    
    return Result;
}

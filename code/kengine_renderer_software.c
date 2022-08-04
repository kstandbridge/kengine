internal void
DrawRectangle(loaded_bitmap *Buffer, v2 MinP, v2 MaxP, v4 Color, rectangle2i ClipRect)
{
    rectangle2i FillRect = Rectangle2i(RoundF32ToS32(MinP.X), RoundF32ToS32(MaxP.X),
                                       RoundF32ToS32(MinP.Y), RoundF32ToS32(MaxP.Y));
    
    FillRect = Intersect(FillRect, ClipRect);
    
    Color.R *= Color.A;
    Color.G *= Color.G;
    Color.B *= Color.B;
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
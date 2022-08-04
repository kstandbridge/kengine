internal void
DrawRectangle(loaded_bitmap *Buffer, v2 MinP, v2 MaxP, v4 Color, rectangle2i ClipRect)
{
    rectangle2i FillRect = Rectangle2i(RoundF32ToS32(MinP.X), RoundF32ToS32(MaxP.X),
                                       RoundF32ToS32(MinP.Y), RoundF32ToS32(MaxP.Y));
    
    FillRect = Intersect(FillRect, ClipRect);
    
    u32 Color32 = ((RoundF32ToU32(Color.A * 255.0f) << 24) |
                   (RoundF32ToU32(Color.R * 255.0f) << 16) |
                   (RoundF32ToU32(Color.G * 255.0f) << 8) |
                   (RoundF32ToU32(Color.B * 255.0f) << 0));
    
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
            *Pixel++ = Color32;
        }
        Row += Buffer->Pitch;
    }
}
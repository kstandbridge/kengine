
inline debug_state *
DEBUGGetState()
{
    debug_state *Result = (debug_state *)GlobalDebugMemory->DebugStorage;
    Assert(Result->IsInitialized);
    
    return Result;
}

internal void
DEBUGTextLine(string Text)
{
    debug_state *DebugState = DEBUGGetState();
    
    WriteLine(DebugState->RenderGroup, DebugState->Assets, V2(DebugState->LeftEdge, DebugState->AtY), DebugState->FontScale, Text, V4(0, 0, 0, 1));
    
    DebugState->AtY += Platform.DEBUGGetLineAdvance()*DebugState->FontScale;
}

internal void
DEBUGStart(loaded_bitmap *DrawBuffer)
{
    debug_state *DebugState = DEBUGGetState();
    
    DebugState->AtY = 20.0f;
    DebugState->FontScale = 0.4f;
    
    DebugState->TempMem = BeginTemporaryMemory(&DebugState->Arena);
    DebugState->RenderGroup = AllocateRenderGroup(DebugState->TempMem.Arena, Megabytes(4), DrawBuffer);
}

internal void
DEBUGEnd()
{
    debug_state *DebugState = DEBUGGetState();
    
    tile_render_work Work;
    ZeroStruct(Work);
    Work.Group = DebugState->RenderGroup;
    Work.ClipRect.MinX = 0;
    Work.ClipRect.MaxX = Work.Group->OutputTarget->Width;
    Work.ClipRect.MinY = 0;
    Work.ClipRect.MaxY = Work.Group->OutputTarget->Height;
    TileRenderWorkThread(&Work);
    
    EndTemporaryMemory(DebugState->TempMem);
}

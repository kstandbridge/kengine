
inline debug_state *
DEBUGGetState()
{
    debug_state *Result = (debug_state *)GlobalDebugMemory->DebugStorage;
    Assert(Result->IsInitialized);
    
    return Result;
}

internal void
DEBUGTextLine(char *Format, ...)
{
    debug_state *DebugState = DEBUGGetState();
    
    va_list ArgsList;
    
    va_start(ArgsList, Format);
    string Text = FormatStringInternal(&DebugState->Arena, Format, ArgsList);
    va_end(ArgsList);
    
    WriteLine(DebugState->RenderGroup, DebugState->Assets, V2(DebugState->LeftEdge, DebugState->AtY), DebugState->FontScale, Text, V4(0, 0, 0, 1), InfinityRectangle2());
    
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
    
    RenderGroupToOutput(DebugState->RenderGroup);
    
    EndTemporaryMemory(DebugState->TempMem);
}

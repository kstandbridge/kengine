
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
    
    WriteLine(&DebugState->RenderGroup, DebugState->Assets, V2(DebugState->LeftEdge, DebugState->AtY), DebugState->FontScale, Text, V4(1, 1, 1, 1), InfinityRectangle2(), F32Max);
    DebugState->AtY += Platform.DEBUGGetLineAdvance()*DebugState->FontScale;
}

internal void
DEBUGStart()
{
    debug_state *DebugState = DEBUGGetState();
    
    DebugState->AtY = 20.0f;
    DebugState->FontScale = 1.0f;
    
    DebugState->TempMem = BeginTemporaryMemory(&DebugState->Arena);
    DebugState->RenderGroup = BeginRenderGroup(DebugState->Assets, DebugState->RenderCommands);
}

internal void
DEBUGEnd()
{
    debug_state *DebugState = DEBUGGetState();
    
    EndRenderGroup(&DebugState->RenderGroup);
    
    EndTemporaryMemory(DebugState->TempMem);
}

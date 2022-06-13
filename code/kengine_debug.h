#ifndef KENGINE_DEBUG_H

typedef struct debug_state
{
    memory_arena Arena;
    
    assets *Assets;
    
    f32 LeftEdge;
    f32 AtY;
    f32 FontScale;
    
    b32 IsInitialized;
    
    // NOTE(kstandbridge): Transient
    temporary_memory TempMem;
    render_group *RenderGroup;
    
} debug_state;


internal void DEBUGTextLine(string Text);

#define KENGINE_DEBUG_H
#endif //KENGINE_DEBUG_H

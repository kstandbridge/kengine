#ifndef KENGINE_RENDER_GROUP_H

typedef struct
{
    render_commands *Commands;
    u16 OldClipRectIndex;
    u16 CurrentClipRectIndex;
    
    loaded_glyph *Glyphs;
    
    // TODO(kstandbridge): Arena will be removed when we no longer load glyphs in draw routine
    memory_arena *Arena;
    
    struct colors *Colors;
    
} render_group;

typedef struct
{
    v2 Size;
    v2 Align;
    v2 P;
} bitmap_dim;

#define KENGINE_RENDER_GROUP_H
#endif //KENGINE_RENDER_GROUP_H

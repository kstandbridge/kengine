#ifndef KENGINE_RENDERER_H

#define MaxVertexCount 32768
#define MaxRenderCommands 512

#define VertexInstanceSize sizeof(vertex_instance)
#define VertexBufferSize MaxVertexCount*VertexInstanceSize

#define RGBv4(R, G ,B) V4((f32)R/255.0f,(f32)G/255.0f,(f32)B/255.0f, 1.0f)

typedef struct vertex_instance
{
    v3 Offset;
    v2 Size;
    v4 Color;
    v4 UV;
} vertex_instance;

typedef enum render_command_type
{
    RenderCommand_Rect,
    
    RenderCommand_Sprite,
    RenderCommand_Glyph,
} render_command_type;

typedef struct render_command
{
    render_command_type Type;
    
    void *Context;
    
    u32 VertexCount;
    umm VertexBufferOffset;
    
} render_command;

typedef struct render_group
{
    v4 ClearColor;
    
    render_command Commands[MaxRenderCommands];
    u32 CurrentCommand;
    
    u32 Width;
    u32 Height;
    
    u8 VertexBuffer[VertexBufferSize];
    umm VertexBufferAt; 
    
} render_group;

render_command *
PushRenderCommandRect(render_group *Group, rectangle2 Bounds, f32 Depth, v4 Color);

void
PushRenderCommandRectOutline(render_group *Group, rectangle2 Bounds, f32 Depth, f32 Thickness, v4 Color);

void
PushRenderCommandAlternateRectOutline(render_group *Group, rectangle2 Bounds, f32 Depth, f32 Thickness, v4 Color1, v4 Color2);

render_command *
PushRenderCommandSprite(render_group *Group, v2 Offset, f32 Depth, v2 Size, v4 Color, v4 UV, void *Texture);

render_command *
PushRenderCommandGlyph(render_group *Group, v2 Offset, f32 Depth, v2 Size, v4 Color, v4 UV, void *Texture);

#define KENGINE_RENDERER_H
#endif //KENGINE_RENDERER_H


#include "kengine_render_group.h"

#define GL_FRAMEBUFFER_SRGB               0x8DB9
#define GL_SRGB8_ALPHA8                   0x8C43

#define GL_SHADING_LANGUAGE_VERSION       0x8B8C

// NOTE(kstandbridge): Windows-specific
#define WGL_CONTEXT_MAJOR_VERSION_ARB           0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB           0x2092
#define WGL_CONTEXT_LAYER_PLANE_ARB             0x2093
#define WGL_CONTEXT_FLAGS_ARB                   0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB            0x9126

#define WGL_CONTEXT_DEBUG_BIT_ARB               0x0001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB  0x0002

#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB        0x00000001
#define WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB 0x00000002

typedef struct opengl_info
{
    b32 ModernContext;
    
    char *Vendor;
    char *Renderer;
    char *Version;
    char *ShadingLanguageVersion;
    char *Extensions;
    
    b32 GL_EXT_texture_sRGB;
    b32 GL_EXT_framebuffer_sRGB;
} opengl_info;

internal opengl_info
OpenGLGetInfo(b32 ModernContext)
{
    opengl_info Result;
    
    Result.ModernContext = ModernContext;
    Result.Vendor = (char *)glGetString(GL_VENDOR);
    Result.Renderer = (char *)glGetString(GL_RENDERER);
    Result.Version = (char *)glGetString(GL_VERSION);
    if(Result.ModernContext)
    {
        Result.ShadingLanguageVersion = (char *)glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        Result.ShadingLanguageVersion = "(none)";
    }
    
    Result.Extensions = (char *)glGetString(GL_EXTENSIONS);
    
    char *At = Result.Extensions;
    while(*At)
    {
        while(IsWhitespace(*At)) {++At;}
        char *End = At;
        while(*End && !IsWhitespace(*End)) {++End;}
        
        umm Count = End - At;        
        
        if(0) {}
        else if(StringsAreEqual(StringInternal(Count, (u8 *)At), String("GL_EXT_texture_sRGB"))) {Result.GL_EXT_texture_sRGB=true;}
        else if(StringsAreEqual(StringInternal(Count, (u8 *)At), String("GL_EXT_framebuffer_sRGB"))) {Result.GL_EXT_framebuffer_sRGB=true;}
        
        At = End;
    }
    
    return(Result);
}

internal void
OpenGLInit(b32 ModernContext)
{
    opengl_info Info = OpenGLGetInfo(ModernContext);
    
    OpenGLDefaultInternalTextureFormat = GL_RGBA8;
    if(Info.GL_EXT_texture_sRGB)
    {
        OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
    }
    
    // TODO(kstandbridge): Need to go back and use extended version of choose pixel format
    // to ensure that our framebuffer is marked as SRGB?
    if(Info.GL_EXT_framebuffer_sRGB)
    {
        glEnable(GL_FRAMEBUFFER_SRGB);
    }
}       

inline void
OpenGLSetScreenspace(s32 Width, s32 Height)
{
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    glMatrixMode(GL_PROJECTION);
    f32 a = SafeRatio1(2.0f, (f32)Width);
    f32 b = SafeRatio1(2.0f, (f32)Height);
    f32 Proj[] =
    {
        a,  0,  0,  0,
        0,  b,  0,  0,
        0,  0,  1,  0,
        -1, -1,  0,  1,
    };
    glLoadMatrixf(Proj);
}

inline void
OpenGLRectangle(v2 MinP, v2 MaxP, v4 Color)
{                    
    glBegin(GL_TRIANGLES);
    
    glColor4f(Color.R, Color.G, Color.B, Color.A);
    
    // NOTE(kstandbridge): Lower triangle
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(MinP.X, MinP.Y);
    
    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(MaxP.X, MinP.Y);
    
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.X, MaxP.Y);
    
    // NOTE(kstandbridge): Upper triangle
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(MinP.X, MinP.Y);
    
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.X, MaxP.Y);
    
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(MinP.X, MaxP.Y);
    
    glEnd();
}

inline void
OpenGLDisplayBitmap(s32 Width, s32 Height, void *Memory, int Pitch)
{
    Assert(Pitch == (Width*4));
    glViewport(0, 0, Width, Height);
    
    glBindTexture(GL_TEXTURE_2D, 1);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, Width, Height, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Memory);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    glEnable(GL_TEXTURE_2D);
    
    glClearColor(1.0f, 0.0f, 1.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    OpenGLSetScreenspace(Width, Height);
    
    // TODO(kstandbridge): Decide how we want to handle aspect ratio - black bars or crop?
    
    v2 MinP = V2(0, 0);
    v2 MaxP = V2((f32)Width, (f32)Height);
    v4 Color = V4(1, 1, 1, 1);
    
    OpenGLRectangle(MinP, MaxP, Color);
}


inline void *
AllocateTexture(u32 Width, u32 Height, void *Data)
{
    GLuint Handle;
    glGenTextures(1, &Handle);
    glBindTexture(GL_TEXTURE_2D, Handle);
    glTexImage2D(GL_TEXTURE_2D, 0, 
                 OpenGLDefaultInternalTextureFormat, 
                 Width, Height, 0,
                 GL_BGRA_EXT, GL_UNSIGNED_BYTE, Data);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    glBindTexture(GL_TEXTURE_2D, 0);
    
    return((void *)Handle);
}

// TODO(kstandbridge): We need to actually deallocate textures
inline void
DeallocateTexture(void *TextureHandle)
{
    GLuint Handle = (GLuint)TextureHandle;
    glDeleteTextures(1, &Handle);
}


internal void
OpenGLRenderCommands(app_render_commands *RenderCommands)
{    
    glViewport(0, 0, RenderCommands->Width, RenderCommands->Height);
    
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();
    
    OpenGLSetScreenspace(RenderCommands->Width, RenderCommands->Height);
    u32 SortEntryCount = RenderCommands->PushBufferElementCount;
    tile_sort_entry *SortedEntries = (tile_sort_entry *)(RenderCommands->PushBufferBase + RenderCommands->SortEntryAt);
    
    tile_sort_entry *SortEntry = SortedEntries;
    for(u32 SortEntryIndex = 0;
        SortEntryIndex < SortEntryCount;
        ++SortEntryIndex, ++SortEntry)
    {
        render_group_entry_header *Header = (render_group_entry_header *)(RenderCommands->PushBufferBase + SortEntry->PushBufferOffset);
        
        void *EntryData = (u8 *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderGroupEntry_render_entry_clear:
            {
                render_entry_clear *RenderEntry = (render_entry_clear *)EntryData;
                
                glClearColor(RenderEntry->Color.R, RenderEntry->Color.G, RenderEntry->Color.B, RenderEntry->Color.A);
                glClear(GL_COLOR_BUFFER_BIT);
            } break;
            
            case RenderGroupEntry_render_entry_bitmap:
            {
                render_entry_bitmap *RenderEntry = (render_entry_bitmap *)EntryData;
                // TODO(kstandbridge): We could instead only draw the bitmap if we have the data, assume its loaded in a background thread
                Assert(RenderEntry->Bitmap);
                
#if 0                
                // TODO(kstandbridge): Add back rotation when we have Cos/Sin
                v2 XAxis = V2(Cos(10.0f*RenderEntry->Angle), Sin(10.0f*RenderEntry->Angle));
                v2 YAxis = Perp(XAxis);
#else
                v2 XAxis = V2(1, 0);
                v2 YAxis = V2(0, 1);
#endif
                
                v2 ScaledXAxis = V2Multiply(V2Set1(RenderEntry->Dim.X), XAxis);
                v2 ScaledYAxis = V2Multiply(V2Set1(RenderEntry->Dim.Y), YAxis);
                v2 MinP = RenderEntry->P;
                v2 MaxP = V2Add(MinP, V2Add(ScaledXAxis, ScaledYAxis));
                
                if(RenderEntry->Bitmap->TextureHandle == 0)
                {
                    RenderEntry->Bitmap->TextureHandle = AllocateTexture(RenderEntry->Bitmap->Width, RenderEntry->Bitmap->Height, RenderEntry->Bitmap->Memory);
                }
                
                
                glBindTexture(GL_TEXTURE_2D, (GLuint)RenderEntry->Bitmap->TextureHandle);
                OpenGLRectangle(MinP, MaxP, RenderEntry->Color);
                
            } break;
            case RenderGroupEntry_render_entry_rectangle:
            {
                glDisable(GL_TEXTURE_2D);
                render_entry_rectangle *RenderEntry = (render_entry_rectangle *)EntryData;
                OpenGLRectangle(RenderEntry->P, V2Add(RenderEntry->P, RenderEntry->Dim), RenderEntry->Color);
                glEnable(GL_TEXTURE_2D);
            } break;
            
            case RenderGroupEntry_render_entry_text:
            {
                render_entry_text *RenderEntry = (render_entry_text *)EntryData;
                v2 P = RenderEntry->P;
                v4 Color = RenderEntry->Color;
                string Text = RenderEntry->Text;
                
                glDisable(GL_TEXTURE_2D);
                glRasterPos2f(P.X, P.Y); 
                glColor4f(Color.R, Color.G, Color.B, Color.A);
                glListBase(1000);
                glCallLists((GLsizei)Text.Size, GL_UNSIGNED_BYTE, Text.Data);
                glEnable(GL_TEXTURE_2D);
                
            } break;
            
            
            InvalidDefaultCase;
        }
    }
}

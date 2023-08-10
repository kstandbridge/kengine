#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

#define USE_DEPTH_BUFFER 1
#define USE_STENCIL_BUFFER 0

typedef char GLchar;

// NOTE(kstandbridge): 0 to disable, enable in powers of 2
#define MULTI_SAMPLING 4

#if KENGINE_INTERNAL
    #define AssertGL(Op) do \
    { \
        Op; \
        GLenum Error = glGetError(); \
        Assert(Error == GL_NO_ERROR); \
    } while(0)
#else
    #define AssertGL(Op) Op
#endif

#if KENGINE_INTERNAL

    #define GL_DEBUG_SEVERITY_HIGH_ARB        0x9146
    #define GL_DEBUG_SEVERITY_LOW_ARB         0x9148
    #define GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB   0x8242

    typedef void gl_debug_proc_arb(GLenum source,GLenum type,GLuint id,GLenum severity,GLsizei length,const GLchar *message,const void *userParam);
    typedef void gl_debug_message_callback_arb(gl_debug_proc_arb callback, const void *userParam);

    internal void
    OpenGLDebugCallback(GLenum Source, GLenum Type, GLuint Id, GLenum Severity,
                        GLsizei Length, const GLchar *Message, const void *User)
    {
        PlatformConsoleOut("Error: %S", String_(Length, (u8 *)Message));
        if((Severity >= GL_DEBUG_SEVERITY_LOW_ARB) &&
           (Severity <= GL_DEBUG_SEVERITY_HIGH_ARB))
        {
            Assert(0);
        }
    }

    internal void
    SetupOpenGLDebugCallback()
    {
        const GLubyte *ExtensionString = 0;
        AssertGL(ExtensionString = glGetString(GL_EXTENSIONS));
        if(ExtensionString)
        {
            // NOTE(kstandbridge): Parse Extensions
            {
                const GLubyte *Start = ExtensionString;
                const GLubyte *At = ExtensionString;
                while(*At)
                {
                    if(IsWhitespace(*At++))
                    {
                        umm Length = At - Start - 1;
                        string Actual = String_(Length, (u8 *)Start);
                        Start = At;
                        
                        if(StringsAreEqual(String("GL_ARB_debug_output"), Actual))
                        {
                            gl_debug_message_callback_arb *glDebugMessageCallbackARB =
                            #if KENGINE_WIN32
                                (gl_debug_message_callback_arb *)wglGetProcAddress("glDebugMessageCallbackARB");
                            #elif KENGINE_LINUX
                                (gl_debug_message_callback_arb *)glXGetProcAddressARB((void *)"glDebugMessageCallbackARB");
                            #else
                                #error Missing gl_debug_message_callback_arb for platform
                            #endif

                            Assert(glDebugMessageCallbackARB);
                            AssertGL(glDebugMessageCallbackARB(OpenGLDebugCallback, 0));
                            AssertGL(glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS_ARB));

                            break;
                        }
                    }
                }
            }
        }
        else
        {
            PlatformConsoleOut("Error: glGetString failed!");
        }
    }
#endif


internal rectangle2i
AspectRatioFit(u32 RenderWidth, u32 RenderHeight,
               u32 WindowWidth, u32 WindowHeight)
{
    rectangle2i Result = {};
    
    if((RenderWidth > 0) &&
       (RenderHeight > 0) &&
       (WindowWidth > 0) &&
       (WindowHeight > 0))
    {
        f32 OptimalWindowWidth = (f32)WindowHeight * ((f32)RenderWidth / (f32)RenderHeight);
        f32 OptimalWindowHeight = (f32)WindowWidth * ((f32)RenderHeight / (f32)RenderWidth);

        if(OptimalWindowWidth > (f32)WindowWidth)
        {
            // NOTE(casey): Width-constrained display - top and bottom black bars
            Result.MinX = 0;
            Result.MaxX = WindowWidth;

            f32 Empty = (f32)WindowHeight - OptimalWindowHeight;
            s32 HalfEmpty = RoundF32ToS32(0.5f*Empty);
            s32 UseHeight = RoundF32ToS32(OptimalWindowHeight);

            Result.MinY = HalfEmpty;
            Result.MaxY = Result.MinY + UseHeight;
        }
        else
        {
            // NOTE(casey): Height-constrained display - left and right black bars
            Result.MinY = 0;
            Result.MaxY = WindowHeight;

            f32 Empty = (f32)WindowWidth - OptimalWindowWidth;
            s32 HalfEmpty = RoundF32ToS32(0.5f*Empty);
            s32 UseWidth = RoundF32ToS32(OptimalWindowWidth);

            Result.MinX = HalfEmpty;
            Result.MaxX = Result.MinX + UseWidth;
        }
    }

    return(Result);
}

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"



internal void
OpenGLRenderInit()
{
    // TODO(kstandbridge): Load GL extensions and set up GL state
    AssertGL(glEnable(GL_FRAMEBUFFER_SRGB));
}

internal void
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

internal void
OpenGLDestroy()
{
    
}

internal void
OpenGLRectangle(v2 MinP, v2 MaxP, v4 Color)
{                    
    glBegin(GL_TRIANGLES);

    glColor4f(Color.R, Color.G, Color.B, Color.A);
    
    // NOTE(casey): Lower triangle
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(MinP.X, MinP.Y);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(MaxP.X, MinP.Y);
    
    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.X, MaxP.Y);

    // NOTE(casey): Upper triangle
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(MinP.X, MinP.Y);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(MaxP.X, MaxP.Y);

    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(MinP.X, MaxP.Y);
    
    glEnd();
}

inline v2
V2ScalarMultiply(f32 A, v2 B)
{
    v2 Result =
    {
        .X = A*B.X,
        .Y = A*B.Y,
    };

    return Result;
}


// TODO(kstandbridge): ditch this
global u32 TextureBindCount;

internal void
OpenGLRenderFrame(app_render_commands *Commands)
{
    rectangle2i DrawRegion = AspectRatioFit(Commands->RenderWidth, Commands->RenderHeight,
                                            Commands->WindowWidth, Commands->WindowHeight);
    u32 WindowWidth = GetWidth(DrawRegion);
    u32 WindowHeight = GetHeight(DrawRegion);
    
    AssertGL(glViewport(DrawRegion.MinX, DrawRegion.MinY, WindowWidth, WindowHeight));

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    OpenGLSetScreenspace(Commands->RenderWidth, Commands->RenderHeight);

    AssertGL(glClearColor(0, 0, 0, 0));
    glClear(GL_COLOR_BUFFER_BIT);

    u32 SortEntryCount = Commands->PushBufferElementCount;
    tile_sort_entry *SortEntries = (tile_sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);

    tile_sort_entry *Entry = SortEntries;
    for(u32 SortEntryIndex = 0;
        SortEntryIndex < SortEntryCount;
        ++SortEntryIndex, ++Entry)
    {
        render_group_entry_header *Header = 
            (render_group_entry_header *)(Commands->PushBufferBase + Entry->PushBufferOffet);
        
        void *Data = (u8 *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderGroupEntryType_render_entry_rectangle:
            {
                render_entry_rectangle *Entry = (render_entry_rectangle *)Data;

                glDisable(GL_TEXTURE_2D);
                OpenGLRectangle(Entry->P, V2Add(Entry->P, Entry->Dim), Entry->Color);
                glEnable(GL_TEXTURE_2D);

            } break;

            case RenderGroupEntryType_render_entry_bitmap:
            {
                render_entry_bitmap *Entry = (render_entry_bitmap *)Data;
                Assert(Entry->Bitmap);

                v2 XAxis = {1, 0};
                v2 YAxis = {0, 1};

                v2 MinP = Entry->P;
                v2 MaxP = V2Add(V2Add(MinP, V2ScalarMultiply(Entry->Size.X, XAxis)),
                                V2ScalarMultiply(Entry->Size.Y, YAxis));

                if(Entry->Bitmap->Handle)
                {
                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);
                }
                else
                {
                    Entry->Bitmap->Handle = ++TextureBindCount;
                    glBindTexture(GL_TEXTURE_2D, Entry->Bitmap->Handle);

                    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, Entry->Bitmap->Width, Entry->Bitmap->Height, 0,
                                GL_BGRA_EXT, GL_UNSIGNED_BYTE, Entry->Bitmap->Memory);

                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
                    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
                    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
                }

                OpenGLRectangle(Entry->P, MaxP, Entry->Color);
            } break;

            InvalidDefaultCase;
        }
    }
}

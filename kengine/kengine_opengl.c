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

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

global s32 SpriteWidth;
global s32 SpriteHeight;
global s32 SpriteComp;
global stbi_uc *SpriteBytes;
global GLuint TextureHandle;

internal void
OpenGLRenderInit()
{
    // TODO(kstandbridge): Load GL extensions and set up GL state
    memory_arena Arena = {0};
    string File = PlatformReadEntireFile(&Arena, String("sprite.png"));
    SpriteBytes = stbi_load_from_memory(File.Data, (s32)File.Size, &SpriteWidth, &SpriteHeight, &SpriteComp, 4);

    glGenTextures(1, &TextureHandle);

    AssertGL(glClearColor(100.0f/255.0f, 149.0f/255.0f, 237.0f/255.0f, 1.0f));
}

internal void
OpenGLDestroy()
{
    
}

internal void
OpenGLRenderResize(u32 Width, u32 Height)
{
    AssertGL(glViewport(0, 0, Width, Height));
}

internal void
OpenGLRenderFrame()
{
    glBindTexture(GL_TEXTURE_2D, TextureHandle);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, SpriteWidth, SpriteHeight, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, SpriteBytes);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    glEnable(GL_TEXTURE_2D);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
        
    glBegin(GL_TRIANGLES);

    f32 P = 0.9f;

    // NOTE(kstandbridge): Lower triangle
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-P, -P);

    glTexCoord2f(1.0f, 1.0f);
    glVertex2f(P, -P);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(P, P);

    // NOTE(kstandbridge): Upper triangle
    glTexCoord2f(0.0f, 1.0f);
    glVertex2f(-P, -P);

    glTexCoord2f(1.0f, 0.0f);
    glVertex2f(P, P);
    
    glTexCoord2f(0.0f, 0.0f);
    glVertex2f(-P, P);
    
    AssertGL(glEnd());
}

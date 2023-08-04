#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540

#define USE_DEPTH_BUFFER 1
#define USE_STENCIL_BUFFER 0

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
                            PFNGLDEBUGMESSAGECALLBACKARBPROC glDebugMessageCallbackARB =
                            #if KENGINE_WIN32
                                (void *)wglGetProcAddress("glDebugMessageCallbackARB");
                            #elif KENGINE_LINUX
                                (PFNGLDEBUGMESSAGECALLBACKARBPROC)glXGetProcAddressARB((void *)"glDebugMessageCallbackARB");
                            #else
                                #error Missing glDebugMessageCallbackARB for platform
                            #endif                               
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

internal void
OpenGLRenderInit()
{
    //TODO (kstandbridge): Load GL extensions and set up GL state

    AssertGL(glClearColor(100.0f/255.0f, 149.0f/255.0f, 237.0f/255.0f, 1.0f));
}

internal void
OpenGLDestory()
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
    AssertGL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT));

    glBegin(GL_TRIANGLES);
    
    glColor3f(1.f, 0.f, 0.f);
    glVertex2f(0.f, 0.5f);
    glColor3f(0.f, 1.f, 0.f);
    glVertex2f(0.5f, -0.5f);
    glColor3f(0.f, 0.f, 1.f);
    glVertex2f(-0.5f, -0.5f);
    
    AssertGL(glEnd());
}

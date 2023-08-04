#include <X11/Xlib.h>
#include <GL/gl.h>
#include <GL/glx.h>

// NOTE(kstandbridge): Downloaded from https://www.opengl.org/registry/api/GL/
#include "kengine/glext.h"
#include "kengine/glxext.h"

#if KENGINE_INTERNAL
    #define AssertGL(Op) \
    { \
        Op; \
        GLenum Error = glGetError(); \
        Assert(Error == GL_NO_ERROR); \
    }

    internal void
    OpenGLDebugCallback(GLenum Source, GLenum Type, GLuint Id, GLenum Severity,
                        GLsizei Length, const GLchar *Message, const void *User)
    {
        PlatformConsoleOut("Error: %S", String_(Length, (u8 *)Message));
        if((Severity >= GL_DEBUG_SEVERITY_LOW) &&
           (Severity <= GL_DEBUG_SEVERITY_HIGH_ARB))
        {
            Assert(0);
        }
    }
#else
    #define AssertGL(Op) Op
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

typedef struct app_memory
{
    struct app_state *AppState;

    u32 ArgCount;
    char **Args;

} app_memory;
global app_memory GlobalAppMemory;

s32
main(int ArgCount, char *Args[])
{
    s32 Result = 0;

    GlobalAppMemory.ArgCount = ArgCount - 1;
    GlobalAppMemory.Args = Args + 1;

#if KENGINE_WIN32
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
#elif KENGINE_LINUX
    GlobalLinuxState.MemorySentinel.Prev = &GlobalLinuxState.MemorySentinel;
    GlobalLinuxState.MemorySentinel.Next = &GlobalLinuxState.MemorySentinel;
#endif

    //Result = MainLoop(&GlobalAppMemory);

    Display *Display = XOpenDisplay(0);
    if(Display)
    {
        const char *GlxExtensions = glXQueryExtensionsString(Display, DefaultScreen(Display));
        if(GlxExtensions)
        {
            b32 glx_ARB_create_context = false;
            b32 glx_ARB_multisample = false;
            b32 glx_ARB_framebuffer_sRGB = false;
            b32 glx_SGI_swap_control = false;
            b32 glx_MESA_swap_control = false;
            b32 glx_EXT_swap_control = false;
            b32 glx_EXT_swap_control_tear = false;
            
            // NOTE(kstandbridge): Parse XQueryExtensions
            {
                const char *Start = GlxExtensions;
                const char *At = GlxExtensions;
                while(*At)
                {
                    if(IsWhitespace(*At++))
                    {
                        umm Length = At - Start - 1;
                        string Actual = String_(Length, (u8 *)Start);
                        Start = At;
                        
                        if(StringsAreEqual(String("GLX_ARB_create_context"), Actual))
                        {
                            glx_ARB_create_context = true;
                        }
                        else if(StringsAreEqual(String("GLX_ARB_multisample"), Actual))
                        {
                            glx_ARB_multisample = true;
                        }
                        else if(StringsAreEqual(String("GLX_ARB_framebuffer_sRGB"), Actual))
                        {
                            glx_ARB_framebuffer_sRGB = true;
                        }
                        else if(StringsAreEqual(String("GLX_SGI_swap_control"), Actual))
                        {
                            glx_SGI_swap_control = true;
                        }
                        else if(StringsAreEqual(String("GLX_MESA_swap_control"), Actual))
                        {
                            glx_MESA_swap_control = true;
                        }
                        else if(StringsAreEqual(String("GLX_EXT_swap_control"), Actual))
                        {
                            glx_EXT_swap_control = true;
                        }
                        else if(StringsAreEqual(String("GLX_EXT_swap_control_tear"), Actual))
                        {
                            glx_EXT_swap_control_tear = true;
                        }
                    }
                }
            }

            s32 AttributeList[32];
            // NOTE(kstandbridge): Set attribute list
            {
                s32 *At = AttributeList;

    #define USE_DEPTH_BUFFER 1
    #define USE_STENCIL_BUFFER 1

    // NOTE(kstandbridge): 0 to disable, enable in powers of 2
    #define MULTI_SAMPLING 4

                *At++ = GLX_X_VISUAL_TYPE; *At++ = GLX_TRUE_COLOR;
                *At++ = GLX_DOUBLEBUFFER;  *At++ = true;
                *At++ = GLX_RED_SIZE;      *At++ = 8;
                *At++ = GLX_GREEN_SIZE;    *At++ = 8;
                *At++ = GLX_BLUE_SIZE;     *At++ = 8;
                *At++ = GLX_DEPTH_SIZE;    *At++ = USE_DEPTH_BUFFER ? 24 : 0;
                *At++ = GLX_STENCIL_SIZE;  *At++ = USE_STENCIL_BUFFER ? 8 : 0;
                if (glx_ARB_framebuffer_sRGB)
                {
                    *At++ = GLX_FRAMEBUFFER_SRGB_CAPABLE_ARB;
                    *At++ = true;
                }
                if (glx_ARB_multisample)
                {
                    *At++ = GLX_SAMPLE_BUFFERS_ARB;
                    *At++ = 1;
                    *At++ = GLX_SAMPLES_ARB;
                    *At++ = MULTI_SAMPLING;
                }
                *At++ = 0;
            }

            s32 FBConfigItemCount;
            GLXFBConfig *FBConfig = glXChooseFBConfig(Display, DefaultScreen(Display), AttributeList, &FBConfigItemCount);
            if(FBConfig && FBConfigItemCount)
            {
                XVisualInfo *VisualInfo = glXGetVisualFromFBConfig(Display, FBConfig[0]);
                if(VisualInfo)
                {
                    Window RootWindow = DefaultRootWindow(Display);

                    Colormap Colormap = XCreateColormap(Display, RootWindow, VisualInfo->visual, AllocNone);
                    if(Colormap)
                    {
                        XSetWindowAttributes SetWindowAttributes =
                        {
                            .colormap = Colormap,
                            .event_mask = StructureNotifyMask
                        };
#define WINDOW_WIDTH 960
#define WINDOW_HEIGHT 540
                        Window Window = XCreateWindow(Display, RootWindow, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                                      0, VisualInfo->depth, InputOutput, VisualInfo->visual,
                                                      CWColormap | CWEventMask, &SetWindowAttributes);
                        XSelectInput(Display, Window, ExposureMask | ButtonPressMask | KeyPressMask);
                        if(Window)
                        {
                            XSizeHints *SizeHints = XAllocSizeHints();
                            if(SizeHints)
                            {
                                SizeHints->flags |= PMinSize | PMaxSize;
                                SizeHints->min_width = SizeHints->max_width = WINDOW_WIDTH;
                                SizeHints->min_height = SizeHints->max_height = WINDOW_HEIGHT;
                                XSetWMNormalHints(Display, Window, SizeHints);
                                XFree(SizeHints);
                            }
                            else
                            {
                                PlatformConsoleOut("Error: XAllocSizeHints failed!");
                            }

                            Atom WM_DELETE_WINDOW = XInternAtom(Display, "WM_DELETE_WINDOW", false);
                            XSetWMProtocols(Display, Window, &WM_DELETE_WINDOW, 1);

                            XStoreName(Display, Window, "kengine OpenGL");
                            XMapWindow(Display, Window);

                            GLXContext Context = 0;
                            if(glx_ARB_create_context)
                            {
                                s32 ContextAttributesList[] =
                                {
                                    GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
                                    GLX_CONTEXT_MINOR_VERSION_ARB, 0,
#if KENGINE_INTERNAL
                                    GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_DEBUG_BIT_ARB,
#endif
                                    GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                                    None
                                };
                                PFNGLXCREATECONTEXTATTRIBSARBPROC glXCreateContextAttribsARB = 
                                    (PFNGLXCREATECONTEXTATTRIBSARBPROC)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
                                Context = glXCreateContextAttribsARB(Display, FBConfig[0], 0, true, ContextAttributesList);
                                if(!Context)
                                {
                                    PlatformConsoleOut("Warning: Failed to create modern OpenGL context, trying legacy context!\n");
                                }
                            }

                            if(!Context)
                            {
                                Context = glXCreateContext(Display, VisualInfo, 0, GL_TRUE);
                            }

                            if(Context)
                            {

                                if(glXMakeCurrent(Display, Window, Context))
                                {
                                    
                                    // NOTE(kstandbridge): Setup vSync
                                    {
                                        if(glx_EXT_swap_control)
                                        {
                                            PFNGLXSWAPINTERVALEXTPROC glXSwapIntervalEXT =
                                                (PFNGLXSWAPINTERVALEXTPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalEXT");
                                            glXSwapIntervalEXT(Display, Window, glx_EXT_swap_control_tear ? -1 : 1);
                                        }
                                        else if(glx_MESA_swap_control)
                                        {
                                            PFNGLXSWAPINTERVALMESAPROC glXSwapIntervalMESA =
                                                (PFNGLXSWAPINTERVALMESAPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalMESA");
                                            if(glXSwapIntervalMESA(1))
                                            {
                                                PlatformConsoleOut("Error: glXSwapIntervalMESA failed!");
                                            }
                                        }
                                        else if(glx_SGI_swap_control)
                                        {
                                            PFNGLXSWAPINTERVALSGIPROC glXSwapIntervalSGI =
                                                (PFNGLXSWAPINTERVALSGIPROC)glXGetProcAddressARB((const GLubyte*)"glXSwapIntervalSGI");
                                            if(glXSwapIntervalSGI(1))
                                            {
                                                PlatformConsoleOut("Error: glXSwapIntervalSGI failed!");
                                            }
                                        }
                                    }

                                    // NOTE(kstandbridge): Setup debug mode
#if KENGINE_INTERNAL
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
                                                                (PFNGLDEBUGMESSAGECALLBACKARBPROC)glXGetProcAddressARB((const GLubyte*)"glDebugMessageCallbackARB");
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

                                    if(glx_ARB_multisample)
                                    {
                                        AssertGL(glEnable(GL_MULTISAMPLE_ARB));
                                    }

                                    OpenGLRenderInit();

                                    b32 IsRunning = true;
                                    while(IsRunning)
                                    {
                                        if(XPending(Display))
                                        {
                                            XEvent Event;
                                            XNextEvent(Display, &Event);

                                            if(Event.type == ConfigureNotify)
                                            {
                                                OpenGLRenderResize(Event.xconfigure.width, Event.xconfigure.height);
                                            }
                                            else if(Event.type == ClientMessage)
                                            {
                                                if(Event.xclient.data.l[0] == WM_DELETE_WINDOW)
                                                {
                                                    break;
                                                }
                                            }
                                            else if((Event.type == KeyPress) &&
                                                    (Event.xkey.keycode == 9)) // KEY_ESCAPE
                                            {
                                                IsRunning = false;
                                            }

                                            continue;
                                        }

                                        OpenGLRenderFrame();

                                        glXSwapBuffers(Display, Window);
                                    }

                                    OpenGLDestory();

                                    glXMakeCurrent(Display, None, 0);
                                }
                                else
                                {
                                    PlatformConsoleOut("Error: glXMakeCurrent failed!");
                                }
                                glXDestroyContext(Display, Context);
                            }
                            else
                            {
                                PlatformConsoleOut("Error: glXCreateContext failed!");
                            }
                            XDestroyWindow(Display, Window);
                        }
                        else
                        {
                            PlatformConsoleOut("Error: XCreateWindow failed!");
                        }
                        XFreeColormap(Display, Colormap);
                    }
                    else
                    {
                        PlatformConsoleOut("Error: XCreateColormap failed!");
                    }
                }
                else
                {
                    PlatformConsoleOut("Error: glXGetVisualFromFBConfig failed!");
                }
                XFree(VisualInfo);
            }
            else
            {
                PlatformConsoleOut("Error: glXChooseFBConfig failed!");
            }
            XFree(FBConfig);
        }
        else
        {
            PlatformConsoleOut("Error: glXQueryExtensionsString failed!\n");
        }
        XCloseDisplay(Display);
    }
    else
    {
        PlatformConsoleOut("Error: XOpenDisplay failed\n");
    }

    return Result;
}
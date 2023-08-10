
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

    GlobalLinuxState.MemorySentinel.Prev = &GlobalLinuxState.MemorySentinel;
    GlobalLinuxState.MemorySentinel.Next = &GlobalLinuxState.MemorySentinel;

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
                        Window Window = XCreateWindow(Display, RootWindow, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT,
                                                      0, VisualInfo->depth, InputOutput, VisualInfo->visual,
                                                      CWColormap | CWEventMask, &SetWindowAttributes);
                        XSelectInput(Display, Window, StructureNotifyMask | ExposureMask | ButtonPressMask | KeyPressMask);
                        if(Window)
                        {
#if 0
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
#endif
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

#if KENGINE_INTERNAL
                                    SetupOpenGLDebugCallback();
#endif

                                    if(glx_ARB_multisample)
                                    {
                                        AssertGL(glEnable(GL_MULTISAMPLE_ARB));
                                    }

                                    OpenGLRenderInit();

                                    // TODO(kstandbridge): Figure out our pushbuffer size;
                                    u32 PushBufferSize = Megabytes(4);
                                    void *PushBuffer = PlatformAllocateMemory(PushBufferSize, 0);

                                    b32 IsRunning = true;
                                    s32 WindowWidth = WINDOW_WIDTH;
                                    s32 WindowHeight = WINDOW_HEIGHT;

                                    while(IsRunning)
                                    {
                                        if(XPending(Display))
                                        {
                                            XEvent Event;
                                            XNextEvent(Display, &Event);

                                            if(Event.type == ConfigureNotify)
                                            {
                                                WindowWidth = Event.xconfigure.width;
                                                WindowHeight = Event.xconfigure.height;
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

                                        app_render_commands RenderCommands_ =
                                        {
                                            .RenderWidth = WindowWidth,
                                            .RenderHeight = WindowHeight,
                                            .WindowWidth = WindowWidth,
                                            .WindowHeight = WindowHeight,
                                            .MaxPushBufferSize = PushBufferSize,
                                            .PushBufferSize = 0,
                                            .PushBufferBase = PushBuffer,
                                            .PushBufferElementCount = 0,
                                            .SortEntryAt = PushBufferSize,
                                        };
                                        app_render_commands *RenderCommands = &RenderCommands_;

                                        AppUpdateAndRender(RenderCommands);

                                        SortEntries(RenderCommands);

                                        OpenGLRenderFrame(RenderCommands);

                                        glXSwapBuffers(Display, Window);
                                    }

                                    OpenGLDestroy();

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
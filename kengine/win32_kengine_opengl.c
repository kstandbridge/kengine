internal void
Win32LogError(const char *Message)
{
#define CONSOLE_OUT 1
#if CONSOLE_OUT
    PlatformConsoleOut("%s\n", Message);
#else
    OutputDebugStringA(Message);
    OutputDebugStringA("!\n");
#endif

    DWORD Error = GetLastError();

    LPWSTR ErrorString;
    if(FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, 0, Error,
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), (LPWSTR)&ErrorString, 0, 0))
    {
#if CONSOLE_OUT
        PlatformConsoleOut("%ls", ErrorString);
#else
        OutputDebugStringW(ErrorString);
#endif
        LocalFree(ErrorString);
    }
}


typedef struct app_memory
{
    struct app_state *AppState;

    u32 ArgCount;
    char **Args;

} app_memory;
global app_memory GlobalAppMemory;

internal HGLRC
Win32CreateOldOpenGLContext(HDC DeviceContext)
{
    HGLRC Result = 0;

    PIXELFORMATDESCRIPTOR PixelFormatDescriptor =
    {
        .nSize = sizeof(PixelFormatDescriptor),
        .nVersion = 1,
        .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER |
                   (USE_DEPTH_BUFFER ? 0 : PFD_DEPTH_DONTCARE),
        .iPixelType = PFD_TYPE_RGBA,
        .cColorBits = 24,
        .cDepthBits = (USE_DEPTH_BUFFER ? 24 : 0),
        .cStencilBits = (USE_STENCIL_BUFFER ? 8 : 0)
    };

    s32 PixelFormat = ChoosePixelFormat(DeviceContext, &PixelFormatDescriptor);
    if(PixelFormat)
    {
        if(DescribePixelFormat(DeviceContext, PixelFormat, sizeof(PixelFormatDescriptor), &PixelFormatDescriptor))
        {
            if(SetPixelFormat(DeviceContext, PixelFormat, &PixelFormatDescriptor))
            {
                Result = wglCreateContext(DeviceContext);
                if(Result)
                {
                    if(!wglMakeCurrent(DeviceContext, Result))
                    {
                        Win32LogError("wglMakeCurrent failed");
                        wglDeleteContext(Result);
                    }
                }
                else
                {
                    Win32LogError("wglCreateContext failed");
                }
            }
            else
            {
                Win32LogError("SetPixelFormat failed");
            }
        }
        else
        {
            Win32LogError("DescribePixelFormat failed");
        }
    }
    else
    {
        Win32LogError("ChoosePixelFormat failed");
    }

    return Result;
}

internal HGLRC
Win32OpenGLCreateContext(HDC DeviceContext)
{
    HGLRC Result = 0;

    wgl_choose_pixel_format_arb *wglChoosePixelFormatARB = 0;
    wgl_create_context_attribs_arb *wglCreateContextAttribsARB = 0;
    wgl_swap_interval_ext *wglSwapIntervalEXT = 0;
    s32 wgl_ARB_multisample = 0;
    s32 wgl_ARB_framebuffer_sRGB = 0;
    s32 wgl_EXT_swap_control_tear = 0;

    HWND DummyWindow = CreateWindowExW(0, L"STATIC", L"Dummy Window", WS_OVERLAPPED, CW_USEDEFAULT, CW_USEDEFAULT,
                                      CW_USEDEFAULT, CW_USEDEFAULT, 0, 0, 0, 0);
    if(DummyWindow)
    {
        HDC DeviceContext = GetDC(DummyWindow);
        if(DeviceContext)
        {
            HGLRC GLRenderContext = Win32CreateOldOpenGLContext(DeviceContext);
            if(GLRenderContext)
            {
                
                // NOTE(kstandbridge): Parse extensions strings
                wgl_get_extensions_string_arb *wglGetExtensionsStringARB =
                    (void*)wglGetProcAddress("wglGetExtensionsStringARB");
                if (wglGetExtensionsStringARB != (void*)0 &&
                    wglGetExtensionsStringARB != (void*)1 &&
                    wglGetExtensionsStringARB != (void*)2 &&
                    wglGetExtensionsStringARB != (void*)3 &&
                    wglGetExtensionsStringARB != (void*)-1)
                
                {
                    const char *GlxExtensions = wglGetExtensionsStringARB(DeviceContext);
                    
                    const char *Start = GlxExtensions;
                    const char *At = GlxExtensions;
                    while(*At)
                    {
                        if(IsWhitespace(*At++))
                        {
                            umm Length = At - Start - 1;
                            string Actual = String_(Length, (u8 *)Start);
                            Start = At;
                            
                            if(StringsAreEqual(String("WGL_ARB_pixel_format"), Actual))
                            {
                                wglChoosePixelFormatARB = (void*)wglGetProcAddress("wglChoosePixelFormatARB");
                            }
                            else if(StringsAreEqual(String("WGL_ARB_create_context"), Actual))
                            {
                                wglCreateContextAttribsARB = (void*)wglGetProcAddress("wglCreateContextAttribsARB");
                            }
                            else if(StringsAreEqual(String("WGL_EXT_swap_control"), Actual))
                            {
                                wglSwapIntervalEXT = (void*)wglGetProcAddress("wglSwapIntervalEXT");
                            }
                            else if(StringsAreEqual(String("WGL_ARB_framebuffer_sRGB"), Actual))
                            {
                                wgl_ARB_framebuffer_sRGB = 1;
                            }
                            else if(StringsAreEqual(String("WGL_ARB_multisample"), Actual))
                            {
                                wgl_ARB_multisample = 1;
                            }
                            else if(StringsAreEqual(String("WGL_EXT_swap_control_tear"), Actual))
                            {
                                wgl_EXT_swap_control_tear = true;
                            }
                        }
                    }

                    wglMakeCurrent(0, 0);
                    wglDeleteContext(GLRenderContext);
                }
            }
            ReleaseDC(DummyWindow, DeviceContext);
        }
        DestroyWindow(DummyWindow);
    }
    else
    {
        Win32LogError("CreateWindowExW failed");
    }

    if(wglCreateContextAttribsARB && wglChoosePixelFormatARB)
    {
         s32 AttributeList[32];
        // NOTE(kstandbridge): Set attribute list
        {
            s32 *At = AttributeList;

            *At++ = WGL_DRAW_TO_WINDOW_ARB; *At++ = GL_TRUE;
            *At++ = WGL_ACCELERATION_ARB;   *At++ = WGL_FULL_ACCELERATION_ARB;
            *At++ = WGL_SUPPORT_OPENGL_ARB; *At++ = GL_TRUE;
            *At++ = WGL_DOUBLE_BUFFER_ARB;  *At++ = GL_TRUE;
            *At++ = WGL_PIXEL_TYPE_ARB;     *At++ = WGL_TYPE_RGBA_ARB;
            *At++ = WGL_COLOR_BITS_ARB;     *At++ = 24;

            if (USE_DEPTH_BUFFER)
            {
                *At++ = WGL_DEPTH_BITS_ARB;
                *At++ = 24;
            }
            if (USE_STENCIL_BUFFER)
            {
                *At++ = WGL_STENCIL_BITS_ARB;
                *At++ = 8;
            }
            if (wgl_ARB_framebuffer_sRGB)
            {
                *At++ = WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB;
                *At++ = GL_TRUE;
            }
            if (wgl_ARB_multisample)
            {
                *At++ = WGL_SAMPLE_BUFFERS_ARB;
                *At++ = 1;
                *At++ = WGL_SAMPLES_ARB;
                *At++ = MULTI_SAMPLING;
            }
            *At = 0;
        }

        s32 Formats;
        UINT NumFormats;
        if (wglChoosePixelFormatARB(DeviceContext, AttributeList, 0, 1, &Formats, &NumFormats) && 
            (NumFormats != 0))
        {

            PIXELFORMATDESCRIPTOR PixelFormatDescriptor =
            {
                .nSize = sizeof(PixelFormatDescriptor)
            };

            if(DescribePixelFormat(DeviceContext, Formats, sizeof(PixelFormatDescriptor), &PixelFormatDescriptor))
            {
                if(SetPixelFormat(DeviceContext, Formats, &PixelFormatDescriptor))
                {
                    s32 ContextAttributeList[] =
                    {
                        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
                        WGL_CONTEXT_MINOR_VERSION_ARB, 0,
#if KENGINE_INTERNAL
                        WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
#endif
                        WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
                        0,
                    };

                    Result = wglCreateContextAttribsARB(DeviceContext, 0, ContextAttributeList);
                    if(Result)
                    {
                        if(wglMakeCurrent(DeviceContext, Result))
                        {
                            if(wgl_ARB_multisample)
                            {
                                AssertGL(glEnable(GL_MULTISAMPLE_ARB));
                            }
                        }
                        else
                        {
                            Win32LogError("wglMakeCurrent failed");
                            wglDeleteContext(Result);
                            Result = 0;
                        }
                    }
                    else
                    {
                        Win32LogError("wglCreateContextAttribsARB failed");
                    }
                }
                else
                {
                    Win32LogError("SetPixelFormat failed");
                }

            }
            else
            {
                Win32LogError("DescribePixelFormat failed");
            }
        }
        else
        {
            Win32LogError("wglChoosePixelFormatARB failed");
        }
        
    }

    if(!Result)
    {
        PlatformConsoleOut("Warning: Failed to create modern OpenGL context, trying legacy context!\n");
        Result = Win32CreateOldOpenGLContext(DeviceContext);
    }

    if(Result)
    {
        if(wglSwapIntervalEXT)
        {
            if(!wglSwapIntervalEXT(wgl_EXT_swap_control_tear ? -1 : 1))
            {
                Win32LogError("wglSwapIntervalEXT failed");
            }
        }

#if KENGINE_INTERNAL
        SetupOpenGLDebugCallback();
#endif

    }

    return Result;
}

internal LRESULT CALLBACK
Win32WindowProc(HWND Window, UINT Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CREATE:
        {   
            if((GlobalWin32State.DeviceContext = GetDC(Window)))
            {
                if((GlobalWin32State.GLRenderContext = Win32OpenGLCreateContext(GlobalWin32State.DeviceContext)))
                {
                    OpenGLRenderInit();
                }
                else
                {
                    ReleaseDC(Window, GlobalWin32State.DeviceContext);
                    Result = -1;
                }
            }
            else
            {
                Win32LogError("GetDC failed");
                Result = -1;
            }
        } break;

        case WM_DESTROY:
        {
            if(GlobalWin32State.GLRenderContext)
            {
                OpenGLDestroy();

                wglMakeCurrent(0, 0);
                wglDeleteContext(GlobalWin32State.GLRenderContext);
            }

            if(GlobalWin32State.DeviceContext)
            {
                ReleaseDC(Window, GlobalWin32State.DeviceContext);
            }

            PostQuitMessage(0);

        } break;

        case WM_SIZE:
        {
            OpenGLRenderResize(LOWORD(LParam), HIWORD(LParam));
        } break;

        default:
        {
            Result = DefWindowProcW(Window, Message, WParam, LParam);
        } break;
    }


    return Result;
}

s32 WINAPI
WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPTSTR CmdLine, s32 CmdShow)
{
    s32 Result = 0;

    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;


    WNDCLASSEXW WindowClass =
    {
        .cbSize = sizeof(WindowClass),
        .lpfnWndProc = Win32WindowProc,
        .hInstance = Instance,
        .hIcon = LoadIconA(0, IDI_APPLICATION),
        .hCursor = LoadCursorA(0, IDC_ARROW),
        .lpszClassName = L"kengine_window_class"
    };

    if(RegisterClassExW(&WindowClass))
    {
        DWORD ExStyle = WS_EX_APPWINDOW;
        DWORD Style = WS_OVERLAPPEDWINDOW;

        s32 WindowWidth = WINDOW_WIDTH;
        s32 WindowHeight = WINDOW_HEIGHT;

        Style &= ~WS_THICKFRAME & ~WS_MAXIMIZEBOX;
        
        RECT Rect = 
        {
            .top = 0,
            .left = 0,
            .right = WINDOW_WIDTH,
            .bottom = WINDOW_HEIGHT
        };
        if(AdjustWindowRectEx(&Rect, Style, FALSE, ExStyle))
        {
            WindowWidth = Rect.right - Rect.left;
            WindowHeight = Rect.bottom - Rect.top;
        }
        else
        {
            Win32LogError("AdjustWindowRectEx failed");
            Style = WS_OVERLAPPEDWINDOW;
        }
        

        HWND Window = CreateWindowExW(ExStyle, WindowClass.lpszClassName, L"kengine opengl", Style | WS_VISIBLE, 
                                      CW_USEDEFAULT, CW_USEDEFAULT, WindowWidth, WindowHeight, 0, 0, 
                                      WindowClass.hInstance, 0);
        if(Window)
        {
            for(;;)
            {
                MSG Message;
                if(PeekMessageW(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        break;
                    }
                    TranslateMessage(&Message);
                    DispatchMessageW(&Message);
                    
                    continue;
                }

                OpenGLRenderFrame();

                if(!SwapBuffers(GlobalWin32State.DeviceContext))
                {
                    Win32LogError("SwapBuffers failed");
                }
            }

            UnregisterClassW(WindowClass.lpszClassName, WindowClass.hInstance);
        }
        else
        {
            Win32LogError("CreateWindowExW failed");
        }
    }
    else
    {
        Win32LogError("RegisterClassExW failed!");
    }

    return Result;
}
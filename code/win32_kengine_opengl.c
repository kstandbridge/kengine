internal void
Win32SetOpenGLPixelFormat(HDC WindowDC)
{
    s32 SuggestedPixelFormatIndex = 0;
    GLuint ExtendedPick = 0;
    if(wglChoosePixelFormatARB)
    {
        int IntAttribList[] =
        {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB, GL_TRUE,
            0,
        };
        
        if(!OpenGLDefaultInternalTextureFormat)
        {
            IntAttribList[10] = 0;
        }
        
        wglChoosePixelFormatARB(WindowDC, IntAttribList, 0, 1, 
                                &SuggestedPixelFormatIndex, &ExtendedPick);
    }
    
    if(!ExtendedPick)
    {
        PIXELFORMATDESCRIPTOR DesiredPixelFormat;
        DesiredPixelFormat.nSize = sizeof(DesiredPixelFormat);
        DesiredPixelFormat.nVersion = 1;
        DesiredPixelFormat.iPixelType = PFD_TYPE_RGBA;
        DesiredPixelFormat.dwFlags = PFD_SUPPORT_OPENGL|PFD_DRAW_TO_WINDOW|PFD_DOUBLEBUFFER;
        DesiredPixelFormat.cColorBits = 32;
        DesiredPixelFormat.cAlphaBits = 8;
        DesiredPixelFormat.iLayerType = PFD_MAIN_PLANE;
        
        SuggestedPixelFormatIndex = Win32ChoosePixelFormat(WindowDC, &DesiredPixelFormat);
    }
    
    PIXELFORMATDESCRIPTOR SuggestedPixelFormat;
    Win32DescribePixelFormat(WindowDC, SuggestedPixelFormatIndex,
                             sizeof(SuggestedPixelFormat), &SuggestedPixelFormat);
    Win32SetPixelFormat(WindowDC, SuggestedPixelFormatIndex, &SuggestedPixelFormat);
}

internal void
Win32LoadWGLExtensions()
{
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.lpfnWndProc = Win32DefWindowProcA;
    WindowClass.hInstance = Win32GetModuleHandleA(0);
    WindowClass.lpszClassName = "kEngineWGLLoader";
    
    if(Win32RegisterClassExA(&WindowClass))
    {
        HWND Window = Win32CreateWindowExA(
                                           0,
                                           WindowClass.lpszClassName,
                                           "kEngine",
                                           0,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           CW_USEDEFAULT,
                                           0,
                                           0,
                                           WindowClass.hInstance,
                                           0);
        
        HDC WindowDC = Win32GetDC(Window);
        Win32SetOpenGLPixelFormat(WindowDC);
        HGLRC OpenGLRC = Win32wglCreateContext(WindowDC);
        if(Win32wglMakeCurrent(WindowDC, OpenGLRC))        
        {
            wglChoosePixelFormatARB = 
            (wgl_choose_pixel_format_arb *)Win32wglGetProcAddress("wglChoosePixelFormatARB");
            wglCreateContextAttribsARB =
            (wgl_create_context_attribs_arb *)Win32wglGetProcAddress("wglCreateContextAttribsARB");
            wglSwapIntervalEXT = (wgl_swap_interval_ext *)Win32wglGetProcAddress("wglSwapIntervalEXT");
            wglGetExtensionsStringEXT = (wgl_get_extensions_string_ext *)Win32wglGetProcAddress("wglGetExtensionsStringEXT");
            
            if(wglGetExtensionsStringEXT)
            {
                char *Extensions = (char *)wglGetExtensionsStringEXT();
                char *At = Extensions;
                while(*At)
                {
                    while(IsWhitespace(*At)) {++At;}
                    char *End = At;
                    while(*End && !IsWhitespace(*End)) {++End;}
                    
                    umm Count = End - At;        
                    
                    if(0) {}
                    else if(StringsAreEqual(String_(Count, (u8 *)At), String("WGL_EXT_framebuffer_sRGB"))) {OpenGLSupportsSRGBFramebuffer = true;}
                    
                    At = End;
                }
            }
            
            Win32wglMakeCurrent(0, 0);
        }
        
        Win32wglDeleteContext(OpenGLRC);
        Win32ReleaseDC(Window, WindowDC);
        Win32DestroyWindow(Window);
    }
}

s32 Win32OpenGLAttribs[] =
{
    WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
    WGL_CONTEXT_MINOR_VERSION_ARB, 0,
    WGL_CONTEXT_FLAGS_ARB, 0 // NOTE(kstandbridge): Enable for testing WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
#if KENGINE_INTERNAL
    |WGL_CONTEXT_DEBUG_BIT_ARB
#endif
    ,
    WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_COMPATIBILITY_PROFILE_BIT_ARB,
    0,
};

internal opengl_info
Win32OpenGLGetInfo(b32 ModernContext)
{
    opengl_info Result;
    
    Result.ModernContext = ModernContext;
    Result.Vendor = (char *)Win32glGetString(GL_VENDOR);
    Result.Renderer = (char *)Win32glGetString(GL_RENDERER);
    Result.Version = (char *)Win32glGetString(GL_VERSION);
    if(Result.ModernContext)
    {
        Result.ShadingLanguageVersion = (char *)Win32glGetString(GL_SHADING_LANGUAGE_VERSION);
    }
    else
    {
        Result.ShadingLanguageVersion = "(none)";
    }
    
    Result.Extensions = (char *)Win32glGetString(GL_EXTENSIONS);
    
    char *At = Result.Extensions;
    while(*At)
    {
        while(IsWhitespace(*At)) {++At;}
        char *End = At;
        while(*End && !IsWhitespace(*End)) {++End;}
        
        umm Count = End - At;        
        
        if(0) {}
        else if(StringsAreEqual(String_(Count, (u8 *)At), String("GL_EXT_texture_sRGB"))) {Result.GL_EXT_texture_sRGB=true;}
        else if(StringsAreEqual(String_(Count, (u8 *)At), String("GL_EXT_framebuffer_sRGB"))) {Result.GL_EXT_framebuffer_sRGB=true;}
        
        At = End;
    }
    
    return(Result);
}

internal HGLRC
Win32InitOpenGL(HDC WindowDC)
{
    Win32LoadWGLExtensions();
    
    b32 ModernContext = true;
    HGLRC OpenGLRC = 0;
    if(wglCreateContextAttribsARB)
    {
        Win32SetOpenGLPixelFormat(WindowDC);
        OpenGLRC = wglCreateContextAttribsARB(WindowDC, 0, Win32OpenGLAttribs);
    }
    
    if(!OpenGLRC)
    {
        ModernContext = false;
        OpenGLRC = Win32wglCreateContext(WindowDC);
    }
    
    if(Win32wglMakeCurrent(WindowDC, OpenGLRC))
    {
        opengl_info Info = Win32OpenGLGetInfo(ModernContext);
        
        OpenGLDefaultInternalTextureFormat = GL_RGBA8;
        if(Info.GL_EXT_texture_sRGB)
        {
            OpenGLDefaultInternalTextureFormat = GL_SRGB8_ALPHA8;
        }
        
        // TODO(kstandbridge): Need to go back and use extended version of choose pixel format
        // to ensure that our framebuffer is marked as SRGB?
        if(Info.GL_EXT_framebuffer_sRGB)
        {
            Win32glEnable(GL_FRAMEBUFFER_SRGB);
        }
        
        if(wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(1);
        }
        HardwareRendering = true;
    }
    
    return OpenGLRC;
}

inline void
Win32OpenGLSetScreenspace(s32 Width, s32 Height)
{
    Win32glMatrixMode(GL_MODELVIEW);
    Win32glLoadIdentity();
    
    Win32glMatrixMode(GL_PROJECTION);
    f32 a = SafeRatio1(2.0f, (f32)Width);
    f32 b = SafeRatio1(2.0f, (f32)Height);
    f32 Proj[] =
    {
        a,  0,  0,  0,
        0,  b,  0,  0,
        0,  0,  1,  0,
        -1, -1,  0,  1,
    };
    Win32glLoadMatrixf(Proj);
}

inline void
Win32OpenGLRectangle(v2 MinP, v2 MaxP, v4 Color)
{                    
    Win32glBegin(GL_TRIANGLES);
    
    Win32glColor4f(Color.R, Color.G, Color.B, Color.A);
    
    // NOTE(kstandbridge): Lower triangle
    Win32glTexCoord2f(0.0f, 0.0f);
    Win32glVertex2f(MinP.X, MinP.Y);
    
    Win32glTexCoord2f(1.0f, 0.0f);
    Win32glVertex2f(MaxP.X, MinP.Y);
    
    Win32glTexCoord2f(1.0f, 1.0f);
    Win32glVertex2f(MaxP.X, MaxP.Y);
    
    // NOTE(kstandbridge): Upper triangle
    Win32glTexCoord2f(0.0f, 0.0f);
    Win32glVertex2f(MinP.X, MinP.Y);
    
    Win32glTexCoord2f(1.0f, 1.0f);
    Win32glVertex2f(MaxP.X, MaxP.Y);
    
    Win32glTexCoord2f(0.0f, 1.0f);
    Win32glVertex2f(MinP.X, MaxP.Y);
    
    Win32glEnd();
}

inline GLuint 
Win32OpenGLAllocateTexture(u32 Width, u32 Height, void *Data)
{
    GLuint Result;
    
    Win32glGenTextures(1, &Result);
    Win32glBindTexture(GL_TEXTURE_2D, Result);
    Win32glTexImage2D(GL_TEXTURE_2D, 0, 
                      OpenGLDefaultInternalTextureFormat, 
                      Width, Height, 0,
                      GL_BGRA_EXT, GL_UNSIGNED_BYTE, Data);
    
    Win32glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    Win32glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);    
    Win32glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    Win32glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);    
    Win32glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    
    Win32glBindTexture(GL_TEXTURE_2D, 0);
    
    return Result;
}

// TODO(kstandbridge): We need to actually deallocate textures
inline void
Win32OpenGLDeallocateTexture(GLuint Handle)
{
    Win32glDeleteTextures(1, &Handle);
}


internal void
Win32OpenGLRenderCommands(render_commands *Commands)
{    
    Win32glViewport(0, 0, Commands->Width, Commands->Height);
    
    Win32glEnable(GL_TEXTURE_2D);
    Win32glEnable(GL_SCISSOR_TEST);
    Win32glEnable(GL_BLEND);
    Win32glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    
    Win32glMatrixMode(GL_TEXTURE);
    Win32glLoadIdentity();
    
    Win32OpenGLSetScreenspace(Commands->Width, Commands->Height);
    
    u32 ClipRectIndex = 0XFFFFFFFF;
    sort_entry *SortEntries = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
    sort_entry *SortEntry = SortEntries;
    
    for(u32 SortEntryIndex = 0;
        SortEntryIndex < Commands->PushBufferElementCount;
        ++SortEntryIndex, ++SortEntry)
    {
        render_group_command_header *Header = (render_group_command_header *)(Commands->PushBufferBase + SortEntry->Index);
        if(ClipRectIndex != Header->ClipRectIndex)
        {
            ClipRectIndex = Header->ClipRectIndex;
            Assert(ClipRectIndex < Commands->ClipRectCount);
            
            render_command_cliprect *Clip = Commands->ClipRects + ClipRectIndex;
            Win32glScissor(Clip->Bounds.MinX, Clip->Bounds.MinY, 
                           Clip->Bounds.MaxX - Clip->Bounds.MinX, 
                           Clip->Bounds.MaxY - Clip->Bounds.MinY);
        }
        
        void *Data = (u8 *)Header + sizeof(*Header);
        switch(Header->Type)
        {
            case RenderGroupCommand_Clear:
            {
                render_group_command_clear *Command = (render_group_command_clear *)Data;
                
                Win32glClearColor(Command->Color.R, Command->Color.G, Command->Color.B, Command->Color.A);
                Win32glClear(GL_COLOR_BUFFER_BIT);
            } break;
            
            case RenderGroupCommand_Bitmap:
            {
                render_group_command_bitmap *Command = (render_group_command_bitmap *)Data;
                // TODO(kstandbridge): We could instead only draw the bitmap if we have the data, assume its loaded in a background thread
                Assert(Command->Bitmap);
                
#if 0                
                // TODO(kstandbridge): Add back rotation when we have Cos/Sin
                v2 XAxis = V2(Cos(10.0f*Command->Angle), Sin(10.0f*Command->Angle));
                v2 YAxis = Perp(XAxis);
#else
                v2 XAxis = V2(1, 0);
                v2 YAxis = V2(0, 1);
#endif
                
                v2 ScaledXAxis = V2Multiply(V2Set1(Command->Dim.X), XAxis);
                v2 ScaledYAxis = V2Multiply(V2Set1(Command->Dim.Y), YAxis);
                v2 MinP = Command->P;
                v2 MaxP = V2Add(MinP, V2Add(ScaledXAxis, ScaledYAxis));
                
                if(Command->Bitmap->TextureHandle == 0)
                {
                    Command->Bitmap->TextureHandle = (void *)Win32OpenGLAllocateTexture(Command->Bitmap->Width, Command->Bitmap->Height, Command->Bitmap->Memory);
                }
                
                
                Win32glBindTexture(GL_TEXTURE_2D, (GLuint)Command->Bitmap->TextureHandle);
                Win32OpenGLRectangle(MinP, MaxP, Command->Color);
                
            } break;
            case RenderGroupCommand_Rectangle:
            {
                Win32glDisable(GL_TEXTURE_2D);
                render_group_command_rectangle *Command = (render_group_command_rectangle *)Data;
                Win32OpenGLRectangle(Command->Bounds.Min, Command->Bounds.Max, Command->Color);
                Win32glEnable(GL_TEXTURE_2D);
            } break;
            
#if 0            
            case RenderGroupEntry_render_entry_text:
            {
                render_entry_text *Command = (render_entry_text *)Data;
                v2 P = Command->P;
                v4 Color = Command->Color;
                string Text = Command->Text;
                
                glDisable(GL_TEXTURE_2D);
                
                glColor4f(Color.R, Color.G, Color.B, Color.A);
                glRasterPos2f(P.X, P.Y); 
                glListBase(1000);
                glCallLists((GLsizei)Text.Size, GL_UNSIGNED_BYTE, Text.Data);
                
                glEnable(GL_TEXTURE_2D);
                
            } break;
#endif
            
            
            InvalidDefaultCase;
        }
    }
}
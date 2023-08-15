typedef struct linux_offscreen_buffer
{
    // NOTE(kstandbridge): Pixels are alwasy 32-bits wide, Memory Order BB GG RR XX
    Pixmap Pixmap;
    XImage *Image;
    s32 Width;
    s32 Height;
    s32 Pitch;
} linux_offscreen_buffer;

typedef struct linux_window_dimension
{
    s32 Width;
    s32 Height;
} linux_window_dimension;

global b32 GlobalRunning;
global memory_arena GlobalArena;
global linux_offscreen_buffer GlobalBackbuffer;

linux_window_dimension
LinuxGetWindowDimensions(Display *Display, Window Window)
{
    XWindowAttributes Attributes;
    XGetWindowAttributes(Display, Window, &Attributes);

    linux_window_dimension Result =
    {
        .Width = Attributes.width,
        .Height = Attributes.height
    };

    return Result;
}

internal void
RenderWeirdGradient(linux_offscreen_buffer Buffer, s32 BlueOffset, s32 GreenOffset)
{
    u8 *Row = (u8 *)Buffer.Image->data;    
    for(s32 Y = 0;
        Y < Buffer.Height;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(s32 X = 0;
            X < Buffer.Width;
            ++X)
        {
            u8 Blue = (X + BlueOffset);
            u8 Green = (Y + GreenOffset);

            *Pixel++ = ((Green << 8) | Blue);
        }
        
        Row += Buffer.Pitch;
    }
}

internal void
LinuxResizeDIBSection(linux_offscreen_buffer *Buffer, Display *Display, Window Window, s32 Width, s32 Height)
{
    if(Buffer->Image)
    {
        // TODO(kstandbridge): Free memory of GlobalImage->data
        //XDestroyImage(GlobalImage);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    Buffer->Image = XCreateImage(Display, DefaultVisual(Display, 0),
                                24, ZPixmap, 0, 0, Buffer->Width, Buffer->Height, 32, 0);
    Buffer->Image->data = PushSize_(&GlobalArena, Buffer->Image->bytes_per_line * Buffer->Image->height, DefaultArenaPushParams());

    if(Buffer->Pixmap)
    {
        XFreePixmap(Display, Buffer->Pixmap);
    }
    Buffer->Pixmap = XCreatePixmap(Display, Window, Buffer->Width, Buffer->Height, 24);
    int BytesPerPixel = 4;
    Buffer->Pitch = Width*BytesPerPixel;
}

internal void
LinuxDisplayBufferInWindow(Display *Display, Window Window, GC GraphicsContext,
                           s32 WindowWidth, s32 WindowHeight,
                           linux_offscreen_buffer Buffer)
{
    if(Buffer.Pixmap && Buffer.Image)
    {
        XPutImage(Display, Buffer.Pixmap, GraphicsContext, Buffer.Image, 0, 0, 0, 0, Buffer.Image->width, Buffer.Image->height);
        
        XCopyArea(Display, Buffer.Pixmap, Window, GraphicsContext, 0, 0, Buffer.Image->width, Buffer.Image->height, 0, 0);
    }

    XFlush(Display);
}

internal void
LinuxProcessPendingMessages(Display *Display, Window Window, Atom WmDeleteWindow, GC GraphicsContext)
{
    while(GlobalRunning && XPending(Display))
    {
        XEvent Event;
        XNextEvent(Display, &Event);
        switch(Event.type)
        {
            case DestroyNotify:
            {
                if ((Display == Event.xdestroywindow.display) &&
                    (Window == Event.xdestroywindow.window))
                {
                    GlobalRunning = false;
                }
            } break;
            
            case ClientMessage:
            {
                if ((Atom)Event.xclient.data.l[0] == WmDeleteWindow)
                {
                    GlobalRunning = false;
                }
            } break;

            case Expose:
            case MapNotify:
            {
                linux_window_dimension Dimension = LinuxGetWindowDimensions(Display, Window);
                LinuxDisplayBufferInWindow(Display, Window, GraphicsContext,
                                           Dimension.Width, Dimension.Height,
                                           GlobalBackbuffer);
            } break;
#if 0
            case MotionNotify:
            {
                f32 X = (f32)Event.xmotion.x;
                f32 Y = (f32)Event.xmotion.y;

                PlatformConsoleOut("Mouse at %f / %f\n", X, Y);
            } break;
#endif

            default:
            break;
        }
    }
}

int
main(int argc, char **argv)
{
    GlobalLinuxState.MemorySentinel.Prev = &GlobalLinuxState.MemorySentinel;
    GlobalLinuxState.MemorySentinel.Next = &GlobalLinuxState.MemorySentinel;

    Display *Display = XOpenDisplay(0);
    s32 Screen = DefaultScreen(Display);
    
    Window Window = XCreateSimpleWindow(Display, RootWindow(Display, Screen),
                                        0, 0, 1280, 720, 1,
                                        BlackPixel(Display, Screen), WhitePixel(Display, Screen));

    XSizeHints *SizeHints = XAllocSizeHints();
    SizeHints->flags |= PMinSize | PMaxSize;
    SizeHints->min_width = SizeHints->max_width = 1280;
    SizeHints->min_height = SizeHints->max_height = 720;
    XSetWMNormalHints(Display, Window, SizeHints);
    XFree(SizeHints);

    GC GraphicsContext = XCreateGC(Display, Window, 0, 0);

    XSelectInput(Display, Window, ExposureMask | StructureNotifyMask |
                 KeyReleaseMask | PointerMotionMask | ButtonPressMask |
                 ButtonReleaseMask);
    XMapWindow(Display, Window);

    Atom WmDeleteWindow = XInternAtom(Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(Display, Window, &WmDeleteWindow, 1);

    LinuxResizeDIBSection(&GlobalBackbuffer, Display, Window, 1280, 720);
    
    s32 XOffset = 0;
    s32 YOffset = 0;

    GlobalRunning = true;
    while(GlobalRunning)
    {
        LinuxProcessPendingMessages(Display, Window, WmDeleteWindow, GraphicsContext);
 
        if((GlobalBackbuffer.Image) && 
           (GlobalBackbuffer.Image->data))
        {
            RenderWeirdGradient(GlobalBackbuffer, XOffset, YOffset);
        }

        linux_window_dimension Dimension = LinuxGetWindowDimensions(Display, Window);
        LinuxDisplayBufferInWindow(Display, Window, GraphicsContext,
                                   Dimension.Width, Dimension.Height,
                                   GlobalBackbuffer);

        ++XOffset;
        YOffset += 2;
    }

    XFreeGC(Display, GraphicsContext);
    XDestroyWindow(Display, Window);
    XFlush(Display);

    return 0;
}
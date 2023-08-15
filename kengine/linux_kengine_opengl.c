global b32 GlobalRunning;
global Pixmap GlobalPixmap;
global XImage *GlobalImage;
global memory_arena GlobalArena;

internal void
LinuxResizePixmap(Display *Display, u32 Width, u32 Height)
{

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
            case ConfigureNotify:
            {
                u32 Width = (Event.xconfigure.width >= 0) ? Event.xconfigure.width : 0;
                u32 Height = (Event.xconfigure.height >= 0) ? Event.xconfigure.height : 0;
                if(!GlobalImage ||
                   ((GlobalImage->width != Width) &&
                    (GlobalImage->height != Height)))
                {
                    PlatformConsoleOut("Resize window %ux%u\n", Width, Height);
                    //LinuxResizePixmap(Display, Width, Height);
                    if(GlobalImage)
                    {
                        // TODO(kstandbridge): Free memory of GlobalImage->data
                        //XDestroyImage(GlobalImage);
                    }
                    GlobalImage = XCreateImage(Display, DefaultVisual(Display, 0),
                                               24, ZPixmap, 0, 0, Width, Height, 32, 0);
                    GlobalImage->data = PushSize_(&GlobalArena, GlobalImage->bytes_per_line * GlobalImage->height, DefaultArenaPushParams());

                    for(s32 Index = 0;
                        Index < (Width*Height*4);
                        Index += 4)
                    {
                        GlobalImage->data[Index + 0] = 0; // NOTE(kstandbridge): // Blue
                        GlobalImage->data[Index + 1] = 0; // NOTE(kstandbridge): // Green
                        GlobalImage->data[Index + 2] = 255; // NOTE(kstandbridge): // Red
                        GlobalImage->data[Index + 3] = 0; // NOTE(kstandbridge): // Dead pixel, always zero
                    }
                    if(GlobalPixmap)
                    {
                        XFreePixmap(Display, GlobalPixmap);
                    }
                    GlobalPixmap = XCreatePixmap(Display, Window, Width, Height, 24);
                }
            } break;
            
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
                XClearWindow(Display, Window);
                
                if(GlobalPixmap && GlobalImage)
                {
                    XPutImage(Display, GlobalPixmap, GraphicsContext, GlobalImage, 0, 0, 0, 0, GlobalImage->width, GlobalImage->height);
                    
                    XCopyArea(Display, GlobalPixmap, Window, GraphicsContext, 0, 0, GlobalImage->width, GlobalImage->height, 0, 0);
                }

                XFlush(Display);
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
                                        0, 0, 1920*0.5f, 1080*0.5f, 1,
                                        BlackPixel(Display, Screen), WhitePixel(Display, Screen));

    XSizeHints *SizeHints = XAllocSizeHints();
    SizeHints->flags |= PMinSize | PMaxSize;
    SizeHints->min_width = SizeHints->max_width = 1920*0.5f;
    SizeHints->min_height = SizeHints->max_height = 1080*0.5f;
    XSetWMNormalHints(Display, Window, SizeHints);
    XFree(SizeHints);

    GC GraphicsContext = XCreateGC(Display, Window, 0, 0);

    XSelectInput(Display, Window, ExposureMask | StructureNotifyMask |
                 KeyReleaseMask | PointerMotionMask | ButtonPressMask |
                 ButtonReleaseMask);
    XMapWindow(Display, Window);

    Atom WmDeleteWindow = XInternAtom(Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(Display, Window, &WmDeleteWindow, 1);
    GlobalRunning = true;
    while(GlobalRunning)
    {
        LinuxProcessPendingMessages(Display, Window, WmDeleteWindow, GraphicsContext);
    }

    XFreeGC(Display, GraphicsContext);
    XDestroyWindow(Display, Window);
    XFlush(Display);

    return 0;
}
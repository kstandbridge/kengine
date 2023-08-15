global b32 GlobalRunning;
global memory_arena GlobalArena;


global Pixmap GlobalPixmap;
global XImage *GlobalImage;
global s32 BitmapWidth;
global s32 BitmapHeight;
global s32 BytesPerPixel = 4;

internal void
RenderWeirdGradient(s32 BlueOffset, s32 GreenOffset)
{
    s32 Pitch = BitmapWidth*BytesPerPixel;
    u8 *Row = (u8 *)GlobalImage->data;    
    for(s32 Y = 0;
        Y < BitmapHeight;
        ++Y)
    {
        u32 *Pixel = (u32 *)Row;
        for(s32 X = 0;
            X < BitmapWidth;
            ++X)
        {
            u8 Blue = (X + BlueOffset);
            u8 Green = (Y + GreenOffset);
            
            *Pixel++ = ((Green << 8) | Blue);
        }

        Row += Pitch;
    }
}

internal void
LinuxResize(Display *Display, Window Window)
{
    if(GlobalImage)
    {
        // TODO(kstandbridge): Free memory of GlobalImage->data
        //XDestroyImage(GlobalImage);
    }
    GlobalImage = XCreateImage(Display, DefaultVisual(Display, 0),
                                24, ZPixmap, 0, 0, BitmapWidth, BitmapHeight, 32, 0);
    GlobalImage->data = PushSize_(&GlobalArena, GlobalImage->bytes_per_line * GlobalImage->height, DefaultArenaPushParams());

    if(GlobalPixmap)
    {
        XFreePixmap(Display, GlobalPixmap);
    }
    GlobalPixmap = XCreatePixmap(Display, Window, BitmapWidth, BitmapHeight, 24);
}

internal void
LinuxUpdateWindow(Display *Display, Window Window, GC GraphicsContext)
{
    if(GlobalPixmap && GlobalImage)
    {
        XPutImage(Display, GlobalPixmap, GraphicsContext, GlobalImage, 0, 0, 0, 0, GlobalImage->width, GlobalImage->height);
        
        XCopyArea(Display, GlobalPixmap, Window, GraphicsContext, 0, 0, GlobalImage->width, GlobalImage->height, 0, 0);
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
            case ConfigureNotify:
            {
                u32 Width = (Event.xconfigure.width >= 0) ? Event.xconfigure.width : 0;
                u32 Height = (Event.xconfigure.height >= 0) ? Event.xconfigure.height : 0;
                if(!GlobalImage ||
                   ((GlobalImage->width != Width) &&
                    (GlobalImage->height != Height)))
                {
                    BitmapWidth = Width;
                    BitmapHeight = Height;
                    LinuxResize(Display, Window);
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
    
    s32 XOffset = 0;
    s32 YOffset = 0;

    GlobalRunning = true;
    while(GlobalRunning)
    {
        LinuxProcessPendingMessages(Display, Window, WmDeleteWindow, GraphicsContext);
 
        if((GlobalImage) && 
           (GlobalImage->data))
        {
            RenderWeirdGradient(XOffset, YOffset);
        }

        LinuxUpdateWindow(Display, Window, GraphicsContext);

        ++XOffset;
        YOffset += 2;
    }

    XFreeGC(Display, GraphicsContext);
    XDestroyWindow(Display, Window);
    XFlush(Display);

    return 0;
}
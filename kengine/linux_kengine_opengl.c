internal void
LinuxProcessPendingMessages(Display *Display, Window Window, Atom WmDeleteWindow)
{
    while(XPending(Display))
    {
        XEvent Event;
        XNextEvent(Display, &Event);
        switch(Event.type)
        {
            case ConfigureNotify:
            {
                u32 Width = (Event.xconfigure.width >= 0) ? Event.xconfigure.width : 0;
                u32 Height = (Event.xconfigure.height >= 0) ? Event.xconfigure.height : 0;
                PlatformConsoleOut("SIZE: %u / %u\n", Width, Height);
            } break;
            
            case DestroyNotify:
            {
                if ((Display == Event.xdestroywindow.display) &&
                    (Window == Event.xdestroywindow.window))
                {
                    PlatformConsoleOut("Destroy\n");
                }
            } break;
            
            case ClientMessage:
            {
                if ((Atom)Event.xclient.data.l[0] == WmDeleteWindow)
                {
                    PlatformConsoleOut("Delete window\n");
                }
            } break;

            case ButtonPress:
            {
                XCloseDisplay(Display);
                break;
            }
            
            case MotionNotify:
            {
                f32 X = (f32)Event.xmotion.x;
                f32 Y = (f32)Event.xmotion.y;

                PlatformConsoleOut("Mouse at %f / %f\n", X, Y);
            } break;

            default:
            break;
        }
    }
}

int
main(int argc, char **argv)
{
    Display *Display = XOpenDisplay(0);
    s32 Black = BlackPixel(Display, DefaultScreen(Display));
    s32 White = WhitePixel(Display, DefaultScreen(Display));
    
    Window Window = XCreateSimpleWindow(Display, DefaultRootWindow(Display),
                                        0, 0, 100, 100, 0, White, Black);

    XSelectInput(Display, Window, ExposureMask | StructureNotifyMask |
                 KeyReleaseMask | PointerMotionMask | ButtonPressMask |
                 ButtonReleaseMask);

    XMapWindow(Display, Window);

    Atom WmDeleteWindow = XInternAtom(Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(Display, Window, &WmDeleteWindow, 1);

    for(;;)
    {
        LinuxProcessPendingMessages(Display, Window, WmDeleteWindow);
    }

    return 0;
}
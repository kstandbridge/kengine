int
main(int argc, char **argv)
{
    Display *Display = XOpenDisplay(0);
    s32 Black = BlackPixel(Display, DefaultScreen(Display));
    s32 White = WhitePixel(Display, DefaultScreen(Display));
    
    Window Window = XCreateSimpleWindow(Display, DefaultRootWindow(Display),
                                        0, 0, 100, 100, 0, White, Black);

    XSelectInput(Display, Window, ButtonPressMask | StructureNotifyMask);

    XMapWindow(Display, Window);

    for(;;)
    {
        XEvent Event;
        XNextEvent(Display, &Event);
        switch(Event.type)
        {
            case ButtonPress:
            {
                XCloseDisplay(Display);
                break;
            }
        }
    }

    return 0;
}
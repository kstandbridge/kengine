#include <X11/Xlib.h>
#include <X11/Xutil.h>

#define VK_W           25
#define VK_A           38
#define VK_S           39
#define VK_D           40
#define VK_Q           24
#define VK_E           26
#define VK_UP          111
#define VK_DOWN        116
#define VK_LEFT        113
#define VK_RIGHT       114
#define VK_ESCAPE      9
#define VK_SPACE       65

typedef struct linux_state
{
    memory_arena Arena;
    b32 Running;
    offscreen_buffer Backbuffer;

    Pixmap Pixmap;
    XImage *Image;
    
} linux_state;
global linux_state *GlobalLinuxState;

internal window_dimension
LinuxGetWindowDimensions(Display *Display, Window Window)
{
    XWindowAttributes Attributes;
    XGetWindowAttributes(Display, Window, &Attributes);

    window_dimension Result =
    {
        .Width = Attributes.width,
        .Height = Attributes.height
    };

    return Result;
}

internal void
LinuxResizeDIBSection(linux_state *State, Display *Display, Window Window, s32 Width, s32 Height)
{
    offscreen_buffer *Buffer = &State->Backbuffer;
    if(State->Image)
    {
        // TODO(kstandbridge): Free memory of GlobalImage->data
        //XDestroyImage(GlobalImage);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    State->Image = XCreateImage(Display, DefaultVisual(Display, 0),
                                24, ZPixmap, 0, 0, Buffer->Width, Buffer->Height, 32, 0);
    State->Image->data = Buffer->Memory = PushSize_(&State->Arena, State->Image->bytes_per_line * State->Image->height, DefaultArenaPushParams());

    if(State->Pixmap)
    {
        XFreePixmap(Display, State->Pixmap);
    }
    State->Pixmap = XCreatePixmap(Display, Window, Buffer->Width, Buffer->Height, 24);
    int BytesPerPixel = 4;
    Buffer->Pitch = Width*BytesPerPixel;
}

internal void
LinuxDisplayBufferInWindow(linux_state *State,
                           Display *Display, Window Window, GC GraphicsContext,
                           s32 WindowWidth, s32 WindowHeight)
{
    offscreen_buffer *Buffer = &State->Backbuffer;

    if(State->Pixmap && State->Image)
    {
        XPutImage(Display, State->Pixmap, GraphicsContext, State->Image, 0, 0, 0, 0, State->Image->width, State->Image->height);
        
        XCopyArea(Display, State->Pixmap, Window, GraphicsContext, 0, 0, Buffer->Width, Buffer->Height, 0, 0);
    }

    XFlush(Display);
}

internal void
LinuxProcessPendingMessages(linux_state *State, Display *Display, Window Window, Atom WmDeleteWindow, GC GraphicsContext)
{
    while(State->Running && XPending(Display))
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
                    State->Running = false;
                }
            } break;
            
            case ClientMessage:
            {
                if ((Atom)Event.xclient.data.l[0] == WmDeleteWindow)
                {
                    State->Running = false;
                }
            } break;

            case Expose:
            case MapNotify:
            {
                window_dimension Dimension = LinuxGetWindowDimensions(Display, Window);
                LinuxDisplayBufferInWindow(State,
                                           Display, Window, GraphicsContext,
                                           Dimension.Width, Dimension.Height);
            } break;

            case KeyRelease:
            case KeyPress:
            {
                if(Event.xkey.keycode == VK_W)
                {
                }
                else if(Event.xkey.keycode == VK_A)
                {
                }
                else if(Event.xkey.keycode == VK_S)
                {
                }
                else if(Event.xkey.keycode == VK_D)
                {
                }
                else if(Event.xkey.keycode == VK_Q)
                {
                }
                else if(Event.xkey.keycode == VK_E)
                {
                }
                else if(Event.xkey.keycode == VK_UP)
                {
                }
                else if(Event.xkey.keycode == VK_LEFT)
                {
                }
                else if(Event.xkey.keycode == VK_DOWN)
                {
                }
                else if(Event.xkey.keycode == VK_RIGHT)
                {
                }
                if(Event.xkey.keycode == VK_ESCAPE)
                {
                    PlatformConsoleOut("ESCAPE: ");
                    if(Event.type == KeyPress)
                    {
                        PlatformConsoleOut("IsDown ");
                    }
                    else
                    {
                        PlatformConsoleOut("WasDown");
                    }
                    PlatformConsoleOut("\n");
                }
                else if(Event.xkey.keycode == VK_SPACE)
                {
                }
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
    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;
    GlobalLinuxState = BootstrapPushStruct(linux_state, Arena);

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

    XSelectInput(Display, Window, StructureNotifyMask | PropertyChangeMask |
                                  PointerMotionMask | ButtonPressMask | ButtonReleaseMask |
                                  KeyPressMask | KeyReleaseMask);
    XMapWindow(Display, Window);

    Atom WmDeleteWindow = XInternAtom(Display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(Display, Window, &WmDeleteWindow, 1);

    LinuxResizeDIBSection(GlobalLinuxState, Display, Window, 1280, 720);
    
    s32 XOffset = 0;
    s32 YOffset = 0;

    GlobalLinuxState->Running = true;
    while(GlobalLinuxState->Running)
    {
        LinuxProcessPendingMessages(GlobalLinuxState, Display, Window, WmDeleteWindow, GraphicsContext);
 
        // TODO(kstandbridge): Process controller input

        if((GlobalLinuxState->Image) && 
           (GlobalLinuxState->Image->data))
        {
            RenderWeirdGradient(&GlobalLinuxState->Backbuffer, XOffset, YOffset);
        }

        window_dimension Dimension = LinuxGetWindowDimensions(Display, Window);
        LinuxDisplayBufferInWindow(GlobalLinuxState,
                                   Display, Window, GraphicsContext,
                                   Dimension.Width, Dimension.Height);

        ++XOffset;
        YOffset += 2;
    }

    XFreeGC(Display, GraphicsContext);
    XDestroyWindow(Display, Window);
    XFlush(Display);

    return 0;
}
#include <math.h>

#include "handmade.h"
#include "handmade.c"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <alsa/asoundlib.h>
#include <linux/joystick.h>

#include "linux_kengine_opengl.h"

global linux_state *GlobalLinuxState;

internal void *
LinuxLoadFunction(void *Library, char *Name)
{
    void *Result = dlsym(Library, Name);

    if(!Result)
    {
        LinuxConsoleOut("Failed to load function %s\n", Name);
        InvalidCodePath;
    }

    return Result;
}

internal void *
LinuxLoadLibrary(char *Name)
{
    void *Result = dlopen(Name, RTLD_NOW | RTLD_LOCAL);
    
    if(!Result)
    {
        LinuxConsoleOut("Failed to load library %s\n", Name);
        InvalidCodePath;
    }

    return Result;
}

internal void
LinuxUnloadLibrary(void *Library)
{
    if(Library != 0)
    {
        dlclose(Library);
        Library = 0;
    }
    else
    {
        LinuxConsoleOut("Failed to unload, invalid handle\n");
        InvalidCodePath;
    }
}

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
LinuxProcessKeyboardMessage(button_state *NewState, b32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
    else
    {
        InvalidCodePath;
    }
}

internal void
LinuxProcessPendingMessages(linux_state *State, Display *Display, Window Window, Atom WmDeleteWindow, GC GraphicsContext,
                            controller_input *KeyboardController)
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
                    LinuxProcessKeyboardMessage(&KeyboardController->MoveUp, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_A)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->MoveLeft, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_S)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->MoveDown, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_D)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->MoveRight, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_Q)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->LeftShoulder, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_E)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->RightShoulder, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_UP)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->ActionUp, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_LEFT)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->ActionLeft, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_DOWN)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->ActionDown, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_RIGHT)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->ActionRight, Event.type == KeyPress);
                }
                if(Event.xkey.keycode == VK_ESCAPE)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->Back, Event.type == KeyPress);
                }
                else if(Event.xkey.keycode == VK_SPACE)
                {
                    LinuxProcessKeyboardMessage(&KeyboardController->Start, Event.type == KeyPress);
                }

                b32 AltKeyWasDown = Event.xkey.state & VK_ALT_MASK;
                if((Event.xkey.keycode == VK_F4) && AltKeyWasDown)
                {
                    GlobalLinuxState->Running = false;
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

global void *GlobalAlsaLibrary;

#define ALSA_OPEN(name) int name(snd_pcm_t **, const char *, snd_pcm_stream_t, int)
typedef ALSA_OPEN(ALSA_snd_pcm_open);

internal ALSA_OPEN(snd_pcm_open_stub)
{
    return -1;
}
global ALSA_snd_pcm_open *snd_pcm_open_ = snd_pcm_open_stub;
#define snd_pcm_open snd_pcm_open_

internal char *(*ALSA_snd_strerror)(int errnum);
internal int (*ALSA_snd_pcm_hw_params_malloc)(snd_pcm_hw_params_t **);
internal int (*ALSA_snd_pcm_hw_params_any)(snd_pcm_t *, snd_pcm_hw_params_t *);
internal int (*ALSA_snd_pcm_hw_params_set_access)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_access_t);
internal int (*ALSA_snd_pcm_hw_params_set_format)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_format_t);
internal int (*ALSA_snd_pcm_hw_params_set_rate_near)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int *, int *);
internal int (*ALSA_snd_pcm_hw_params_set_channels)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int);
internal int (*ALSA_snd_pcm_hw_params_set_periods)(snd_pcm_t *, snd_pcm_hw_params_t *, unsigned int, int);
internal int (*ALSA_snd_pcm_hw_params_set_buffer_size)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t);
internal int (*ALSA_snd_pcm_hw_params_set_buffer_size_near)(snd_pcm_t *, snd_pcm_hw_params_t *, snd_pcm_uframes_t *);
internal int (*ALSA_snd_pcm_hw_params)(snd_pcm_t *, snd_pcm_hw_params_t *);
internal int (*ALSA_snd_pcm_hw_params_free)(snd_pcm_hw_params_t *);
internal int (*ALSA_snd_pcm_drain)(snd_pcm_t *);
internal int (*ALSA_snd_pcm_close)(snd_pcm_t *);
internal snd_pcm_sframes_t (*ALSA_snd_pcm_writei)(snd_pcm_t *, const void *, snd_pcm_uframes_t);
internal int (*ALSA_snd_pcm_prepare)(snd_pcm_t *);

internal void
LinuxLoadAlsaSymbol(char *Name, void **Address)
{
    *Address = LinuxLoadFunction(GlobalAlsaLibrary, Name);
    if (*Address == 0)
    {
        PlatformConsoleOut("Failed to load Alsa symbol %s\n", Address);
        InvalidCodePath;
    }
}
#define LINUX_ALSA_SYMBOL(type) LinuxLoadAlsaSymbol(#type, (void **) (char *) &ALSA_##type)

internal void
LinuxInitSound(s32 SamplesPerSecond, s32 BufferSize, linux_sound_player *SoundPlayer)
{

#define AUDIO_CHANNELS 2

    u32 Periods = 4;     
    u32 PeriodSize = 512;
    
    Assert(GlobalAlsaLibrary == 0);

    GlobalAlsaLibrary = LinuxLoadLibrary("libasound.so");
    if(GlobalAlsaLibrary)
    {
        *(void **)(&snd_pcm_open) = dlsym(GlobalAlsaLibrary, "snd_pcm_open");

        LINUX_ALSA_SYMBOL(snd_strerror);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_malloc);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_any);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_set_access);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_set_format);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_set_rate_near);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_set_channels);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_set_periods);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_set_buffer_size);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_set_buffer_size_near);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params);
        LINUX_ALSA_SYMBOL(snd_pcm_hw_params_free);
        LINUX_ALSA_SYMBOL(snd_pcm_drain);
        LINUX_ALSA_SYMBOL(snd_pcm_close);
        LINUX_ALSA_SYMBOL(snd_pcm_writei);
        LINUX_ALSA_SYMBOL(snd_pcm_prepare);


        SoundPlayer->PcmHandle = 0;
        SoundPlayer->SamplesPerSecond = SamplesPerSecond;
        SoundPlayer->IsPlaying = false;
        SoundPlayer->PeriodSize = PeriodSize;
        SoundPlayer->BytesPerFrame = sizeof(s16) * AUDIO_CHANNELS;

        linux_sound_output *SoundOutput = &SoundPlayer->Output;
        SoundOutput->RunningSampleIndex = 0;
        SoundOutput->LatencySampleCount = 0;
        SoundOutput->BytesPerSample = sizeof(s16) * AUDIO_CHANNELS;
        SoundOutput->Buffer.Size = 0;
        SoundOutput->Buffer.ReadIndex = 0;
        SoundOutput->Buffer.Data = 0;

        snd_pcm_hw_params_t *HwParams = 0;
        s32 Error;
        u32 NrSamples;
        u32 SampleSize;
        snd_pcm_uframes_t RequestedBufferSize;
        snd_pcm_uframes_t BufferSize;
        s32 FileDesc;

        char *Name = "default";

        Error = snd_pcm_open(&SoundPlayer->PcmHandle, Name, SND_PCM_STREAM_PLAYBACK, 0);
        if(Error < 0) 
        {
            char *ErrorValue =  ALSA_snd_strerror(Error);
            PlatformConsoleOut("Init: cannot open audio device %s (%s)\n", Name, ErrorValue); 
            InvalidCodePath; 
        }
        Error = ALSA_snd_pcm_hw_params_malloc(&HwParams);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_any(SoundPlayer->PcmHandle, HwParams);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_access(SoundPlayer->PcmHandle, HwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_format(SoundPlayer->PcmHandle, HwParams, SND_PCM_FORMAT_S16_LE);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_rate_near(SoundPlayer->PcmHandle, HwParams, &SoundPlayer->SamplesPerSecond, 0);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_channels(SoundPlayer->PcmHandle, HwParams, AUDIO_CHANNELS);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_periods(SoundPlayer->PcmHandle, HwParams, Periods, 0);
        if(Error < 0) InvalidCodePath;
        RequestedBufferSize = BufferSize = (PeriodSize * Periods) / (AUDIO_CHANNELS * sizeof(s16));
        Error = ALSA_snd_pcm_hw_params_set_buffer_size_near(SoundPlayer->PcmHandle, HwParams, &BufferSize);
        if(Error < 0) InvalidCodePath;
        if (RequestedBufferSize != BufferSize) 
        {
            LinuxConsoleOut("Alsa adjusted buffer size from %lu to %lu\n", RequestedBufferSize, BufferSize);
        }
        Error = ALSA_snd_pcm_hw_params(SoundPlayer->PcmHandle, HwParams);
        if(Error < 0) InvalidCodePath;

        ALSA_snd_pcm_hw_params_free(HwParams);

        SoundOutput->SamplesPerSecond = SoundPlayer->SamplesPerSecond;
        SoundOutput->RunningSampleIndex = 0;
        SoundOutput->LatencySampleCount = SoundOutput->SamplesPerSecond / 15;
        NrSamples = SoundOutput->SamplesPerSecond * AUDIO_CHANNELS;     // 1 second
        SampleSize = NrSamples * sizeof(s16);
        SampleSize = AlignPow2(SampleSize, 4096);
        SoundOutput->Buffer.Data = 0;
        FileDesc = memfd_create("/circ-audio-buf", 0);
        if (FileDesc >= 0)
        {
            s32 FileStatus = ftruncate(FileDesc, SampleSize);
            if (FileStatus == 0)
            {
                u8 *BufferBase = (u8 *)mmap(0, SampleSize << 1, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
                if (BufferBase != MAP_FAILED)
                {
                    void *Map1 = mmap(BufferBase, SampleSize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, FileDesc, 0);
                    if (Map1 == BufferBase)
                    {
                        void *Map2 = mmap(BufferBase + SampleSize, SampleSize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, FileDesc, 0);
                        if (Map2 == (BufferBase + SampleSize))
                        {
                            
                            SoundOutput->Buffer.Size = SampleSize;
                            SoundOutput->Buffer.Data = BufferBase;
                        }
                    }
                }
            }
            
            FileStatus = close(FileDesc);
        }

        if (SoundOutput->Buffer.Data == 0)
        {
            InvalidCodePath
        }
    }
    else
    {
        InvalidCodePath;
    }
}

internal void *
LinuxAudioThread(void *Context)
{
    void *Result = 0;

    linux_sound_player *Player = (linux_sound_player *)Context;
    linux_sound_output *Output = &Player->Output;
    linux_audio_buffer *Buffer = &Output->Buffer;

    while (Player->IsPlaying)
    {
        u32 CopySize = (u32)Player->PeriodSize;
        if ((CopySize + Buffer->ReadIndex) > Buffer->Size)
        {
            CopySize = Buffer->Size - Buffer->ReadIndex;
        }
        
        u32 FrameCount = CopySize / Player->BytesPerFrame;
        snd_pcm_sframes_t ReturnValue = ALSA_snd_pcm_writei(Player->PcmHandle, Buffer->Data + Buffer->ReadIndex,
                                                            FrameCount);
        
        if (ReturnValue < 0)
        {
            ALSA_snd_pcm_prepare(Player->PcmHandle);
        }
        else
        {
            u32 WrittenBytes = ReturnValue * Player->BytesPerFrame;
            u32 NextReadIndex = Buffer->ReadIndex + WrittenBytes;
            if (NextReadIndex >= Buffer->Size) {
                NextReadIndex -= Buffer->Size;
            }
            
            u32 NextWriteIndex = NextReadIndex + Player->PeriodSize;
            if (NextWriteIndex >= Buffer->Size) {
                NextWriteIndex -= Buffer->Size;
            }
            
            Buffer->WriteIndex = NextWriteIndex;
            CompletePreviousWritesBeforeFutureWrites;
            Buffer->ReadIndex = NextReadIndex;
        }
    }

    return Result;
}

internal void
LinuxStartPlayingSound(linux_sound_player *SoundPlayer)
{
    if(!SoundPlayer->IsPlaying)
    {
        pthread_attr_t ThreadAttr;
        pthread_attr_init(&ThreadAttr);
        pthread_attr_setdetachstate(&ThreadAttr, PTHREAD_CREATE_DETACHED);
        SoundPlayer->IsPlaying = true;

        s32 ErrorNumber = pthread_create(&SoundPlayer->Thread, &ThreadAttr, LinuxAudioThread, SoundPlayer);

        pthread_attr_destroy(&ThreadAttr);

        if(ErrorNumber)
        {
            SoundPlayer->IsPlaying = false;
        }
    }
    else
    {
        LinuxConsoleOut("Error: Sound already playing!\n");
        InvalidCodePath;
    }
}

internal void
LinuxFillSoundBuffer(linux_sound_output *SoundOutput, sound_output_buffer *SourceBuffer)
{
    SoundOutput->RunningSampleIndex += SourceBuffer->SampleCount;
}

internal void
LinuxFreeSound(linux_sound_player *SoundPlayer)
{
    linux_sound_output *SoundOutput = &SoundPlayer->Output;

    if(SoundPlayer->PcmHandle)
    {
        ALSA_snd_pcm_drain(SoundPlayer->PcmHandle);
        ALSA_snd_pcm_close(SoundPlayer->PcmHandle);
        SoundPlayer->PcmHandle = 0;
    }

    munmap(SoundOutput->Buffer.Data, SoundOutput->Buffer.Size << 1);
    LinuxUnloadLibrary(GlobalAlsaLibrary);
}



internal u32
LinuxFindJoysticks(linux_state *State)
{
    // TODO(kstandbridge): Monitor joysticks for being plugged in/out

    struct ff_effect *Effect = &State->Joystick.Effect;
    u8 *JoystickIDs = State->Joystick.IDs;
    s32 *JoystickFileDescriptors = State->Joystick.FileDescriptors;
    s32 *JoystickEventFileDescriptors = State->Joystick.EventFileDescriptors;

    char FileDescription[] = "/dev/input/jsX";
    char PathBuffer[128];
    u32 Joysticks = 0;

    if (Effect->id != -1)
    {
        Effect->type = FF_RUMBLE;
        Effect->u.rumble.strong_magnitude = 60000;
        Effect->u.rumble.weak_magnitude = 0;
        Effect->replay.length = 200;
        Effect->replay.delay = 0;
        Effect->id = -1;         // NOTE(kstandbridge): ID must be set to -1 for every new effect
    }

    for (u8 JoystickIndex = 0;
         JoystickIndex < ArrayCount(State->Joystick.IDs);
         ++JoystickIndex)
    {
        if (JoystickIDs[JoystickIndex] == 0)
        {
            u8 JoystickID = JoystickIDs[Joysticks];
            if (JoystickFileDescriptors[JoystickID] && JoystickEventFileDescriptors[JoystickID])
            {
                ++Joysticks;
                continue;
            }
            else if (!JoystickFileDescriptors[JoystickID])
            {
                FileDescription[13] = (char)(JoystickIndex + 0x30);
                s32 FileDescriptor = open(FileDescription, O_RDONLY | O_NONBLOCK);
                if (FileDescriptor < 0)
                {
                    continue;
                }
                JoystickFileDescriptors[JoystickID] = FileDescriptor;
            }

            if (!JoystickEventFileDescriptors[JoystickID])
            {
                for (u8 EventID = 0;
                     EventID <= 99;
                     EventID++)
                {
                    string Path = FormatStringToBuffer((u8 *)PathBuffer, sizeof(PathBuffer), "/sys/class/input/js%d/device/event%d", JoystickIndex, EventID);
                    if (LinuxDirectoryExists(Path))
                    {
                        Path = FormatStringToBuffer((u8 *)PathBuffer, sizeof(PathBuffer), "/dev/input/event%d", EventID);
                        PathBuffer[Path.Size] = '\0';
                        s32 EventFileDescriptor = open(PathBuffer, O_RDWR);
                        if (EventFileDescriptor >= 0)
                        {
                            JoystickEventFileDescriptors[JoystickIndex] = EventFileDescriptor;
                            if (ioctl(EventFileDescriptor, EVIOCSFF, &Effect) == -1)
                            {
                                PlatformConsoleOut("Error: could not send effect to the driver for %s\n", PathBuffer);
                            }
                            break;
                        }
                    }
                }
            }

            JoystickIDs[Joysticks++] = JoystickIndex;
        }
        else
        {
            ++Joysticks;
        }
    }

    return Joysticks;
}

internal void
LinuxProcessXboxDigitalButtons(u32 ButtonState,
                               button_state *OldState,
                               button_state *NewState)
{
    NewState->EndedDown = ButtonState;
    NewState->HalfTransitionCount += (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
    OldState->EndedDown = NewState->EndedDown;
}

internal f32
LinuxProcessXboxAnalogStick(s16 Value, u16 Deadzone)
{
    f32 Result = 0;
    
    if (Value < -Deadzone)
    {
        Result = ((f32)(Value + Deadzone)) / (32767.0f - (f32)Deadzone);
    }
    else if (Value > Deadzone)
    {
        Result = ((f32)(Value - Deadzone)) / (32767.0f - (f32)Deadzone);
    }

    return Result;
}

internal void 
LinuxJoystickPopulateGameInput(linux_state *State, app_input *NewInput, app_input *OldInput, u32 NumberOfJoysticks)
{
    s32 *JoystickFileDescriptors = State->Joystick.FileDescriptors;

    u32 StartIndex = 1; // NOTE(kstandbridge): Keyboard is index 0, controllers start at 1
    u32 TotalNumberOfJoysticks = StartIndex + NumberOfJoysticks;

    if (NumberOfJoysticks)
    {
        if (TotalNumberOfJoysticks > ArrayCount(NewInput->Controllers))
        {
            TotalNumberOfJoysticks = ArrayCount(NewInput->Controllers);
        }
        for (u8 JoySitckIndex = StartIndex;
            JoySitckIndex < TotalNumberOfJoysticks;
            ++JoySitckIndex)
        {
            u8 JoystickID = JoystickFileDescriptors[JoySitckIndex - StartIndex];
            if (JoystickID == 0)
            {
                continue;
            }

            controller_input *OldController = GetController(OldInput, JoySitckIndex);
            controller_input *NewController = GetController(NewInput, JoySitckIndex);
            NewController->IsConnected = true;
            NewController->IsAnalog = OldController->IsAnalog;
            for (u8 Button = 0; Button < ArrayCount(NewController->Buttons); ++Button)
            {
                NewController->Buttons[Button].HalfTransitionCount = 0;
            }
            NewController->StickAverageX = OldController->StickAverageX;
            NewController->StickAverageY = OldController->StickAverageY;

            struct js_event JoystickEvent;

            while (read(JoystickFileDescriptors[JoySitckIndex - StartIndex], &JoystickEvent, sizeof(JoystickEvent)) > 0)
            {
                if (JoystickEvent.type >= JS_EVENT_INIT)
                {
                    JoystickEvent.type -= JS_EVENT_INIT;
                }

                if (JoystickEvent.type == JS_EVENT_BUTTON)
                {
                    if (JoystickEvent.number == JOYSTICK_BUTTON_A)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->ActionDown,
                                                       &NewController->ActionDown);
                    }
                    else if (JoystickEvent.number == JOYSTICK_BUTTON_B)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->ActionRight,
                                                       &NewController->ActionRight);
                    }
                    else if (JoystickEvent.number == JOYSTICK_BUTTON_X)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->ActionLeft,
                                                       &NewController->ActionLeft);
                    }
                    else if (JoystickEvent.number == JOYSTICK_BUTTON_Y)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->ActionUp,
                                                       &NewController->ActionUp);
                    }
                    else if (JoystickEvent.number == JOYSTICK_BUTTON_LEFT_SHOULDER)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->LeftShoulder,
                                                       &NewController->LeftShoulder);
                    }
                    else if (JoystickEvent.number == JOYSTICK_BUTTON_RIGHT_SHOULDER)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->RightShoulder,
                                                       &NewController->RightShoulder);
                    }
                    else if (JoystickEvent.number == JOYSTICK_BUTTON_BACK)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->Back,
                                                       &NewController->Back);
                    }
                    else if (JoystickEvent.number == JOYSTICK_BUTTON_START)
                    {
                        LinuxProcessXboxDigitalButtons((u32)JoystickEvent.value,
                                                       &OldController->Start,
                                                       &NewController->Start);
                    }
                }
                else if (JoystickEvent.type == JS_EVENT_AXIS)
                {
                    if (JoystickEvent.number == JOYSTICK_AXIS_LEFT_THUMB_X)
                    {
                        NewController->StickAverageX = LinuxProcessXboxAnalogStick(JoystickEvent.value, XBOX_CONTROLLER_DEADZONE);
                        NewController->IsAnalog = true;
                    }
                    else if (JoystickEvent.number == JOYSTICK_AXIS_LEFT_THUMB_Y)
                    {
                        NewController->StickAverageY = -LinuxProcessXboxAnalogStick(JoystickEvent.value, XBOX_CONTROLLER_DEADZONE);
                        NewController->IsAnalog = true;
                    }
                    else if (JoystickEvent.number == JOYSTICK_AXIS_DPAD_X)
                    {
                        if (JoystickEvent.value > XBOX_CONTROLLER_DEADZONE)
                        {
                            NewController->StickAverageX = 1.0f;
                        }
                        else if (JoystickEvent.value < -XBOX_CONTROLLER_DEADZONE)
                        {
                            NewController->StickAverageX = -1.0f;
                        }
                        else
                        {
                            NewController->StickAverageX = 0.0f;
                        }
                        NewController->IsAnalog = false;
                    }
                    else if (JoystickEvent.number == JOYSTICK_AXIS_DPAD_Y)
                    {
                        if (JoystickEvent.value > XBOX_CONTROLLER_DEADZONE)
                        {
                            NewController->StickAverageY = -1.0f;
                        }
                        else if (JoystickEvent.value < -XBOX_CONTROLLER_DEADZONE)
                        {
                            NewController->StickAverageY = 1.0f;
                        }
                        else
                        {
                            NewController->StickAverageY = 0.0f;
                        }
                        NewController->IsAnalog = false;
                    }

                    f32 AnalogButtonThreshold = 0.5f;
                    LinuxProcessXboxDigitalButtons((NewController->StickAverageY < -AnalogButtonThreshold) ? 1 : 0,
                                                   &OldController->MoveDown, &NewController->MoveDown);
                    LinuxProcessXboxDigitalButtons((NewController->StickAverageY > AnalogButtonThreshold) ? 1 : 0,
                                                   &OldController->MoveUp, &NewController->MoveUp);
                    LinuxProcessXboxDigitalButtons((NewController->StickAverageX < -AnalogButtonThreshold) ? 1 : 0,
                                                   &OldController->MoveLeft, &NewController->MoveLeft);
                    LinuxProcessXboxDigitalButtons((NewController->StickAverageX > AnalogButtonThreshold) ? 1 : 0,
                                                   &OldController->MoveRight, &NewController->MoveRight);
                }
            }
        }
    }
}

inline f32
LinuxGetSecondsElapsed(u64 Start, u64 End, u64 Frequency)
{
    f32 Result = ((f32)(End - Start) / (f32)Frequency);

    return Result;
}      

int
main(int argc, char **argv)
{
    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;
    GlobalLinuxState = BootstrapPushStruct(linux_state, Arena);

    s64 PerfCountFrequency = LinuxGetOSTimerFrequency();

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

    // NOTE(kstandbridge): Sound test
    linux_sound_output *SoundOutput = &GlobalLinuxState->SoundPlayer.Output; 
    LinuxInitSound(48000, SoundOutput->SecondaryBufferSize, &GlobalLinuxState->SoundPlayer);
    SoundOutput->SecondaryBufferSize = SoundOutput->SamplesPerSecond*SoundOutput->BytesPerSample;
    SoundOutput->LatencySampleCount = SoundOutput->SamplesPerSecond / 15;
    LinuxStartPlayingSound(&GlobalLinuxState->SoundPlayer);

    s32 MonitorRefreshHz = 60;
    s32 GameUpdateHz = MonitorRefreshHz / 2;
    u32 ExpectedFramesPerUpdate = 1;
    f32 TargetSecondsPerFrame = (f32)ExpectedFramesPerUpdate / (f32)GameUpdateHz;

    u32 NumberOfJoysticks = LinuxFindJoysticks(GlobalLinuxState);

    app_input Input[2] = {0};
    app_input *NewInput = &Input[0];
    app_input *OldInput = &Input[1];

    app_memory AppMemory = {0};

    b32 SoundIsValid = false;
    GlobalLinuxState->Running = true;

    u64 LastCounter = LinuxReadOSTimer();
    s64 LastCycleCount = __rdtsc();
    while(GlobalLinuxState->Running)
    {
        controller_input *OldKeyboardController = GetController(OldInput, 0);
        controller_input *NewKeyboardController = GetController(NewInput, 0);
        ZeroStruct(*NewKeyboardController);

        NewKeyboardController->IsConnected = true;
        for(s32 ButtonIndex = 0;
            ButtonIndex < ArrayCount(NewKeyboardController->Buttons);
            ++ButtonIndex)
        {
            NewKeyboardController->Buttons[ButtonIndex].EndedDown =
                OldKeyboardController->Buttons[ButtonIndex].EndedDown;
        }

        LinuxProcessPendingMessages(GlobalLinuxState, Display, Window, WmDeleteWindow, GraphicsContext, NewKeyboardController);
 
        LinuxJoystickPopulateGameInput(GlobalLinuxState, NewInput, OldInput, NumberOfJoysticks);

        
        // NOTE(kstandbridge): Sound output test
        u32 PlayCursor = SoundOutput->Buffer.ReadIndex;
        u32 WriteCursor = SoundOutput->Buffer.WriteIndex;
        if (!SoundIsValid)
        {
            SoundOutput->RunningSampleIndex = WriteCursor / SoundOutput->BytesPerSample;
            SoundIsValid = true;
        }
        
        u32 ByteToLock = ((SoundOutput->RunningSampleIndex*SoundOutput->BytesPerSample) %
                            SoundOutput->Buffer.Size);
        
        u32 ExpectedSoundBytesPerFrame =
            (u32)((f32)(SoundOutput->SamplesPerSecond*SoundOutput->BytesPerSample) * TargetSecondsPerFrame);
        
        u32 ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
        
        u32 SafeWriteCursor = WriteCursor + SoundOutput->SafetyBytes;
        b32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);
        
        u32 TargetCursor = 0;
        if(AudioCardIsLowLatency)
        {
            TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
        }
        else
        {
            TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame +
                            SoundOutput->SafetyBytes);
        }
        TargetCursor = (TargetCursor % SoundOutput->Buffer.Size);
        
        u32 BytesToWrite = 0;
        if(ByteToLock > TargetCursor)
        {
            BytesToWrite = (SoundOutput->Buffer.Size - ByteToLock);
            BytesToWrite += TargetCursor;
        }
        else
        {
            BytesToWrite = TargetCursor - ByteToLock;
        }

        sound_output_buffer SoundBuffer = 
        {
            .SamplesPerSecond = SoundOutput->SamplesPerSecond,
            .SampleCount = BytesToWrite / SoundOutput->BytesPerSample,
            .Samples = (s16 *)(SoundOutput->Buffer.Data + ByteToLock),
        };
        
        BytesToWrite = SoundBuffer.SampleCount*SoundOutput->BytesPerSample;

        if((GlobalLinuxState->Image) && 
           (GlobalLinuxState->Image->data))  
        {
            AppUpdateAndRender(&AppMemory, NewInput, &GlobalLinuxState->Backbuffer, &SoundBuffer);
        }

        if(SoundIsValid)
        {
            LinuxFillSoundBuffer(SoundOutput, &SoundBuffer);
        }

        u64 WorkCounter = LinuxReadOSTimer();
        f32 WorkSecondsElapsed = LinuxGetSecondsElapsed(LastCounter, WorkCounter, PerfCountFrequency);
                
        f32 SecondsElapsedForFrame = WorkSecondsElapsed;
        if(SecondsElapsedForFrame < TargetSecondsPerFrame)
        {                        
            useconds_t SleepMS = (useconds_t)(1000.0f * (TargetSecondsPerFrame -
                                                SecondsElapsedForFrame));
            if(SleepMS > 0)
            {
                usleep(SleepMS);
            }
            
            while(SecondsElapsedForFrame < TargetSecondsPerFrame)
            {                            
                SecondsElapsedForFrame = LinuxGetSecondsElapsed(LastCounter, LinuxReadOSTimer(), PerfCountFrequency);
            }
        }
        else
        {
            // TODO(kstandbridge): Missed frame?
        }

        window_dimension Dimension = LinuxGetWindowDimensions(Display, Window);
        LinuxDisplayBufferInWindow(GlobalLinuxState,
                                   Display, Window, GraphicsContext,
                                   Dimension.Width, Dimension.Height);

        u64 EndCycleCount = __rdtsc();
        
        u64 EndCounter = LinuxReadOSTimer();

        u64 CyclesElapsed = EndCycleCount - LastCycleCount;
        s64 CounterElapsed = EndCounter - LastCounter;
        f64 MSPerFrame = (((1000.0f*(f64)CounterElapsed) / (f64)PerfCountFrequency));
        f64 FPS = (f64)PerfCountFrequency / (f64)CounterElapsed;
        f64 MCPF = ((f64)CyclesElapsed / (1000.0f * 1000.0f));

#if 1
        LinuxConsoleOut("%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
#endif
        
        LastCounter = EndCounter;
        LastCycleCount = EndCycleCount;

        app_input *Temp = NewInput;
        NewInput = OldInput;
        OldInput = Temp;
        // TODO(kstandbridge): Should I clear these here?
    }

    GlobalLinuxState->SoundPlayer.IsPlaying = false;

    // TODO(kstandbridge): Figure out a way to wait for the sound thread to finish so we can free
    // If we had a thread pool could we do CompleteAllWork or similar?
    // LinuxFreeSound(&GlobalLinuxState->SoundPlayer);

    ClearArena(&GlobalLinuxState->Arena);

    XFreeGC(Display, GraphicsContext);
    XDestroyWindow(Display, Window);
    XFlush(Display);

    return 0;
}
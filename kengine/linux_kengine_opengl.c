#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <alsa/asoundlib.h>

#include <sys/mman.h>
#include <dlfcn.h>      // dlopen, dlsym, dlclose

#include <math.h>

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

#define VK_SHIFT_MASK  0x01
#define VK_CTRL_MASK   0x04
#define VK_ALT_MASK    0x08

#define VK_F1          67
#define VK_F2          68
#define VK_F3          69
#define VK_F4          70
#define VK_F5          71
#define VK_F6          72
#define VK_F7          73
#define VK_F8          74
#define VK_F9          75
#define VK_F10         76
#define VK_F11         95
#define VK_F12         96

typedef struct linux_audio_buffer
{
    u32 Size;
    volatile u32 ReadIndex;
    volatile u32 WriteIndex;
    u8 *Data;

} linux_audio_buffer;

typedef struct linux_sound_output
{
    u32 SamplesPerSecond;
    u32 RunningSampleIndex;
    u32 LastReadIndex;
    u32 LatencySampleCount;
    u32 BytesPerSample;
    u32 SafetyBytes;
    
    linux_audio_buffer Buffer;

    snd_pcm_uframes_t PeriodSize;
    u32 BytesPerFrame;

    snd_pcm_t *PcmHandle;

    s32 ToneHz;
    s16 ToneVolume;
    s32 WavePeriod;
    s32 SecondaryBufferSize;
    f32 tSine;
} linux_sound_output;

typedef struct linux_state
{
    memory_arena Arena;
    b32 Running;
    offscreen_buffer Backbuffer;

    Pixmap Pixmap;
    XImage *Image;
    
} linux_state;
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

global void *AlsaLibrary;

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
    *Address = LinuxLoadFunction(AlsaLibrary, Name);
    if (*Address == 0)
    {
        PlatformConsoleOut("Failed to load Alsa symbol %s\n", Address);
        InvalidCodePath;
    }
}
#define LINUX_ALSA_SYMBOL(type) LinuxLoadAlsaSymbol(#type, (void **) (char *) &ALSA_##type)

internal void
LinuxInitSound(s32 SamplesPerSecond, s32 BufferSize, linux_sound_output *SoundOutput)
{

#define AUDIO_CHANNELS 2

    // 48000 SamplesPerSecond
    u32 Periods = 4;     
    u32 PeriodSize = 512;
    
    Assert(AlsaLibrary == 0);

    AlsaLibrary = LinuxLoadLibrary("libasound.so");
    if(AlsaLibrary)
    {
        *(void **)(&snd_pcm_open) = dlsym(AlsaLibrary, "snd_pcm_open");

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


        u32 PlayerSamplesPerSecond = SamplesPerSecond;
        SoundOutput->PcmHandle = 0;
        SoundOutput->SamplesPerSecond = 0;
        //SoundOutput->IsPlaying = 0;
        SoundOutput->PeriodSize = PeriodSize;
        SoundOutput->BytesPerFrame = sizeof(s16) * AUDIO_CHANNELS;

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

        Error = snd_pcm_open(&SoundOutput->PcmHandle, Name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
        if(Error < 0) 
        {
            char *ErrorValue =  ALSA_snd_strerror(Error);
            PlatformConsoleOut("Init: cannot open audio device %s (%s)\n", Name, ErrorValue); 
            InvalidCodePath; 
        }
        Error = ALSA_snd_pcm_hw_params_malloc(&HwParams);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_any(SoundOutput->PcmHandle, HwParams);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_access(SoundOutput->PcmHandle, HwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_format(SoundOutput->PcmHandle, HwParams, SND_PCM_FORMAT_S16_LE);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_rate_near(SoundOutput->PcmHandle, HwParams, &PlayerSamplesPerSecond, 0);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_channels(SoundOutput->PcmHandle, HwParams, AUDIO_CHANNELS);
        if(Error < 0) InvalidCodePath;
        Error = ALSA_snd_pcm_hw_params_set_periods(SoundOutput->PcmHandle, HwParams, Periods, 0);
        if(Error < 0) InvalidCodePath;
        RequestedBufferSize = BufferSize = (PeriodSize * Periods) / (AUDIO_CHANNELS * sizeof(s16));
        Error = ALSA_snd_pcm_hw_params_set_buffer_size_near(SoundOutput->PcmHandle, HwParams, &BufferSize);
        if(Error < 0) InvalidCodePath;
        if (RequestedBufferSize != BufferSize) 
        {
            LinuxConsoleOut("Alsa adjusted buffer size from %lu to %lu\n", RequestedBufferSize, BufferSize);
        }
        Error = ALSA_snd_pcm_hw_params(SoundOutput->PcmHandle, HwParams);
        if(Error < 0) InvalidCodePath;

        ALSA_snd_pcm_hw_params_free(HwParams);

        SoundOutput->SamplesPerSecond = PlayerSamplesPerSecond;
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

internal void
LinuxFillSoundBuffer(linux_sound_output *SoundOutput, u32 Start, u32 BytesToWrite)
{
    void *Region1 = SoundOutput->Buffer.Data + Start;
    u32 Region1Size = BytesToWrite;
    u32 Region2Size = 0;
    if (Start + Region1Size >= SoundOutput->Buffer.Size)
    {
        u32 Diff = SoundOutput->Buffer.Size - Start;
        Region2Size = Region1Size - Diff;
        Region1Size = Diff;
    }

    u32 Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
    s16 *SampleOut = (s16 *)Region1;
    for(u32 SampleIndex = 0;
        SampleIndex < Region1SampleCount;
        ++SampleIndex)
    {
        // TODO(kstandbridge): Draw this out for people
        f32 SineValue = sinf(SoundOutput->tSine);
        s16 SampleValue = (s16)(SineValue * SoundOutput->ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        SoundOutput->tSine += 2.0f*Pi32*1.0f/(f32)SoundOutput->WavePeriod;
        ++SoundOutput->RunningSampleIndex;
    }

    if (Region2Size)
    {
        u32 Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        SampleOut = (s16 *)(Region1 + Region1Size);
        for(u32 SampleIndex = 0;
            SampleIndex < Region2SampleCount;
            ++SampleIndex)
        {
            f32 SineValue = sinf(SoundOutput->tSine);
            s16 SampleValue = (s16)(SineValue * SoundOutput->ToneVolume);
            *SampleOut++ = SampleValue;
            *SampleOut++ = SampleValue;

            SoundOutput->tSine += 2.0f*Pi32*1.0f/(f32)SoundOutput->WavePeriod;
            ++SoundOutput->RunningSampleIndex;
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
    
    // NOTE(kstandbridge): Graphics test
    s32 XOffset = 0;
    s32 YOffset = 0;

    // NOTE(kstandbridge): Sound test
    linux_sound_output SoundOutput = 
    {
        //.SamplesPerSecond = 48000,
        .ToneHz = 256,
        .ToneVolume = 3000,
        //.BytesPerSample = sizeof(s16)*2,
    };
    LinuxInitSound(48000, SoundOutput.SecondaryBufferSize, &SoundOutput);
    SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
    SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
    SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
    LinuxFillSoundBuffer(&SoundOutput, 0, SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample);

    s32 MonitorRefreshHz = 60;
    f32 GameUpdateHz = (f32)(MonitorRefreshHz);
    u32 ExpectedFramesPerUpdate = 1;
    f32 TargetSecondsPerFrame = (f32)ExpectedFramesPerUpdate / (f32)GameUpdateHz;

    b32 SoundIsValid = false;
    GlobalLinuxState->Running = true;
    while(GlobalLinuxState->Running)
    {
        LinuxProcessPendingMessages(GlobalLinuxState, Display, Window, WmDeleteWindow, GraphicsContext);
 
        // TODO(kstandbridge): Process controller input

        // TODO(kstandbridge): Check controller input
        ++XOffset;
        YOffset += 2;

        if((GlobalLinuxState->Image) && 
           (GlobalLinuxState->Image->data))
        {
            RenderWeirdGradient(&GlobalLinuxState->Backbuffer, XOffset, YOffset);
        }
        
        // NOTE(kstandbridge): Sound output test
        u32 PlayCursor = SoundOutput.Buffer.ReadIndex;
        u32 WriteCursor = SoundOutput.Buffer.WriteIndex;
        if (!SoundIsValid)
        {
            SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
            SoundIsValid = true;
        }
        
        u32 ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
                            SoundOutput.Buffer.Size);
        
        u32 ExpectedSoundBytesPerFrame =
        (u32)((f32)(SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) * TargetSecondsPerFrame);
        
        u32 ExpectedFrameBoundaryByte = PlayCursor + ExpectedSoundBytesPerFrame;
        
        u32 SafeWriteCursor = WriteCursor + SoundOutput.SafetyBytes;
        b32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);
        
        u32 TargetCursor = 0;
        if(AudioCardIsLowLatency)
        {
            TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
        }
        else
        {
            TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame +
                            SoundOutput.SafetyBytes);
        }
        TargetCursor = (TargetCursor % SoundOutput.Buffer.Size);
        
        u32 BytesToWrite = 0;
        if(ByteToLock > TargetCursor)
        {
            BytesToWrite = (SoundOutput.Buffer.Size - ByteToLock);
            BytesToWrite += TargetCursor;
        }
        else
        {
            BytesToWrite = TargetCursor - ByteToLock;
        }

        LinuxFillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite);

        // NOTE(kstandbridge): Audio runner?
        {
#if 0
            pthread_mutex_lock(&GlobalAudioMutex);

            u32 RegionSize1 = SoundPeriodSize;
            u32 RegionSize2 = 0;
            if ((RegionSize1 + SoundOutput.Buffer.ReadIndex) >= SoundOutput.Buffer.Size)
            {
                u32 Diff = SoundOutput.Buffer.Size - SoundOutput.Buffer.ReadIndex;
                RegionSize2 = RegionSize1 - Diff;
                RegionSize1 = Diff;
                Copy(RegionSize1, SoundOutput.Buffer.Data + SoundOutput.Buffer.ReadIndex, SoundBuffer);
                Copy(RegionSize2, SoundOutput.Buffer.Data, SoundBuffer + RegionSize1);
                SoundOutput.Buffer.ReadIndex = RegionSize2;
            }
            else
            {
                Copy(RegionSize1, SoundOutput.Buffer.Data + SoundOutput.Buffer.ReadIndex, SoundBuffer);
                SoundOutput.Buffer.ReadIndex += RegionSize1;
            }

            pthread_mutex_unlock(&GlobalAudioMutex);

            snd_pcm_sframes_t ReturnValue = ALSA_snd_pcm_writei(SoundOutput.PcmHandle, SoundBuffer, SoundPeriodSize);
            if (ReturnValue < 0)
            {
                ALSA_snd_pcm_prepare(SoundOutput.PcmHandle);
            }
#else
            u32 CopySize = SoundOutput.PeriodSize;
            if ((CopySize + SoundOutput.Buffer.ReadIndex) > SoundOutput.Buffer.Size)
            {
                CopySize = SoundOutput.Buffer.Size - SoundOutput.Buffer.ReadIndex;
            }
            
            u32 FrameCount = CopySize / SoundOutput.BytesPerFrame;
            snd_pcm_sframes_t ReturnValue = ALSA_snd_pcm_writei(SoundOutput.PcmHandle, SoundOutput.Buffer.Data + SoundOutput.Buffer.ReadIndex,
                                                                FrameCount);
            
            if (ReturnValue < 0)
            {
                ALSA_snd_pcm_prepare(SoundOutput.PcmHandle);
            }
            else
            {
                u32 WrittenBytes = ReturnValue * SoundOutput.BytesPerFrame;
                u32 NextReadIndex = SoundOutput.Buffer.ReadIndex + WrittenBytes;
                if (NextReadIndex >= SoundOutput.Buffer.Size) {
                    NextReadIndex -= SoundOutput.Buffer.Size;
                }
                
                u32 NextWriteIndex = NextReadIndex + SoundOutput.PeriodSize;
                if (NextWriteIndex >= SoundOutput.Buffer.Size) {
                    NextWriteIndex -= SoundOutput.Buffer.Size;
                }
                
                SoundOutput.Buffer.WriteIndex = NextWriteIndex;
                CompletePreviousWritesBeforeFutureWrites;
                SoundOutput.Buffer.ReadIndex = NextReadIndex;
            }
#endif
        }

        window_dimension Dimension = LinuxGetWindowDimensions(Display, Window);
        LinuxDisplayBufferInWindow(GlobalLinuxState,
                                   Display, Window, GraphicsContext,
                                   Dimension.Width, Dimension.Height);
    }

    XFreeGC(Display, GraphicsContext);
    XDestroyWindow(Display, Window);
    XFlush(Display);

    return 0;
}
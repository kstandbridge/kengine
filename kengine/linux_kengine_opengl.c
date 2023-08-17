#include <math.h>

#include "handmade.h"
#include "handmade.c"

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <alsa/asoundlib.h>

#include <sys/mman.h>
#include <dlfcn.h>      // dlopen, dlsym, dlclose
#include <pthread.h>


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

    s32 ToneHz;
    s16 ToneVolume;
    s32 WavePeriod;
    s32 SecondaryBufferSize;
    f32 tSine;
} linux_sound_output;

typedef struct linux_sound_player
{
    snd_pcm_t *PcmHandle;
    u32 SamplesPerSecond;
    volatile b32 IsPlaying;
    pthread_t Thread;
    snd_pcm_uframes_t PeriodSize;
    u32 BytesPerFrame;
    

    linux_sound_output Output;

} linux_sound_player;

typedef struct linux_state
{
    memory_arena Arena;
    b32 Running;
    offscreen_buffer Backbuffer;

    Pixmap Pixmap;
    XImage *Image;

    linux_sound_player SoundPlayer;
    
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
    
    // NOTE(kstandbridge): Graphics test
    s32 XOffset = 0;
    s32 YOffset = 0;

    // NOTE(kstandbridge): Sound test
    linux_sound_output *SoundOutput = &GlobalLinuxState->SoundPlayer.Output; 
    SoundOutput->ToneHz = 256;
    SoundOutput->ToneVolume = 3000;
    LinuxInitSound(48000, SoundOutput->SecondaryBufferSize, &GlobalLinuxState->SoundPlayer);
    SoundOutput->WavePeriod = SoundOutput->SamplesPerSecond/SoundOutput->ToneHz;
    SoundOutput->SecondaryBufferSize = SoundOutput->SamplesPerSecond*SoundOutput->BytesPerSample;
    SoundOutput->LatencySampleCount = SoundOutput->SamplesPerSecond / 15;
    LinuxStartPlayingSound(&GlobalLinuxState->SoundPlayer);

    s32 MonitorRefreshHz = 60;
    f32 GameUpdateHz = (f32)(MonitorRefreshHz);
    u32 ExpectedFramesPerUpdate = 1;
    f32 TargetSecondsPerFrame = (f32)ExpectedFramesPerUpdate / (f32)GameUpdateHz;

    u64 LastCounter = LinuxReadOSTimer();
    s64 LastCycleCount = __rdtsc();

    b32 SoundIsValid = false;
    GlobalLinuxState->Running = true;
    while(GlobalLinuxState->Running)
    {
        LinuxProcessPendingMessages(GlobalLinuxState, Display, Window, WmDeleteWindow, GraphicsContext);
 
        // TODO(kstandbridge): Process controller input

        // TODO(kstandbridge): Check controller input
        ++XOffset;
        YOffset += 2;
        
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
            GameUpdateAndRender(&GlobalLinuxState->Backbuffer, XOffset, YOffset, &SoundBuffer, SoundOutput->ToneHz);
        }

        if(SoundIsValid)
        {
            LinuxFillSoundBuffer(SoundOutput, &SoundBuffer);
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
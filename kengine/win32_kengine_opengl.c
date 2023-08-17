#include <math.h>

#include "handmade.h"
#include "handmade.c"

#include <windows.h>
#include <xinput.h>
#include <dsound.h>



typedef struct win32_sound_output
{
    s32 SamplesPerSecond;
    s32 ToneHz;
    s16 ToneVolume;
    u32 RunningSampleIndex;
    s32 WavePeriod;
    s32 BytesPerSample;
    s32 SecondaryBufferSize;
    f32 tSine;
    s32 LatencySampleCount;
} win32_sound_output;

typedef struct win32_state
{
    memory_arena Arena;
    b32 Running;
    offscreen_buffer Backbuffer;

    BITMAPINFO Info;
    LPDIRECTSOUNDBUFFER SecondaryBuffer;
} win32_state;
global win32_state *GlobalWin32State;

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub)
{
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);

internal void
Win32LoadXInput(void)    
{
    // TODO(kstandbridge): Test this on Windows 8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        // TODO(kstandbridge): Diagnostic
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }

    if(!XInputLibrary)
    {
        // TODO(kstandbridge): Diagnostic
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    
    if(XInputLibrary)
    {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState");
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}

        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState");
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}

        // TODO(kstandbridge): Diagnostic
    }
    else
    {
        // TODO(kstandbridge): Diagnostic
    }
}

internal void
Win32InitDSound(HWND Window, s32 SamplesPerSecond, s32 BufferSize)
{
    // NOTE(kstandbridge): Load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if(DSoundLibrary)
    {
        // NOTE(kstandbridge): Get a DirectSound object! - cooperative
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)
            GetProcAddress(DSoundLibrary, "DirectSoundCreate");

        // TODO(kstandbridge): Double-check that this works on XP - DirectSound8 or 7??
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0)))
        {
            WAVEFORMATEX WaveFormat =
            {
                .wFormatTag = WAVE_FORMAT_PCM,
                .nChannels = 2,
                .nSamplesPerSec = SamplesPerSecond,
                .wBitsPerSample = 16,
                .cbSize = 0
            };
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;

            if(SUCCEEDED(DirectSound->lpVtbl->SetCooperativeLevel(DirectSound, Window, DSSCL_PRIORITY)))
            {
                DSBUFFERDESC BufferDescription = 
                {
                    .dwSize = sizeof(BufferDescription),
                    .dwFlags = DSBCAPS_PRIMARYBUFFER,
                };

                // NOTE(kstandbridge): "Create" a primary buffer
                // TODO(kstandbridge): DSBCAPS_GLOBALFOCUS?
                LPDIRECTSOUNDBUFFER PrimaryBuffer;
                if(SUCCEEDED(DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &BufferDescription, &PrimaryBuffer, 0)))
                {
                    HRESULT Error = PrimaryBuffer->lpVtbl->SetFormat(PrimaryBuffer, &WaveFormat);
                    if(SUCCEEDED(Error))
                    {
                        // NOTE(kstandbridge): We have finally set the format!
                        PlatformConsoleOut("Primary buffer format was set.\n");
                    }
                    else
                    {
                        // TODO(kstandbridge): Diagnostic
                    }
                }
                else
                {
                    // TODO(kstandbridge): Diagnostic
                }
            }
            else
            {
                // TODO(kstandbridge): Diagnostic
            }

            // TODO(kstandbridge): DSBCAPS_GETCURRENTPOSITION2
            DSBUFFERDESC BufferDescription = 
            {
                .dwSize = sizeof(BufferDescription),
                .dwFlags = 0,
                .dwBufferBytes = BufferSize,
                .lpwfxFormat = &WaveFormat,
            };
            HRESULT Error = DirectSound->lpVtbl->CreateSoundBuffer(DirectSound, &BufferDescription, &GlobalWin32State->SecondaryBuffer, 0);
            if(SUCCEEDED(Error))
            {
                PlatformConsoleOut("Secondary buffer created successfully.\n");
            }
        }
        else
        {
            // TODO(kstandbridge): Diagnostic
        }
    }
    else
    {
        // TODO(kstandbridge): Diagnostic
    }
}

internal window_dimension
Win32GetWindowDimension(HWND Window)
{
    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    
    window_dimension Result = 
    {
        .Width = ClientRect.right - ClientRect.left,
        .Height = ClientRect.bottom - ClientRect.top
    };

    return Result;
}

internal void
Win32ResizeDIBSection(win32_state *State, s32 Width, s32 Height)
{
    offscreen_buffer *Buffer = &State->Backbuffer;

    // TODO(kstandbridge): Bulletproof this.
    // Maybe don't free first, free after, then free first if that fails.

    if(Buffer->Memory)
    {
        // TODO(kstandbridge): Free the Buffer-Memory
        // VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;

    int BytesPerPixel = 4;

    // NOTE(kstandbridge): When the biHeight field is negative, this is the clue to
    // Windows to treat this bitmap as top-down, not bottom-up, meaning that
    // the first three bytes of the image are the color for the top left pixel
    // in the bitmap, not the bottom left!
    State->Info.bmiHeader.biSize = sizeof(State->Info.bmiHeader);
    State->Info.bmiHeader.biWidth = Buffer->Width;
    State->Info.bmiHeader.biHeight = -Buffer->Height;
    State->Info.bmiHeader.biPlanes = 1;
    State->Info.bmiHeader.biBitCount = 32;
    State->Info.bmiHeader.biCompression = BI_RGB;

    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*BytesPerPixel;
    Buffer->Memory = PushSize_(&State->Arena, BitmapMemorySize, DefaultArenaPushParams());
    Buffer->Pitch = Width*BytesPerPixel;

    // TODO(kstandbridge): Probably clear this to black
}

internal void
Win32DisplayBufferInWindow(win32_state *State,
                           HDC DeviceContext, s32 WindowWidth, s32 WindowHeight)
{
    offscreen_buffer *Buffer = &State->Backbuffer;

    // TODO(kstandbridge): Aspect ratio correction
    // TODO(kstandbridge): Plat with stetch mnodes
    StretchDIBits(DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &State->Info,
        DIB_RGB_COLORS, SRCCOPY);
}

internal LRESULT CALLBACK
Win32MainWindowCallback(HWND Window,
                        UINT Message,
                        WPARAM WParam,
                        LPARAM LParam)
{
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_CLOSE:
        {
            GlobalWin32State->Running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            PlatformConsoleOut("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY:
        {
            GlobalWin32State->Running = false;
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            u32 VKCode = WParam;
            b32 WasDown = ((LParam & (1 << 30)) != 0);
            b32 IsDown = ((LParam & (1 << 31)) == 0);
            if(WasDown != IsDown)
            {
                if(VKCode == 'W')
                {
                }
                else if(VKCode == 'A')
                {
                }
                else if(VKCode == 'S')
                {
                }
                else if(VKCode == 'D')
                {
                }
                else if(VKCode == 'Q')
                {
                }
                else if(VKCode == 'E')
                {
                }
                else if(VKCode == VK_UP)
                {
                }
                else if(VKCode == VK_LEFT)
                {
                }
                else if(VKCode == VK_DOWN)
                {
                }
                else if(VKCode == VK_RIGHT)
                {
                }
                else if(VKCode == VK_ESCAPE)
                {
                    PlatformConsoleOut("ESCAPE: ");
                    if(IsDown)
                    {
                        PlatformConsoleOut("IsDown ");
                    }
                    if(WasDown)
                    {
                        PlatformConsoleOut("WasDown");
                    }
                    PlatformConsoleOut("\n");
                }
                else if(VKCode == VK_SPACE)
                {
                }
            }

            b32 AltKeyWasDown = (LParam & (1 << 29));
            if((VKCode == VK_F4) && AltKeyWasDown)
            {
                GlobalWin32State->Running = false;
            }
        } break;

        case WM_PAINT:
        {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(GlobalWin32State, DeviceContext,
                                       Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal void
Win32ClearSoundBuffer(win32_sound_output *SoundOutput)
{
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalWin32State->SecondaryBuffer->lpVtbl->Lock(GlobalWin32State->SecondaryBuffer, 0, SoundOutput->SecondaryBufferSize,
                                                                 &Region1, &Region1Size, &Region2, &Region2Size, 0)))
    {
        // TODO(kstandbridge): assert that Region1Size/Region2Size is valid
        u8 *DestSample = (u8 *)Region1;
        for(DWORD ByteIndex = 0;
            ByteIndex < Region1Size;
            ++ByteIndex)
        {
            *DestSample++ = 0;
        }
        
        DestSample = (u8 *)Region2;
        for(DWORD ByteIndex = 0;
            ByteIndex < Region2Size;
            ++ByteIndex)
        {
            *DestSample++ = 0;
        }

        GlobalWin32State->SecondaryBuffer->lpVtbl->Unlock(GlobalWin32State->SecondaryBuffer, Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
Win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite,
                     sound_output_buffer *SourceBuffer)
{
    // TODO(kstandbridge): More strenuous test!
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalWin32State->SecondaryBuffer->lpVtbl->Lock(GlobalWin32State->SecondaryBuffer, ByteToLock, BytesToWrite,
                                                                 &Region1, &Region1Size, &Region2, &Region2Size, 0)))
    {
        // TODO(kstandbridge): assert that Region1Size/Region2Size is valid

        // TODO(kstandbridge): Collapse these two loops
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        s16 *DestSample = (s16 *)Region1;
        s16 *SourceSample = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0;
            SampleIndex < Region1SampleCount;
            ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (s16 *)Region2;
        for(DWORD SampleIndex = 0;
            SampleIndex < Region2SampleCount;
            ++SampleIndex)
        {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalWin32State->SecondaryBuffer->lpVtbl->Unlock(GlobalWin32State->SecondaryBuffer, Region1, Region1Size, Region2, Region2Size);
    }    
}

int CALLBACK
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;
    GlobalWin32State = BootstrapPushStruct(win32_state, Arena);

    s64 PerfCountFrequency = Win32GetOSTimerFrequency();

    Win32LoadXInput();

    WNDCLASS WindowClass = 
    {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = Win32MainWindowCallback,
        .hInstance = hInstance,
        // .hIcon,
        .lpszClassName = "HandmadeHeroWindowClass",
    };
    

    Win32ResizeDIBSection(GlobalWin32State, 1280, 720);
    
    if(RegisterClassA(&WindowClass))
    {
        HWND Window =
            CreateWindowExA(
                0,
                WindowClass.lpszClassName,
                "Handmade Hero",
                WS_OVERLAPPEDWINDOW | WS_VISIBLE,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                CW_USEDEFAULT,
                0,
                0,
                hInstance,
                0);

        if(Window)
        {
            HDC DeviceContext = GetDC(Window);

            // NOTE(kstandbridge): Graphics test
            s32 XOffset = 0;
            s32 YOffset = 0;

            // NOTE(kstandbridge): Sound test
            win32_sound_output SoundOutput = 
            {
                .SamplesPerSecond = 48000,
                .ToneHz = 256,
                .ToneVolume = 3000,
                .BytesPerSample = sizeof(s16)*2,
            };
            SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);
            GlobalWin32State->SecondaryBuffer->lpVtbl->Play(GlobalWin32State->SecondaryBuffer, 0, 0, DSBPLAY_LOOPING);
            
            s16 *SoundSamples = (s16 *)PushSize_(&GlobalWin32State->Arena, SoundOutput.SecondaryBufferSize, DefaultArenaPushParams());

            u64 LastCounter = Win32ReadOSTimer();
            s64 LastCycleCount = __rdtsc();

            GlobalWin32State->Running = true;
            while(GlobalWin32State->Running)
            {
                MSG Message;
                while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
                {
                    if(Message.message == WM_QUIT)
                    {
                        GlobalWin32State->Running = false;
                    }
                    
                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                // TODO(kstandbridge): Should we poll this more frequently
                for (DWORD ControllerIndex = 0;
                     ControllerIndex < XUSER_MAX_COUNT;
                     ++ControllerIndex)
                {
                    XINPUT_STATE ControllerState;
                    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                    {
                        // NOTE(kstandbridge): This controller is plugged in
                        // TODO(kstandbridge): See if ControllerState.dwPacketNumber increments too rapidly
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        b32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        b32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        b32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        b32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        b32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        b32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        b32 LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        b32 RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        b32 AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        b32 BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        b32 XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        b32 YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
        
                        s16 StickX = Pad->sThumbLX;
                        s16 StickY = Pad->sThumbLY;

                        // TODO(kstandbridge): Check controller input
                        // TODO(kstandbridge): We will do deadzone handling later using
                        // XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE
                        // XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE 8689

                        XOffset += StickX / 4096;
                        YOffset += StickY / 4096;

                        SoundOutput.ToneHz = 512 + (s32)(256.0f*((f32)StickY / 30000.0f));
                        SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                    }
                    else
                    {
                        // NOTE(kstandbridge): The controller is not available
                    }
                }

                DWORD ByteToLock = 0;
                DWORD TargetCursor = 0;
                DWORD BytesToWrite = 0;
                DWORD PlayCursor = 0;
                DWORD WriteCursor = 0;
                b32 SoundIsValid = false;
                // TODO(kstandbridge): Tighten up sound logic so that we know where we should be
                // writing to and can anticipate the time spent in the game update.
                if(SUCCEEDED(GlobalWin32State->SecondaryBuffer->lpVtbl->GetCurrentPosition(GlobalWin32State->SecondaryBuffer, &PlayCursor, &WriteCursor)))
                {
                    ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
                                        SoundOutput.SecondaryBufferSize);

                    TargetCursor =
                        ((PlayCursor +
                          (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) %
                         SoundOutput.SecondaryBufferSize);
                    if(ByteToLock > TargetCursor)
                    {
                        BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                        BytesToWrite += TargetCursor;
                    }
                    else
                    {
                        BytesToWrite = TargetCursor - ByteToLock;
                    }

                    SoundIsValid = true;
                }

                sound_output_buffer SoundBuffer = 
                {
                    .SamplesPerSecond = SoundOutput.SamplesPerSecond,
                    .SampleCount = BytesToWrite / SoundOutput.BytesPerSample,
                    .Samples = SoundSamples,
                };

                
                GameUpdateAndRender(&GlobalWin32State->Backbuffer, XOffset, YOffset, &SoundBuffer, SoundOutput.ToneHz);

                // NOTE(casey): DirectSound output test
                if(SoundIsValid)
                {
                    Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                }

                window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(GlobalWin32State, DeviceContext,
                                            Dimension.Width, Dimension.Height);

                u64 EndCycleCount = __rdtsc();
                
                u64 EndCounter = Win32ReadOSTimer();

                u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                s64 CounterElapsed = EndCounter - LastCounter;
                f64 MSPerFrame = (((1000.0f*(f64)CounterElapsed) / (f64)PerfCountFrequency));
                f64 FPS = (f64)PerfCountFrequency / (f64)CounterElapsed;
                f64 MCPF = ((f64)CyclesElapsed / (1000.0f * 1000.0f));

#if 1
                Win32ConsoleOut("%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
#endif
                
                LastCounter = EndCounter;
                LastCycleCount = EndCycleCount;
            }
        }
        else
        {
            // TODO(kstandbridge): Logging
        }
    }
    else
    {
        // TODO(kstandbridge): Logging
    }
    
    return 0;
}
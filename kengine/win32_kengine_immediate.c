#include <xinput.h>
#include <dsound.h>

#include "win32_kengine_immediate.h"

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
                .dwFlags = DSBCAPS_GETCURRENTPOSITION2,
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

    s32 BytesPerPixel = 4;
    Buffer->BytesPerPixel = BytesPerPixel;

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

    if((WindowWidth >= Buffer->Width*2) &&
       (WindowHeight >= Buffer->Height*2))
    {
        StretchDIBits(DeviceContext,
                      0, 0, 2*Buffer->Width, 2*Buffer->Height,
                      0, 0, Buffer->Width, Buffer->Height,
                      Buffer->Memory,
                      &State->Info,
                      DIB_RGB_COLORS, SRCCOPY);
    }
    else
    {
        s32 OffsetX = 10;
        s32 OffsetY = 10;

        PatBlt(DeviceContext, 0, 0, WindowWidth, OffsetY, BLACKNESS);
        PatBlt(DeviceContext, 0, OffsetY + Buffer->Height, WindowWidth, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, 0, 0, OffsetX, WindowHeight, BLACKNESS);
        PatBlt(DeviceContext, OffsetX + Buffer->Width, 0, WindowWidth, WindowHeight, BLACKNESS);

        StretchDIBits(DeviceContext,
            OffsetX, OffsetY, Buffer->Width, Buffer->Height,
            0, 0, Buffer->Width, Buffer->Height,
            Buffer->Memory,
            &State->Info,
            DIB_RGB_COLORS, SRCCOPY);
    }
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
            Win32ConsoleOut("WM_ACTIVATEAPP\n");
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
            Win32ConsoleOut("Error: keyboard input from non-dispatched message!\n");
            InvalidCodePath;
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

internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState,
                                button_state *OldState, DWORD ButtonBit,
                                button_state *NewState)
{
    NewState->EndedDown = ((XInputButtonState & ButtonBit) == ButtonBit);
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;
}

internal f32
Win32ProcessXInputStickValue(SHORT Value, SHORT DeadZoneThreshold)
{
    f32 Result = 0;
    
    if(Value < -DeadZoneThreshold)
    {
        Result = (f32)((Value + DeadZoneThreshold) / (32768.0f - DeadZoneThreshold));
    }
    else if(Value > DeadZoneThreshold)
    {
        Result = (f32)((Value - DeadZoneThreshold) / (32767.0f - DeadZoneThreshold));
    }
    
    return Result;
}

internal void
Win32ProcessKeyboardMessage(button_state *NewState, b32 IsDown)
{
    if(NewState->EndedDown != IsDown)
    {
        NewState->EndedDown = IsDown;
        ++NewState->HalfTransitionCount;
    }
}

internal void
Win32ToggleFullscreen(HWND Window)
{
    DWORD Style = GetWindowLong(Window, GWL_STYLE);
    if(Style & WS_OVERLAPPEDWINDOW)
    {
        MONITORINFO MonitorInfo = {sizeof(MonitorInfo)};
        if(GetWindowPlacement(Window, &GlobalWin32State->WindowPlacement) &&
           GetMonitorInfo(MonitorFromWindow(Window, MONITOR_DEFAULTTOPRIMARY), &MonitorInfo))
        {
            SetWindowLong(Window, GWL_STYLE, Style & ~WS_OVERLAPPEDWINDOW);
            SetWindowPos(Window, HWND_TOP,
                         MonitorInfo.rcMonitor.left, MonitorInfo.rcMonitor.top,
                         MonitorInfo.rcMonitor.right - MonitorInfo.rcMonitor.left,
                         MonitorInfo.rcMonitor.bottom - MonitorInfo.rcMonitor.top,
                         SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
    }
    else
    {
        SetWindowLong(Window, GWL_STYLE, Style | WS_OVERLAPPEDWINDOW);
        SetWindowPlacement(Window, &GlobalWin32State->WindowPlacement);
        SetWindowPos(Window, 0, 0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                     SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
    }
}

internal void
Win32ProcessPendingMessages(controller_input *KeyboardController)
{
    MSG Message;
    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalWin32State->Running = false;
            } break;
            
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP:
            {
                b32 AltKeyWasDown = (Message.lParam & (1 << 29));

                u32 VKCode = (u32)Message.wParam;
                b32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                b32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown)
                {
                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveUp, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveLeft, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveDown, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->MoveRight, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E')
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionUp, IsDown);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionLeft, IsDown);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionDown, IsDown);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->ActionRight, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Back, IsDown);
                    }
                    else if(VKCode == VK_SPACE)
                    {
                        Win32ProcessKeyboardMessage(&KeyboardController->Start, IsDown);
                    }
#if KENGINE_INTERNAL
                    else if(VKCode == 'P')
                    {
                        if(IsDown)
                        {
                            GlobalWin32State->Pause = !GlobalWin32State->Pause;
                        }
                    }
                    else if(VKCode == 'L')
                    {
                        if(IsDown)
                        {
                            if(GlobalMemory.InputPlayingIndex == 0)
                            {
                                if(GlobalMemory.InputRecordingIndex == 0)
                                {
                                    Win32BeginRecordingInput(1);
                                }
                                else
                                {
                                    Win32EndRecordingInput();
                                    Win32BeginInputPlayBack(1);
                                }
                            }
                            else
                            {
                                Win32EndInputPlayBack();
                            }
                        }
                    }
#endif
                }

                if(IsDown)
                {
                    if((VKCode == VK_F4) && AltKeyWasDown)
                    {
                        GlobalWin32State->Running = false;
                    }
                    else if((VKCode == VK_RETURN) && AltKeyWasDown)
                    {
                        if(Message.hwnd)
                        {
                            Win32ToggleFullscreen(Message.hwnd);
                        }
                    }
                }
            } break;

            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
        }
    }
}

#if 0

internal void
Win32DebugDrawVertical(offscreen_buffer *Backbuffer,
                  s32 X, s32 Top, s32 Bottom, u32 Color)
{
    if(Top <= 0)
    {
        Top = 0;
    }

    if(Bottom > Backbuffer->Height)
    {
        Bottom = Backbuffer->Height;
    }
    
    if((X >= 0) && (X < Backbuffer->Width))
    {
        u8 *Pixel = ((u8 *)Backbuffer->Memory +
                        X*Backbuffer->BytesPerPixel +
                        Top*Backbuffer->Pitch);
        for(s32 Y = Top;
            Y < Bottom;
            ++Y)
        {
            *(u32 *)Pixel = Color;
            Pixel += Backbuffer->Pitch;
        }
    }
}

inline void
Win32DrawSoundBufferMarker(offscreen_buffer *Backbuffer,
                           win32_sound_output *SoundOutput,
                           f32 C, int PadX, int Top, int Bottom,
                           DWORD Value, u32 Color)
{
    f32 XReal32 = (C * (f32)Value);
    int X = PadX + (int)XReal32;
    Win32DebugDrawVertical(Backbuffer, X, Top, Bottom, Color);
}


internal void
Win32DebugSyncDisplay(offscreen_buffer *Backbuffer,
                      int MarkerCount, win32_debug_time_marker *Markers,
                      int CurrentMarkerIndex,
                      win32_sound_output *SoundOutput, f32 TargetSecondsPerFrame)
{
    int PadX = 16;
    int PadY = 16;

    int LineHeight = 64;
    
    f32 C = (f32)(Backbuffer->Width - 2*PadX) / (f32)SoundOutput->SecondaryBufferSize;
    for(int MarkerIndex = 0;
        MarkerIndex < MarkerCount;
        ++MarkerIndex)
    {
        win32_debug_time_marker *ThisMarker = &Markers[MarkerIndex];
        Assert(ThisMarker->OutputPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputWriteCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputLocation < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->OutputByteCount < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipPlayCursor < SoundOutput->SecondaryBufferSize);
        Assert(ThisMarker->FlipWriteCursor < SoundOutput->SecondaryBufferSize);

        DWORD PlayColor = 0xFFFFFFFF;
        DWORD WriteColor = 0xFFFF0000;
        DWORD ExpectedFlipColor = 0xFFFFFF00;
        DWORD PlayWindowColor = 0xFFFF00FF;

        int Top = PadY;
        int Bottom = PadY + LineHeight;
        if(MarkerIndex == CurrentMarkerIndex)
        {
            Top += LineHeight+PadY;
            Bottom += LineHeight+PadY;

            int FirstTop = Top;
            
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputPlayCursor, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputWriteCursor, WriteColor);

            Top += LineHeight+PadY;
            Bottom += LineHeight+PadY;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation, PlayColor);
            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->OutputLocation + ThisMarker->OutputByteCount, WriteColor);

            Top += LineHeight+PadY;
            Bottom += LineHeight+PadY;

            Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, FirstTop, Bottom, ThisMarker->ExpectedFlipPlayCursor, ExpectedFlipColor);
        }        
        
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor, PlayColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipPlayCursor + 480*SoundOutput->BytesPerSample, PlayWindowColor);
        Win32DrawSoundBufferMarker(Backbuffer, SoundOutput, C, PadX, Top, Bottom, ThisMarker->FlipWriteCursor, WriteColor);
    }
}

#endif

inline f32
Win32GetSecondsElapsed(u64 Start, u64 End, u64 Frequency)
{
    f32 Result = ((f32)(End - Start) / (f32)Frequency);

    return Result;
}

internal FILETIME
Win32GetLastWriteTime(char *FileName)
{
    FILETIME Result = {0};

    WIN32_FILE_ATTRIBUTE_DATA Data;
    if(GetFileAttributesExA(FileName, GetFileExInfoStandard, &Data))
    {
        Result = Data.ftLastWriteTime;
    }

    return Result;
}

internal win32_app_code
Win32LoadAppCode(char *LockPath, char *AppCodePath, char *TempAppCodePath)
{
    win32_app_code Result = {0};

    WIN32_FILE_ATTRIBUTE_DATA Ignored;
    if(GetFileAttributesExA(LockPath, GetFileExInfoStandard, &Ignored))
    {
        // NOTE(kstandbridge): Lock file exists, so still building
    }
    else
    {
        Result.LibraryLastWriteTime = Win32GetLastWriteTime(AppCodePath);

        CopyFile(AppCodePath, TempAppCodePath, FALSE);

        Result.Library = LoadLibrary(TempAppCodePath);

        if(Result.Library)
        {
            Result.UpdateAndRender = (app_update_and_render *)GetProcAddress(Result.Library, "AppUpdateAndRender");    

            Result.GetSoundSamples = (app_get_sound_samples *)GetProcAddress(Result.Library, "AppGetSoundSamples");

            Result.IsValid = (Result.UpdateAndRender &&
                            Result.GetSoundSamples);    
        }

        if(!Result.IsValid)
        {
            Result.UpdateAndRender = 0;
            Result.GetSoundSamples = 0;
        }
    }

    return Result;
}

internal void
Win32UnloadAppCode(win32_app_code *AppCode)
{
    if(AppCode->Library)
    {
        FreeLibrary(AppCode->Library);
        AppCode->Library = 0;
    }

    AppCode->IsValid = false;
    AppCode->UpdateAndRender = 0;
    AppCode->GetSoundSamples = 0;
}

int CALLBACK
WinMain(HINSTANCE hInstance,
        HINSTANCE hPrevInstance,
        LPSTR lpCmdLine,
        int nCmdShow)
{
    InitGlobalMemory();

    GlobalWin32State = BootstrapPushStruct(win32_state, Arena);

    s64 PerfCountFrequency = Win32GetOSTimerFrequency();
    // NOTE(kstandbridge): Set the Windows scheduler granularity to 1ms
    UINT DesiredSchedulerMS = 1;
    b32 SleepIsGranular = (timeBeginPeriod(DesiredSchedulerMS) == TIMERR_NOERROR);

    char LockPath[MAX_PATH];
    FormatStringToBuffer(LockPath, sizeof(LockPath), "%Slock.tmp", GlobalMemory.ExeDirectoryPath);
    char AppCodePath[MAX_PATH];
    FormatStringToBuffer(AppCodePath, sizeof(AppCodePath), "%Shandmade.dll", GlobalMemory.ExeDirectoryPath);
    char TempAppCodePath[MAX_PATH];
    FormatStringToBuffer(TempAppCodePath, sizeof(TempAppCodePath), "%Shandmade_temp.dll", GlobalMemory.ExeDirectoryPath);

    Win32LoadXInput();

    Win32ResizeDIBSection(GlobalWin32State, 960, 540);

    WNDCLASS WindowClass = 
    {
        .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
        .lpfnWndProc = Win32MainWindowCallback,
        .hInstance = hInstance,
        // .hIcon,
        .lpszClassName = "HandmadeHeroWindowClass",
    };

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
            // TODO(kstandbridge): How to query this on Windows?
            s32 MonitorRefreshHz = 60;
            HDC RefreshDC = GetDC(Window);
            s32 Win32RefreshRate = GetDeviceCaps(RefreshDC, VREFRESH);
            ReleaseDC(Window, RefreshDC);
            if(Win32RefreshRate > 1)
            {
                MonitorRefreshHz = Win32RefreshRate;
            }
            f32 GameUpdateHz = (MonitorRefreshHz / 2.0f);
            f32 TargetSecondsPerFrame = 1.0f / (f32)GameUpdateHz;
            
            win32_sound_output SoundOutput = 
            {
                .SamplesPerSecond = 48000,
                .BytesPerSample = sizeof(s16)*2,
            };
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            // TODO(kstandbridge): Get rid of LatencySampleCount
            SoundOutput.LatencySampleCount = 3*(SoundOutput.SamplesPerSecond / GameUpdateHz);
            // TODO(kstandbridge): Acrtuallyh compute this variance and see what the lowest reasonable value is.
            SoundOutput.SafetyBytes = (s32)(((f32)SoundOutput.SamplesPerSecond*(f32)SoundOutput.BytesPerSample / GameUpdateHz)/3.0f);
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            Win32ClearSoundBuffer(&SoundOutput);
            GlobalWin32State->SecondaryBuffer->lpVtbl->Play(GlobalWin32State->SecondaryBuffer, 0, 0, DSBPLAY_LOOPING);
            
            s16 *SoundSamples = (s16 *)PushSize_(&GlobalWin32State->Arena, SoundOutput.SecondaryBufferSize, DefaultArenaPushParams());

            app_input Input[2] = {0};
            app_input *NewInput = &Input[0];
            app_input *OldInput = &Input[1];
            
            app_memory AppMemory = 
            {
                .PlatformAPI.ConsoleOut = Win32ConsoleOut,
                .PlatformAPI.ConsoleOut_ = Win32ConsoleOut_,
                .PlatformAPI.AllocateMemory = Win32AllocateMemory,
                .PlatformAPI.DeallocateMemory = Win32DeallocateMemory,
                .PlatformAPI.FileExist = Win32FileExists,
                .PlatformAPI.ReadEntireFile = Win32ReadEntireFile,
                .PlatformAPI.WriteTextToFile = Win32WriteTextToFile,
                .PlatformAPI.OpenFile = Win32OpenFile,
                .PlatformAPI.WriteFile = Win32WriteFile,
                .PlatformAPI.CloseFile = Win32CloseFile,
                .PlatformAPI.GetOSTimerFrequency = Win32GetOSTimerFrequency,
                .PlatformAPI.ReadOSTimer = Win32ReadOSTimer,
            };

            GlobalWin32State->Running = true;

            u64 LastCounter = Win32ReadOSTimer();
            u64 FlipWallClock = Win32ReadOSTimer();

            s32 DebugTimeMarkerIndex = 0;
            win32_debug_time_marker DebugTimeMarkers[30] = {0};

            b32 SoundIsValid = false;

            win32_app_code AppCode = Win32LoadAppCode(LockPath, AppCodePath, TempAppCodePath);

#if 0
            s64 LastCycleCount = __rdtsc();
#endif
            while(GlobalWin32State->Running)
            {
                NewInput->dtForFrame = TargetSecondsPerFrame;

                FILETIME LibraryLastWriteTime = Win32GetLastWriteTime(AppCodePath);
                if(CompareFileTime(&LibraryLastWriteTime, &AppCode.LibraryLastWriteTime) != 0)
                {
                    Win32UnloadAppCode(&AppCode);
                    AppCode = Win32LoadAppCode(LockPath, AppCodePath, TempAppCodePath);
                }

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

                DWORD MouseButtons[MouseButton_Count] =
                {
                    VK_LBUTTON,
                    VK_MBUTTON,
                    VK_RBUTTON,
                    VK_XBUTTON1,
                    VK_XBUTTON2
                };
                for(s32 ButtonIndex = 0;
                    ButtonIndex < ArrayCount(NewInput->MouseButtons);
                    ++ButtonIndex)
                {
                    NewInput->MouseButtons[ButtonIndex] = OldInput->MouseButtons[ButtonIndex];
                    NewInput->MouseButtons[ButtonIndex].HalfTransitionCount = 0;
                    Win32ProcessKeyboardMessage(&NewInput->MouseButtons[ButtonIndex],
                                                GetKeyState(MouseButtons[ButtonIndex]) & (1 << 15));
                }
                POINT Point;
                GetCursorPos(&Point);
                ScreenToClient(Window, &Point);
                NewInput->MouseX = Point.x;
                NewInput->MouseY = Point.y;

                Win32ProcessPendingMessages(NewKeyboardController);

                if(!GlobalWin32State->Pause)
                {
                    // TODO(kstandbridge): Should we poll this more frequently
                    // NOTE(kstandbridge): Keyboard is index 0, controllers start at 1
                    s32 MaxControllerCount = XUSER_MAX_COUNT;
                    if(MaxControllerCount > (ArrayCount(NewInput->Controllers) - 1))
                    {
                        MaxControllerCount = (ArrayCount(NewInput->Controllers) - 1);
                    }

                    for(DWORD ControllerIndex = 0;
                        ControllerIndex < MaxControllerCount;
                        ++ControllerIndex)
                    {              
                        DWORD OurControllerIndex = ControllerIndex + 1;     
                        controller_input *OldController = &OldInput->Controllers[OurControllerIndex];
                        controller_input *NewController = &NewInput->Controllers[OurControllerIndex];

                        XINPUT_STATE ControllerState;
                        if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS)
                        {
                            NewController->IsConnected = true;
                            NewController->IsAnalog = OldController->IsAnalog;
                            
                            // NOTE(kstandbridge): This controller is plugged in
                            // TODO(kstandbridge): See if ControllerState.dwPacketNumber increments too rapidly
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;
                            
                            // TODO(kstandbridge): This is a square deadzone, check XInput to
                            // verify that the deadzone is "round" and show how to do
                            // round deadzone processing.
                            NewController->StickAverageX = Win32ProcessXInputStickValue(
                                Pad->sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            NewController->StickAverageY = Win32ProcessXInputStickValue(
                                Pad->sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
                            if((NewController->StickAverageX != 0.0f) ||
                                (NewController->StickAverageY != 0.0f))
                            {
                                NewController->IsAnalog = true;
                            }
                            
                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP)
                            {
                                NewController->StickAverageY = 1.0f;
                                NewController->IsAnalog = false;
                            }
                            
                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN)
                            {
                                NewController->StickAverageY = -1.0f;
                                NewController->IsAnalog = false;
                            }
                            
                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT)
                            {
                                NewController->StickAverageX = -1.0f;
                                NewController->IsAnalog = false;
                            }
                            
                            if(Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT)
                            {
                                NewController->StickAverageX = 1.0f;
                                NewController->IsAnalog = false;
                            }
                            
                            f32 Threshold = 0.5f;
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX < -Threshold) ? 1 : 0,
                                &OldController->MoveLeft, 1,
                                &NewController->MoveLeft);
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageX > Threshold) ? 1 : 0,
                                &OldController->MoveRight, 1,
                                &NewController->MoveRight);
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY < -Threshold) ? 1 : 0,
                                &OldController->MoveDown, 1,
                                &NewController->MoveDown);
                            Win32ProcessXInputDigitalButton(
                                (NewController->StickAverageY > Threshold) ? 1 : 0,
                                &OldController->MoveUp, 1,
                                &NewController->MoveUp);
                            
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionDown, XINPUT_GAMEPAD_A,
                                                            &NewController->ActionDown);
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionRight, XINPUT_GAMEPAD_B,
                                                            &NewController->ActionRight);
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionLeft, XINPUT_GAMEPAD_X,
                                                            &NewController->ActionLeft);
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->ActionUp, XINPUT_GAMEPAD_Y,
                                                            &NewController->ActionUp);
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER,
                                                            &NewController->LeftShoulder);
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER,
                                                            &NewController->RightShoulder);
                            
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->Start, XINPUT_GAMEPAD_START,
                                                            &NewController->Start);
                            Win32ProcessXInputDigitalButton(Pad->wButtons,
                                                            &OldController->Back, XINPUT_GAMEPAD_BACK,
                                                            &NewController->Back);
                        }
                        else
                        {
                            // NOTE(kstandbridge): The controller is not available
                            NewController->IsConnected = false;
                        }
                    }

                    if(GlobalMemory.InputRecordingIndex)
                    {
                        Win32RecordInput(NewInput);
                    }
                    if(GlobalMemory.InputPlayingIndex)
                    {
                        app_input Temp = *NewInput;
                        Win32PlayBackInput(NewInput);
                        
                        // NOTE(kstandbridge): Keep mouse input for debug menus/etc
                        for(u32 MouseButtonIndex = 0;
                                MouseButtonIndex < MouseButton_Count;
                                ++MouseButtonIndex)
                        {
                            NewInput->MouseButtons[MouseButtonIndex] = Temp.MouseButtons[MouseButtonIndex];
                        }
                        NewInput->MouseX = Temp.MouseX;
                        NewInput->MouseY = Temp.MouseY;
                    }

                    if(AppCode.IsValid)
                    {
                        AppCode.UpdateAndRender(&AppMemory, NewInput, &GlobalWin32State->Backbuffer);
                    }

                    u64 AudioWallClock = Win32ReadOSTimer();
                    f32 FromBeginToAudioSeconds = Win32GetSecondsElapsed(FlipWallClock, AudioWallClock, PerfCountFrequency);

                    DWORD PlayCursor;
                    DWORD WriteCursor;
                    if(GlobalWin32State->SecondaryBuffer->lpVtbl->GetCurrentPosition(GlobalWin32State->SecondaryBuffer, &PlayCursor, &WriteCursor) == DS_OK)
                    {
                        if(!SoundIsValid)
                        {
                            SoundOutput.RunningSampleIndex = WriteCursor / SoundOutput.BytesPerSample;
                            SoundIsValid = true;
                        }

                        DWORD ByteToLock = ((SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) %
                                            SoundOutput.SecondaryBufferSize);

                        DWORD ExpectedSoundBytesPerFrame =
                            (s32)((f32)(SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample) / GameUpdateHz);
                        f32 SecondsLeftUntilFlip = (TargetSecondsPerFrame - FromBeginToAudioSeconds);
                        DWORD ExpectedBytesUntilFlip = (DWORD)((SecondsLeftUntilFlip/TargetSecondsPerFrame)*(f32)ExpectedSoundBytesPerFrame);

                        DWORD ExpectedFrameBoundaryByte = PlayCursor + ExpectedBytesUntilFlip;
                    
                        DWORD SafeWriteCursor = WriteCursor;
                        if(SafeWriteCursor < PlayCursor)
                        {
                            SafeWriteCursor += SoundOutput.SecondaryBufferSize;
                        }
                        Assert(SafeWriteCursor >= PlayCursor);
                        SafeWriteCursor += SoundOutput.SafetyBytes;
                    
                        b32 AudioCardIsLowLatency = (SafeWriteCursor < ExpectedFrameBoundaryByte);                        

                        DWORD TargetCursor = 0;
                        if(AudioCardIsLowLatency)
                        {
                            TargetCursor = (ExpectedFrameBoundaryByte + ExpectedSoundBytesPerFrame);
                        }
                        else
                        {
                            TargetCursor = (WriteCursor + ExpectedSoundBytesPerFrame +
                                            SoundOutput.SafetyBytes);
                        }
                        TargetCursor = (TargetCursor % SoundOutput.SecondaryBufferSize);
                    
                        DWORD BytesToWrite = 0;
                        if(ByteToLock > TargetCursor)
                        {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                            BytesToWrite += TargetCursor;
                        }
                        else
                        {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }

                        sound_output_buffer SoundBuffer = 
                        {
                            .SamplesPerSecond = SoundOutput.SamplesPerSecond,
                            .SampleCount = BytesToWrite / SoundOutput.BytesPerSample,
                            .Samples = SoundSamples,
                        };
                        if(AppCode.IsValid)
                        {
                            AppCode.GetSoundSamples(&AppMemory, &SoundBuffer);
                        }
#if KENGINE_INTERNAL
                        win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                        Marker->OutputPlayCursor = PlayCursor;
                        Marker->OutputWriteCursor = WriteCursor;
                        Marker->OutputLocation = ByteToLock;
                        Marker->OutputByteCount = BytesToWrite;
                        Marker->ExpectedFlipPlayCursor = ExpectedFrameBoundaryByte;

                        DWORD UnwrappedWriteCursor = WriteCursor;
                        if(UnwrappedWriteCursor < PlayCursor)
                        {
                            UnwrappedWriteCursor += SoundOutput.SecondaryBufferSize;
                        }
    #if 0
                        u32 AudioLatencyBytes = UnwrappedWriteCursor - PlayCursor;
                        f32 AudioLatencySeconds =
                            (((f32)AudioLatencyBytes / (f32)SoundOutput.BytesPerSample) /
                                (f32)SoundOutput.SamplesPerSecond);
                    
                        Win32ConsoleOut("BTL:%u TC:%u BTW:%u - PC:%u WC:%u DELTA:%u (%fs)\n",
                                        ByteToLock, TargetCursor, BytesToWrite,
                                        PlayCursor, WriteCursor, AudioLatencyBytes, AudioLatencySeconds);
    #endif
#endif
                        Win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);
                    }
                    else
                    {
                        SoundIsValid = false;
                    }
                
                    u64 WorkCounter = Win32ReadOSTimer();
                    f32 WorkSecondsElapsed = Win32GetSecondsElapsed(LastCounter, WorkCounter, PerfCountFrequency);
                    
                    f32 SecondsElapsedForFrame = WorkSecondsElapsed;
                    if(SecondsElapsedForFrame < TargetSecondsPerFrame)
                    {                        
                        if(SleepIsGranular)
                        {
                            DWORD SleepMS = (DWORD)(1000.0f * (TargetSecondsPerFrame -
                                                                SecondsElapsedForFrame));
                            if(SleepMS > 0)
                            {
                                Sleep(SleepMS);
                            }
                        }
                        
                        while(SecondsElapsedForFrame < TargetSecondsPerFrame)
                        {                            
                            SecondsElapsedForFrame = Win32GetSecondsElapsed(LastCounter, Win32ReadOSTimer(), PerfCountFrequency);
                        }
                    }
                    else
                    {
                        // TODO(kstandbridge): Missed frame?
                    }

                    u64 EndCounter = Win32ReadOSTimer();
                    LastCounter = EndCounter;

                    window_dimension Dimension = Win32GetWindowDimension(Window);
#if 0
                    Win32DebugSyncDisplay(&GlobalWin32State->Backbuffer, ArrayCount(DebugTimeMarkers), DebugTimeMarkers,
                                        DebugTimeMarkerIndex - 1, &SoundOutput, TargetSecondsPerFrame);
#endif
                    HDC DeviceContext = GetDC(Window);
                    Win32DisplayBufferInWindow(GlobalWin32State, DeviceContext,
                                                Dimension.Width, Dimension.Height);
                    ReleaseDC(Window, DeviceContext);

                    FlipWallClock = Win32ReadOSTimer();

#if KENGINE_INTERNAL
                    {
                        DWORD PlayCursor;
                        DWORD WriteCursor;
                        if(GlobalWin32State->SecondaryBuffer->lpVtbl->GetCurrentPosition(GlobalWin32State->SecondaryBuffer, &PlayCursor, &WriteCursor) == DS_OK)
                        {
                            Assert(DebugTimeMarkerIndex < ArrayCount(DebugTimeMarkers));
                            win32_debug_time_marker *Marker = &DebugTimeMarkers[DebugTimeMarkerIndex];
                            Marker->FlipPlayCursor = PlayCursor;
                            Marker->FlipWriteCursor = WriteCursor;
                        }
                    
                    }
#endif
                    app_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                    
#if 0
                    u64 EndCycleCount = __rdtsc();
                    u64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    LastCycleCount = EndCycleCount;

                    f64 MSPerFrame = 1000.0f*Win32GetSecondsElapsed(LastCounter, EndCounter, PerfCountFrequency);
                    f64 FPS = 0.0f;
                    f64 MCPF = ((f64)CyclesElapsed / (1000.0f * 1000.0f));

                    Win32ConsoleOut("%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
#endif

#if KENGINE_INTERNAL
                    ++DebugTimeMarkerIndex;
                    if(DebugTimeMarkerIndex == ArrayCount(DebugTimeMarkers))
                    {
                        DebugTimeMarkerIndex = 0;
                    }
#endif
                }
                else
                {
                    // TODO(kstandbridge): Logging
                }
            }
        }
    }
    else
    {
        // TODO(kstandbridge): Logging
    }
    
    return 0;
}

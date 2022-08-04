#include "win32_kengine.h"
#include "win32_kengine_shared.c"

#include "kengine_generated.c"
#include "win32_kengine_generated.c"
#include "kengine_renderer_software.c"
#include "kengine_sort.c"
#include "kengine_renderer.c"

internal void
Win32LoadLibraries()
{
    User32 = LoadLibraryA("User32.dll");
    Assert(User32);
    Gdi32 = LoadLibraryA("Gdi32.dll");
    Assert(Gdi32);
}

internal LRESULT
Win32MainWindowCallback_(win32_state *Win32State, HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    switch(Message)
    {
        case WM_CLOSE:
        {
            Win32State->IsRunning = false;
        } break;
        
        case WM_SIZE:
        {
            
            RECT ClientRect;
            if(GetClientRect(Window, &ClientRect))
            {
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                Backbuffer->Width = ClientRect.right - ClientRect.left;
                Backbuffer->Height = ClientRect.bottom - ClientRect.top;
                
                
                if(Backbuffer->Memory)
                {
                    VirtualFree(Backbuffer->Memory, 0, MEM_RELEASE);
                }
                
                Backbuffer->Info.bmiHeader.biSize = sizeof(Backbuffer->Info.bmiHeader);
                Backbuffer->Info.bmiHeader.biWidth = Backbuffer->Width;
                Backbuffer->Info.bmiHeader.biHeight = Backbuffer->Height;
                Backbuffer->Info.bmiHeader.biPlanes = 1;
                Backbuffer->Info.bmiHeader.biBitCount = 32;
                Backbuffer->Info.bmiHeader.biCompression = BI_RGB;
                
                // TODO(kstandbridge): Need to figure out a way we can align this
                //Backbuffer->Pitch = Align16(Backbuffer->Width*BITMAP_BYTES_PER_PIXEL);
                Backbuffer->Pitch = Backbuffer->Width*BITMAP_BYTES_PER_PIXEL;
                
                Backbuffer->Memory = VirtualAlloc(0, sizeof(u32)*Backbuffer->Pitch*Backbuffer->Height,
                                                  MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
                
            }
            else
            {
                // TODO(kstandbridge): Error failed to get client rect?
                InvalidCodePath;
            }
            
        } break;
        
        default:
        {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }
    
    return Result;
}

internal LRESULT __stdcall
Win32MainWindowCallback(HWND Window, u32 Message, WPARAM WParam, LPARAM LParam)
{
    LRESULT Result = 0;
    
    win32_state *Win32State = 0;
    
    if(Message == WM_NCCREATE)
    {
        CREATESTRUCTA *CreateStruct = (CREATESTRUCTA *)LParam;
        Win32State = (win32_state *)CreateStruct->lpCreateParams;
        SetWindowLongPtrA(Window, GWLP_USERDATA, (LONG_PTR)Win32State);
    }
    else
    {
        Win32State = (win32_state *)GetWindowLongPtrA(Window, GWLP_USERDATA);
    }
    
    if(Win32State)
    {        
        Result = Win32MainWindowCallback_(Win32State, Window, Message, WParam, LParam);
    }
    else
    {
        Result = DefWindowProcA(Window, Message, WParam, LParam);
    }
    
    return Result;
    
}

internal void
Win32ProcessPendingMessages(win32_state *Win32State)
{
    Win32State;
    
    MSG Message;
    while(PeekMessageA(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                __debugbreak();
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}

void __stdcall 
WinMainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
    Win32LoadLibraries();
    
    HINSTANCE Instance = GetModuleHandleA(0);
    
    WNDCLASSEXA WindowClass;
    ZeroStruct(WindowClass);
    WindowClass.cbSize = sizeof(WNDCLASSEXA);
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
    WindowClass.lpszClassName = "KengineWindowClass";
    
    win32_state Win32State_;
    ZeroStruct(Win32State_);
    win32_state *Win32State = &Win32State_;
    
    if(RegisterClassExA(&WindowClass))
    {
        Win32State->Window = CreateWindowExA(0,
                                             WindowClass.lpszClassName,
                                             "kengine",
                                             WS_OVERLAPPEDWINDOW,
                                             CW_USEDEFAULT, CW_USEDEFAULT, 1280, 720,
                                             0, 0, Instance, Win32State);
        if(Win32State->Window)
        {
            ShowWindow(Win32State->Window, SW_SHOW);
            
            u32 PushBufferSize = Megabytes(64);
            void *PushBuffer = VirtualAlloc(0, PushBufferSize, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
            
            render_commands Commands_ = RenderCommands(PushBufferSize, 0, PushBuffer, 0, PushBufferSize);
            render_commands *Commands = &Commands_;
            
            Win32State->IsRunning = true;
            while(Win32State->IsRunning)
            {
                Win32ProcessPendingMessages(Win32State);
                
                HDC DeviceContext = GetDC(Win32State->Window);
                
                win32_offscreen_buffer *Backbuffer = &Win32State->Backbuffer;
                
                loaded_bitmap OutputTarget;
                ZeroStruct(OutputTarget);
                OutputTarget.Memory = Backbuffer->Memory;
                OutputTarget.Width = Backbuffer->Width;
                OutputTarget.Height = Backbuffer->Height;
                OutputTarget.Pitch = Backbuffer->Pitch;
                
                // TODO(kstandbridge): Do some rendering
                
#if 0                
                DrawRectangle(&OutputTarget, V2(10.0f, 10.0f), V2(100, 100), V4(1.0f, 0.5f, 0.0f, 1.0f), 
                              Rectangle2i(0, Backbuffer->Width, 0, Backbuffer->Height));
#else
                render_group RenderGroup;
                RenderGroup.Commands = Commands;
                PushRenderEntryClear(&RenderGroup, V4(1.0f, 0.5f, 0.0f, 1.0f), 1.0f);
                
                sort_entry *SortEntries = (sort_entry *)(Commands->PushBufferBase + Commands->SortEntryAt);
                sort_entry *SortEntry = SortEntries;
                for(u32 SortEntryIndex = 0;
                    SortEntryIndex < Commands->PushBufferElementCount;
                    ++SortEntryIndex, ++SortEntry)
                {
                    render_group_command_header *Header = (render_group_command_header *)(Commands->PushBufferBase + SortEntry->Index);
                    void *Data = (u8 *)Header + sizeof(*Header);
                    switch(Header->Type)
                    {
                        case RenderGroupCommandType_Clear:
                        {
                            render_group_command_clear *Command = (render_group_command_clear *)Data;
                            DrawRectangle(&OutputTarget, V2(0.0f, 0.0f), V2((f32)Backbuffer->Width, (f32)Backbuffer->Height), Command->Color, 
                                          Rectangle2i(0, Backbuffer->Width, 0, Backbuffer->Height));
                        } break;
                        
                        InvalidDefaultCase;
                    }
                }
                
#endif
                
                if(!StretchDIBits(DeviceContext, 0, 0, Backbuffer->Width, Backbuffer->Height, 0, 0, Backbuffer->Width, Backbuffer->Height,
                                  Backbuffer->Memory, &Backbuffer->Info, DIB_RGB_COLORS, SRCCOPY))
                {
                    InvalidCodePath;
                }
                
                
                if(!ReleaseDC(Win32State->Window, DeviceContext))
                {
                    InvalidCodePath;
                }
                
                _mm_pause();
            }
        }
        else
        {
            // TODO(kstandbridge): Error creating window
        }
    }
    else
    {
        // TODO(kstandbridge): Error registering window class
    }
    
    ExitProcess(0);
    
    InvalidCodePath;
}
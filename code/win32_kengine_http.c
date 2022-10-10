#include "win32_kengine_http.h"

#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"
#include "win32_kengine_shared.c"
#include "kengine_telemetry.c"

inline http_io_request *
SafeGetNextRequest(server_context *ServerContext)
{
    http_io_request *Result = 0;
    for(;;)
    {
        u32 OrigionalNext = ServerContext->NextRequestIndex;
        u32 NewNext = (OrigionalNext + 1) % ServerContext->RequestCount;
        u32 Index = AtomicCompareExchangeU32(&ServerContext->NextRequestIndex, NewNext, OrigionalNext);
        if(Index == OrigionalNext)
        {
            Result = ServerContext->Requests + Index;
            break;
        }
        else
        {
            _mm_pause();
        }
    }
    return Result;
}

inline http_io_request *
AllocateHttpIoRequest(server_context *ServerContext)
{
    http_io_request *Result = SafeGetNextRequest(ServerContext);
    
    Result->IoContext.ServerContext = ServerContext;
    Result->IoContext.CompletionFunction = ReceiveCompletionCallback;
    Result->HttpRequest = (HTTP_REQUEST *)Result->Buffer;
    
    return Result;
}

inline http_io_response *
SafeGetNextResponse(server_context *ServerContext)
{
    http_io_response *Result = 0;
    for(;;)
    {
        u32 OrigionalNext = ServerContext->NextResponseIndex;
        u32 NewNext = (OrigionalNext + 1) % ServerContext->ResponseCount;
        u32 Index = AtomicCompareExchangeU32(&ServerContext->NextResponseIndex, NewNext, OrigionalNext);
        if(Index == OrigionalNext)
        {
            Result = ServerContext->Responses + Index;
            break;
        }
        else
        {
            _mm_pause();
        }
    }
    return Result;
}

internal void
SendCompletionCallback(http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult)
{
    http_io_response *IoResponse = CONTAINING_RECORD(pIoContext, http_io_response, IoContext);
}

inline http_io_response *
AllocateHttpIoResponse(server_context *ServerContext)
{
    http_io_response *Result = SafeGetNextResponse(ServerContext);
    Result->IoContext.ServerContext = ServerContext;
    Result->IoContext.CompletionFunction = SendCompletionCallback;
    
    Result->HttpResponse.EntityChunkCount = 1;
    Result->HttpResponse.pEntityChunks = &Result->HttpDataChunk;
    
    char *ContentType = "application/json";
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = ContentType;
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = GetNullTerminiatedStringLength(ContentType);
    
    return Result;
}

internal u32
Win32InitializeHttpServer(server_context *ServerContext)
{
    LogVerbose("Initializing HTTP server");
    
    ServerContext->Version.HttpApiMajorVersion = 2;
    ServerContext->Version.HttpApiMinorVersion = 0;
    
    u32 Result = Win32HttpInitialize(ServerContext->Version, HTTP_INITIALIZE_SERVER, 0);
    if(Result == NO_ERROR)
    {
        ServerContext->HttpInit = true;
        LogVerbose("Creating server session");
        Result = Win32HttpCreateServerSession(ServerContext->Version, &ServerContext->SessionId, 0);
        if(Result == NO_ERROR)
        {
            LogVerbose("Creating url group");
            Result = Win32HttpCreateUrlGroup(ServerContext->SessionId, &ServerContext->UrlGroupId, 0);
            if(Result == NO_ERROR)
            {
                LogVerbose("Adding url to url group");
                Result = Win32HttpAddUrlToUrlGroup(ServerContext->UrlGroupId, L"http://localhost:8090/", 0, 0);
                if(Result != NO_ERROR)
                {
                    LogError("HttpAddUrlToUrlGroup failed with error code %u", Result);
                }
            }
            else
            {
                LogError("HttpCreateUrlGroup failed with error code %u", Result);
            }
        }
        else
        {
            LogError("HttpCreateServerSession failed with error code %u", Result);
        }
    }
    else
    {
        LogError("HttpInitialize failed with error code %u", Result);
    }
    
    return Result;
}

internal void
Win32UnitializeHttpServer(server_context *ServerContext)
{
    LogVerbose("Uninitializing HTTP server");
    
    if(ServerContext->UrlGroupId != 0)
    {
        u32 Result = Win32HttpCloseUrlGroup(ServerContext->UrlGroupId);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseUrlGroup failed with error code %u", Result);
        }
        ServerContext->UrlGroupId = 0;
    }
    
    if(ServerContext->SessionId != 0)
    {
        u32 Result = Win32HttpCloseServerSession(ServerContext->SessionId);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseServerSession failed with error code %u", Result);
        }
        ServerContext->SessionId = 0;
    }
    
    if(ServerContext->HttpInit)
    {
        u32 Result = Win32HttpTerminate(HTTP_INITIALIZE_SERVER, 0);
        if(Result != NO_ERROR)
        {
            LogError("HttpTerminate failed with error code %u", Result);
        }
        ServerContext->HttpInit = false;
    }
}

internal void
Win32IoCallbackThread(PTP_CALLBACK_INSTANCE Instance, PVOID Context, void *pOverlapped, ULONG IoResult, ULONG_PTR BytesTransferred, PTP_IO IoThreadpool)
{
    server_context *ServerContext;
    
    http_io_context *IoContext = CONTAINING_RECORD(pOverlapped, http_io_context, Overlapped);
    
    ServerContext = IoContext->ServerContext;
    
    IoContext->CompletionFunction(IoContext, IoThreadpool, IoResult);
    
}

internal b32
Win32InitializeIoThreadPool(server_context *ServerContext)
{
    LogVerbose("Initializing I/O completion Context");
    
    LogVerbose("Creating request queue");
    u32 Result = Win32HttpCreateRequestQueue(ServerContext->Version, L"AsyncHttpServer", 0, 0, &ServerContext->RequestQueue);
    if(Result == NO_ERROR)
    {
        HTTP_BINDING_INFO HttpBindingInfo;
        ZeroStruct(HttpBindingInfo);
        HttpBindingInfo.Flags.Present = 1;
        HttpBindingInfo.RequestQueueHandle = ServerContext->RequestQueue;
        LogVerbose("Setting url group property");
        Result = Win32HttpSetUrlGroupProperty(ServerContext->UrlGroupId, HttpServerBindingProperty, &HttpBindingInfo, sizeof(HttpBindingInfo));
        
        if(Result == NO_ERROR)
        {
            LogVerbose("Creating I/O thread pool");
            ServerContext->IoThreadpool = Win32CreateThreadpoolIo(ServerContext->RequestQueue, Win32IoCallbackThread, 0, 0);
            
            if(ServerContext->IoThreadpool == 0)
            {
                LogError("Creating I/O thread pool failed");
                Result = false;
            }
            else
            {
                Result = true;
            }
        }
        else
        {
            LogError("HttpSetUrlGroupProperty failed with error code %u", Result);
        }
    }
    else
    {
        LogError("HttpCreateRequestQueue failed with error code %u", Result);
    }
    
    return Result;
}

internal void
Win32UninitializeIoCompletionContext(server_context *ServerContext)
{
    LogVerbose("Uninitializing I/O completion Context");
    
    if(ServerContext->RequestQueue != 0)
    {
        LogVerbose("Closing request queue");
        u32 Result = Win32HttpCloseRequestQueue(ServerContext->RequestQueue);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseRequestQueue failed with error code %u", Result);
        }
        ServerContext->RequestQueue = 0;
    }
    
    if(ServerContext->IoThreadpool != 0)
    {
        LogVerbose("Closing I/O thread pool");
        Win32CloseThreadpoolIo(ServerContext->IoThreadpool);
        ServerContext->IoThreadpool = 0;
    }
}

inline http_io_response *
CreateMessageResponse(server_context *ServerContext, u16 StatusCode, string Reason, string Message)
{
    http_io_response *Result = AllocateHttpIoResponse(ServerContext);
    
    if(Result)
    {
        Result->HttpResponse.StatusCode = StatusCode;
        Result->HttpResponse.pReason = (char *)Reason.Data;
        Result->HttpResponse.ReasonLength = Reason.Size;
        
        Result->HttpResponse.pEntityChunks[0].DataChunkType = HttpDataChunkFromMemory;
        Result->HttpResponse.pEntityChunks[0].FromMemory.pBuffer = (char *)Message.Data;
        Result->HttpResponse.pEntityChunks[0].FromMemory.BufferLength = Message.Size;
    }
    else
    {
        LogError("Failed to allocate http io response");
    }
    
    return Result;
}

internal void
ProcessReceiveAndPostResponse(http_io_request *IoRequest, PTP_IO IoThreadpool, u32 IoResult)
{
    server_context *ServerContext = IoRequest->IoContext.ServerContext;
    http_io_response *IoResponse = 0;
    
    switch(IoResult)
    {
        case NO_ERROR:
        {
            string Payload = String_(IoRequest->HttpRequest->pEntityChunks->FromMemory.BufferLength, 
                                     IoRequest->HttpRequest->pEntityChunks->FromMemory.pBuffer);
            LogVerbose("Received %S", Payload);
            if(IoRequest->HttpRequest->Verb == HttpVerbGET)
            {
                IoResponse = CreateMessageResponse(ServerContext, 200, String("OK"), Payload);
#if 0
                IoResponse = CreateMessageResponse(ServerContext, 200, String("OK"), String("{ \"Message\": \"Hello, world!\" }"));
#endif
            }
            else
            {
                LogVerbose("Recieved an unknown request: %S", Payload);
                IoResponse = CreateMessageResponse(ServerContext, 501, String("Not Implemented"), String("{ \"Error\": \"Not Implemented\" }"));
            }
        } break;
        
        case ERROR_MORE_DATA:
        {
            IoResponse = CreateMessageResponse(ServerContext, 413, String("Request Entity Too Large"), String("Large buffer support is not implemented"));
        } break;
        
        default:
        {
            LogError("HttpReceiveHttpRequest call failed asynchronously with error code %u", IoResult);
        } break;
    }
    
    if(IoResponse != 0)
    {
        Win32StartThreadpoolIo(IoThreadpool);
        
        u32 Result = Win32HttpSendHttpResponse(ServerContext->RequestQueue, IoRequest->HttpRequest->RequestId, 0, &IoResponse->HttpResponse,
                                               0, 0, 0, 0, &IoResponse->IoContext.Overlapped, 0);
        
        if((Result != NO_ERROR) &&
           (Result != ERROR_IO_PENDING))
        {
            Win32CancelThreadpoolIo(IoThreadpool);
            
            LogError("HttpSendHttpResponse failed with error code %u", Result);
        }
    }
}

internal void
PostNewReceive(server_context *ServerContext, PTP_IO IoThreadpool)
{
    http_io_request *IoRequest = AllocateHttpIoRequest(ServerContext);
    
    if(IoRequest != 0)
    {
        Win32StartThreadpoolIo(IoThreadpool);
        
        u32 Result = Win32HttpReceiveHttpRequest(ServerContext->RequestQueue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                                 IoRequest->HttpRequest, sizeof(IoRequest->Buffer), 0,
                                                 &IoRequest->IoContext.Overlapped);
        
        if((Result != ERROR_IO_PENDING) &&
           (Result != NO_ERROR))
        {
            Win32CancelThreadpoolIo(IoThreadpool);
            
            LogError("HttpReceiveHttpRequest failed with error code %u", Result);
            
            if(Result == ERROR_MORE_DATA)
            {
                ProcessReceiveAndPostResponse(IoRequest, IoThreadpool, ERROR_MORE_DATA);
            }
        }
    }
    else
    {
        LogError("Allocation of IoRequest failed");
    }
}

internal void
ReceiveCompletionCallback(http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult)
{
    http_io_request *IoRequest = CONTAINING_RECORD(pIoContext, http_io_request, IoContext);
    
    server_context *ServerContext = IoRequest->IoContext.ServerContext;
    
    if(ServerContext->StopServer == false)
    {
        ProcessReceiveAndPostResponse(IoRequest, IoThreadpool, IoResult);
        
        PostNewReceive(ServerContext, IoThreadpool);
    }
}

internal b32
Win32StartHttpServer(server_context *ServerContext)
{
#define OUTSTANDING_REQUESTS 16
#define REQUESTS_PER_PROCESSOR 4
    
    b32 Result = true;
    
    u32 RequestCount = ServerContext->RequestCount;
    
    for(; RequestCount > 0;
        --RequestCount)
    {
        http_io_request *IoRequest = AllocateHttpIoRequest(ServerContext);
        if(IoRequest)
        {
            Win32StartThreadpoolIo(ServerContext->IoThreadpool);
            
            Result = Win32HttpReceiveHttpRequest(ServerContext->RequestQueue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                                 IoRequest->HttpRequest, sizeof(IoRequest->Buffer), 0,
                                                 &IoRequest->IoContext.Overlapped);
            
            if((Result != ERROR_IO_PENDING) &&
               (Result != NO_ERROR))
            {
                Win32CancelThreadpoolIo(ServerContext->IoThreadpool);
                
                if(Result == ERROR_MORE_DATA)
                {
                    ProcessReceiveAndPostResponse(IoRequest, ServerContext->IoThreadpool, ERROR_MORE_DATA);
                }
                
                LogError("HttpReceiveHttpRequest failed with error code %u", Result);
                
                Result = false;
            }
        }
        else
        {
            LogError("Allocation of IoRequest %u failed", RequestCount);
            Result = false;
            break;
        }
        
    }
    
    return Result;
}

internal void
Win32StopHttpServer(server_context *ServerContext)
{
    LogVerbose("Stopping http server");
    
    if(ServerContext->RequestQueue != 0)
    {
        LogVerbose("Shutting down request queue");
        ServerContext->StopServer = true;
        u32 Result = Win32HttpShutdownRequestQueue(ServerContext->RequestQueue);
        if(Result != NO_ERROR)
        {
            LogError("HttpShutdownRequestQueue failed with error code %u", Result);
        }
    }
    
    if(ServerContext->IoThreadpool != 0)
    {
        LogVerbose("Waiting for all IO completion threads to complete");
        b32 CancelPendingCallbacks = false;
        Win32WaitForThreadpoolIoCallbacks(ServerContext->IoThreadpool, CancelPendingCallbacks);
    }
}

void __stdcall 
mainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
#if KENGINE_INTERNAL
    void *BaseAddress = (void *)Terabytes(2);
#else
    void *BaseAddress = 0;
#endif
    
    u64 StorageSize = Kilobytes(4096);
    void *Storage = Win32VirtualAlloc(BaseAddress, StorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    
    memory_arena Arena_;
    ZeroStruct(Arena_);
    memory_arena *Arena = &Arena_;
    InitializeArena(Arena, StorageSize, Storage);
    
    server_context ServerContext_;
    ZeroStruct(ServerContext_);
    server_context *ServerContext = &ServerContext_;
    //ServerContext->Arena = Arena;
    
    HANDLE ProcessHandle = Win32GetCurrentProcess();
    DWORD_PTR ProcessAffintyMask;
    DWORD_PTR SystemAffinityMask;
    u16 RequestCount;
    if(Win32GetProcessAffinityMask(ProcessHandle, &ProcessAffintyMask, &SystemAffinityMask))
    {
        for(RequestCount = 0;
            ProcessAffintyMask;
            ProcessAffintyMask >>= 1)
        {
            if(ProcessAffintyMask & 0x1)
            {
                ++RequestCount;
            }
        }
        
        RequestCount *= REQUESTS_PER_PROCESSOR;
    }
    else
    {
        LogWarning("Could not get process affinty mask");
        RequestCount = OUTSTANDING_REQUESTS;
    }
    
    LogVerbose("Set request count to %u", RequestCount);
    
    ServerContext->RequestCount = RequestCount;
    ServerContext->NextRequestIndex = 0;
    ServerContext->Requests = PushArray(Arena, RequestCount, http_io_request);
    
    ServerContext->ResponseCount = RequestCount;
    ServerContext->NextResponseIndex = 0;
    ServerContext->Responses = PushArray(Arena, RequestCount, http_io_response);
    
    u32 Result = Win32InitializeHttpServer(ServerContext);
    if(Result == NO_ERROR)
    {
        if(Win32InitializeIoThreadPool(ServerContext))
        {
            if(Win32StartHttpServer(ServerContext))
            {
                // TODO(kstandbridge): Console input?
                for(;;)
                {
                    HANDLE InputHandle = Win32GetStdHandle(STD_INPUT_HANDLE);
                    Assert(InputHandle != INVALID_HANDLE_VALUE);
                    
                    u8 Buffer[MAX_PATH];
                    umm BytesRead = 0;
                    Win32ReadFile(InputHandle, Buffer, (DWORD)sizeof(Buffer), (LPDWORD)&BytesRead, 0);
                    
                    string Input = String_(BytesRead - 2, Buffer);
                    if(StringsAreEqual(String("stop"), Input))
                    {
                        LogVerbose("Shutdown requested");
                        break;
                    }
                    else
                    {
                        LogVerbose("Unknown command: %S", Input);
                    }
                }
            }
            else
            {
                LogError("Failed to start server");
            }
        }
        else
        {
            Win32UninitializeIoCompletionContext(ServerContext);
        }
        
        Win32StopHttpServer(ServerContext);
        Win32UnitializeHttpServer(ServerContext);
        Win32UninitializeIoCompletionContext(ServerContext);
    }
    else
    {
        LogError("Failed to initialize http server");
        Win32UnitializeHttpServer(ServerContext);
    }
    
    LogVerbose("Ending telemetry thread");
    EndTelemetryThread();
    Win32ExitProcess(0);
    
    InvalidCodePath;
}
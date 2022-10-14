#include "kengine_platform.h"
#include "kengine_generated.h"
#include "win32_kengine_types.h"
#include "kengine_string.h"
#include "kengine_intrinsics.h"
#include "kengine_math.h"
#include "kengine_http.h"

#ifndef VERSION
#define VERSION 0
#endif // VERSION


#include "win32_kengine_kernel.c"
#include "win32_kengine_generated.c"
#include "win32_kengine_shared.c"
#include "kengine_telemetry.c"

typedef void http_completion_function(struct http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult);

typedef struct http_io_context
{
    OVERLAPPED Overlapped;
    http_completion_function *CompletionFunction;
    struct win32_http_state *HttpState;
    
} http_io_context;

typedef struct http_io_request
{
    b32 InUse;
    memory_arena Arena;
    
    temporary_memory MemoryFlush;
    
    http_io_context IoContext;
    
    u8 *Buffer;
    HTTP_REQUEST *HttpRequest;
} http_io_request;

typedef struct http_io_response
{
    http_io_context IoContext;
    
    HTTP_RESPONSE HttpResponse;
    
    HTTP_DATA_CHUNK HttpDataChunk;
    
} http_io_response;

typedef struct win32_http_state
{
    //memory_arena *Arena;
    
    HTTPAPI_VERSION Version;
    HTTP_SERVER_SESSION_ID SessionId;
    HTTP_URL_GROUP_ID UrlGroupId;
    HANDLE RequestQueue;
    PTP_IO IoThreadpool;
    
    b32 HttpInit;
    b32 StopServer;
    
    u32 RequestCount;
    u32 volatile NextRequestIndex;
    http_io_request *Requests;
    
    u32 ResponseCount;
    u32 volatile NextResponseIndex;
    http_io_response *Responses;
} win32_http_state;

internal void ReceiveCompletionCallback(http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);
internal void SendCompletionCallback(http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);

inline http_io_request *
SafeGetNextRequest(win32_http_state *HttpState)
{
    http_io_request *Result = 0;
    for(;;)
    {
        u32 OrigionalNext = HttpState->NextRequestIndex;
        u32 NewNext = (OrigionalNext + 1) % HttpState->RequestCount;
        u32 Index = AtomicCompareExchangeU32(&HttpState->NextRequestIndex, NewNext, OrigionalNext);
        if(Index == OrigionalNext)
        {
            Result = HttpState->Requests + Index;
            if(!Result->InUse)
            {
                Result->InUse = true;
                break;
            }
            else
            {
                LogVerbose("Request %u in use", Index);
            }
        }
        else
        {
            _mm_pause();
        }
    }
    return Result;
}

inline http_io_request *
AllocateHttpIoRequest(win32_http_state *HttpState)
{
    http_io_request *Result = SafeGetNextRequest(HttpState);
    Result->IoContext.HttpState = HttpState;
    Result->IoContext.CompletionFunction = ReceiveCompletionCallback;
    Result->MemoryFlush = BeginTemporaryMemory(&Result->Arena);
    Result->Buffer = BeginPushSize(Result->MemoryFlush.Arena);
    Result->HttpRequest = (HTTP_REQUEST *)Result->Buffer;
    
    Assert(Result->Arena.TempCount == 2);
    
    return Result;
}

inline http_io_response *
SafeGetNextResponse(win32_http_state *HttpState)
{
    http_io_response *Result = 0;
    for(;;)
    {
        u32 OrigionalNext = HttpState->NextResponseIndex;
        u32 NewNext = (OrigionalNext + 1) % HttpState->ResponseCount;
        u32 Index = AtomicCompareExchangeU32(&HttpState->NextResponseIndex, NewNext, OrigionalNext);
        if(Index == OrigionalNext)
        {
            Result = HttpState->Responses + Index;
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
AllocateHttpIoResponse(win32_http_state *HttpState)
{
    http_io_response *Result = SafeGetNextResponse(HttpState);
    Result->IoContext.HttpState = HttpState;
    Result->IoContext.CompletionFunction = SendCompletionCallback;
    
    Result->HttpResponse.EntityChunkCount = 1;
    Result->HttpResponse.pEntityChunks = &Result->HttpDataChunk;
    
    char *ContentType = "application/json";
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = ContentType;
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = GetNullTerminiatedStringLength(ContentType);
    
    return Result;
}

internal u32
Win32InitializeHttpServer(win32_http_state *HttpState)
{
    LogVerbose("Initializing HTTP server");
    
    HttpState->Version.HttpApiMajorVersion = 2;
    HttpState->Version.HttpApiMinorVersion = 0;
    
    u32 Result = Win32HttpInitialize(HttpState->Version, HTTP_INITIALIZE_SERVER, 0);
    if(Result == NO_ERROR)
    {
        HttpState->HttpInit = true;
        LogVerbose("Creating server session");
        Result = Win32HttpCreateServerSession(HttpState->Version, &HttpState->SessionId, 0);
        if(Result == NO_ERROR)
        {
            LogVerbose("Creating url group");
            Result = Win32HttpCreateUrlGroup(HttpState->SessionId, &HttpState->UrlGroupId, 0);
            if(Result == NO_ERROR)
            {
                LogVerbose("Adding url to url group");
                Result = Win32HttpAddUrlToUrlGroup(HttpState->UrlGroupId, L"http://localhost:8090/", 0, 0);
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
Win32UnitializeHttpServer(win32_http_state *HttpState)
{
    LogVerbose("Uninitializing HTTP server");
    
    if(HttpState->UrlGroupId != 0)
    {
        u32 Result = Win32HttpCloseUrlGroup(HttpState->UrlGroupId);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseUrlGroup failed with error code %u", Result);
        }
        HttpState->UrlGroupId = 0;
    }
    
    if(HttpState->SessionId != 0)
    {
        u32 Result = Win32HttpCloseServerSession(HttpState->SessionId);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseServerSession failed with error code %u", Result);
        }
        HttpState->SessionId = 0;
    }
    
    if(HttpState->HttpInit)
    {
        u32 Result = Win32HttpTerminate(HTTP_INITIALIZE_SERVER, 0);
        if(Result != NO_ERROR)
        {
            LogError("HttpTerminate failed with error code %u", Result);
        }
        HttpState->HttpInit = false;
    }
}

internal void
Win32IoCallbackThread(PTP_CALLBACK_INSTANCE Instance, PVOID Context, void *pOverlapped, ULONG IoResult, ULONG_PTR BytesTransferred, PTP_IO IoThreadpool)
{
    win32_http_state *HttpState;
    
    http_io_context *IoContext = CONTAINING_RECORD(pOverlapped, http_io_context, Overlapped);
    
    HttpState = IoContext->HttpState;
    
    IoContext->CompletionFunction(IoContext, IoThreadpool, IoResult);
    
}

internal b32
Win32InitializeIoThreadPool(win32_http_state *HttpState)
{
    LogVerbose("Initializing I/O completion context");
    
    LogVerbose("Creating request queue");
    u32 Result = Win32HttpCreateRequestQueue(HttpState->Version, L"AsyncHttpServer", 0, 0, &HttpState->RequestQueue);
    if(Result == NO_ERROR)
    {
        HTTP_BINDING_INFO HttpBindingInfo;
        ZeroStruct(HttpBindingInfo);
        HttpBindingInfo.Flags.Present = 1;
        HttpBindingInfo.RequestQueueHandle = HttpState->RequestQueue;
        LogVerbose("Setting url group property");
        Result = Win32HttpSetUrlGroupProperty(HttpState->UrlGroupId, HttpServerBindingProperty, &HttpBindingInfo, sizeof(HttpBindingInfo));
        
        if(Result == NO_ERROR)
        {
            LogVerbose("Creating I/O thread pool");
            HttpState->IoThreadpool = Win32CreateThreadpoolIo(HttpState->RequestQueue, Win32IoCallbackThread, 0, 0);
            
            if(HttpState->IoThreadpool == 0)
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
Win32UninitializeIoCompletionContext(win32_http_state *HttpState)
{
    LogVerbose("Uninitializing I/O completion context");
    
    if(HttpState->RequestQueue != 0)
    {
        LogVerbose("Closing request queue");
        u32 Result = Win32HttpCloseRequestQueue(HttpState->RequestQueue);
        if(Result != NO_ERROR)
        {
            LogError("HttpCloseRequestQueue failed with error code %u", Result);
        }
        HttpState->RequestQueue = 0;
    }
    
    if(HttpState->IoThreadpool != 0)
    {
        LogVerbose("Closing I/O thread pool");
        Win32CloseThreadpoolIo(HttpState->IoThreadpool);
        HttpState->IoThreadpool = 0;
    }
}

inline http_io_response *
CreateMessageResponse(win32_http_state *HttpState, u16 StatusCode, string Reason, string Message)
{
    http_io_response *Result = AllocateHttpIoResponse(HttpState);
    
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

typedef struct platform_http_request
{
    string Endpoint;
    string Payload;
} platform_http_request;

typedef struct platform_http_response
{
    u16 Code;
    string Payload;
} platform_http_response;


internal platform_http_response
HandleHttpRequest(
                  //app_state *AppState, 
                  memory_arena *TranArena, platform_http_request Request)
{
    platform_http_response Result;
    
    if(StringsAreEqual(String("/echo"), Request.Endpoint))
    {
        Result.Code = 200;
        if(Request.Payload.Size == 0)
        {
            Result.Payload = String("Hello, world!");
        }
        else
        {
            Result.Payload = FormatString(TranArena, "%S", Request.Payload);
        }
    }
    else
    {
        Result.Code = 404;
        Result.Payload = String("Not Found");
    }
    
    return Result;
}

internal void
ProcessReceiveAndPostResponse(http_io_request *IoRequest, PTP_IO IoThreadpool, u32 IoResult)
{
    Assert(IoRequest->Arena.TempCount == 2);
    win32_http_state *HttpState = IoRequest->IoContext.HttpState;
    http_io_response *IoResponse = 0;
    
    switch(IoResult)
    {
        case NO_ERROR:
        {
            HTTP_REQUEST *Request = IoRequest->HttpRequest;
            umm TotalSize = sizeof(HTTP_REQUEST);
            TotalSize += Request->UnknownVerbLength;
            TotalSize += Request->RawUrlLength;
            TotalSize += Request->CookedUrl.FullUrlLength;
            for(u32 HeaderIndex = 0;
                HeaderIndex < Request->Headers.UnknownHeaderCount;
                ++HeaderIndex)
            {
                TotalSize += sizeof(HTTP_UNKNOWN_HEADER);
                HTTP_UNKNOWN_HEADER *Header = Request->Headers.pUnknownHeaders + HeaderIndex;
                TotalSize += Header->NameLength;
                TotalSize += Header->RawValueLength;
            }
            for(u32 HeaderIndex = 0;
                HeaderIndex < ArrayCount(Request->Headers.KnownHeaders);
                ++HeaderIndex)
            {
                TotalSize += sizeof(HTTP_KNOWN_HEADER);
                HTTP_KNOWN_HEADER *Header = Request->Headers.KnownHeaders + HeaderIndex;
                TotalSize += Header->RawValueLength;
                
            }
            for(u32 EntityChunkIndex = 0;
                EntityChunkIndex < Request->EntityChunkCount;
                ++EntityChunkIndex)
            {
                TotalSize += sizeof(HTTP_DATA_CHUNK);
                HTTP_DATA_CHUNK *DataChunk = Request->pEntityChunks + EntityChunkIndex;
                // TODO(kstandbridge): Other entitiy chunk types?
                Assert(DataChunk->DataChunkType == HttpDataChunkFromMemory);
                TotalSize += DataChunk->FromMemory.BufferLength;
            }
            for(u32 RequestInfoIndex = 0;
                RequestInfoIndex < Request->RequestInfoCount;
                ++RequestInfoIndex)
            {
                TotalSize += sizeof(HTTP_REQUEST_INFO);
                HTTP_REQUEST_INFO *RequestInfo = Request->pRequestInfo + RequestInfoIndex;
                TotalSize += RequestInfo->InfoLength;
            }
            EndPushSize(IoRequest->MemoryFlush.Arena, TotalSize);
            Assert(IoRequest->Arena.TempCount == 1);
            if(Request->Verb == HttpVerbGET)
            {
                
                platform_http_request PlatformRequest;
                PlatformRequest.Endpoint = String_(Request->RawUrlLength,
                                                   (u8 *)Request->pRawUrl);
                PlatformRequest.Payload = (Request->EntityChunkCount == 0) 
                    ? String("") : String_(Request->pEntityChunks->FromMemory.BufferLength, 
                                           Request->pEntityChunks->FromMemory.pBuffer);
                platform_http_response PlatformResponse = HandleHttpRequest(&IoRequest->Arena,
                                                                            PlatformRequest);
                IoResponse = CreateMessageResponse(HttpState, 
                                                   PlatformResponse.Code, 
                                                   String("OK"),  // TODO(kstandbridge): Code to string?
                                                   PlatformResponse.Payload);
            }
            else
            {
                IoResponse = CreateMessageResponse(HttpState, 501, String("Not Implemented"), 
                                                   String("{ \"Error\": \"Not Implemented\" }"));
            }
        } break;
        
        case ERROR_MORE_DATA:
        {
            EndPushSize(IoRequest->MemoryFlush.Arena, 0);
            IoResponse = CreateMessageResponse(HttpState, 413, String("Payload Too Large"), 
                                               String("{ \"Error\": \"The request is larger than the server is willing or able to process\" }"));
        } break;
        
        default:
        {
            EndPushSize(IoRequest->MemoryFlush.Arena, 0);
            LogError("HttpReceiveHttpRequest call failed asynchronously with error code %u", IoResult);
        } break;
    }
    
    if(IoResponse != 0)
    {
        Win32StartThreadpoolIo(IoThreadpool);
        
        u32 Result = Win32HttpSendHttpResponse(HttpState->RequestQueue, IoRequest->HttpRequest->RequestId, 0, &IoResponse->HttpResponse,
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
PostNewReceive(win32_http_state *HttpState, PTP_IO IoThreadpool)
{
    http_io_request *IoRequest = AllocateHttpIoRequest(HttpState);
    Assert(IoRequest->Arena.TempCount == 2);
    
    if(IoRequest != 0)
    {
        Win32StartThreadpoolIo(IoThreadpool);
        
        umm BufferMaxSize = (IoRequest->Arena.Size - IoRequest->Arena.Used);
        u32 Result = Win32HttpReceiveHttpRequest(HttpState->RequestQueue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                                 IoRequest->HttpRequest, BufferMaxSize, 0,
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
            
            EndPushSize(&IoRequest->Arena, 0);
            Assert(IoRequest->Arena.TempCount == 1);
            EndTemporaryMemory(IoRequest->MemoryFlush);
            Assert(IoRequest->Arena.TempCount == 0);
            CheckArena(&IoRequest->Arena);
            CompletePreviousWritesBeforeFutureWrites;
            IoRequest->InUse = false;
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
    Assert(IoRequest->Arena.TempCount == 2);
    
    win32_http_state *HttpState = IoRequest->IoContext.HttpState;
    
    if(HttpState->StopServer == false)
    {
        ProcessReceiveAndPostResponse(IoRequest, IoThreadpool, IoResult);
        
        PostNewReceive(HttpState, IoThreadpool);
    }
    else
    {
        EndPushSize(&IoRequest->Arena, 0);
    }
    
    Assert(IoRequest->Arena.TempCount == 1);
    EndTemporaryMemory(IoRequest->MemoryFlush);
    Assert(IoRequest->Arena.TempCount == 0);
    CheckArena(&IoRequest->Arena);
    CompletePreviousWritesBeforeFutureWrites;
    IoRequest->InUse = false;
}

internal b32
Win32StartHttpServer(win32_http_state *HttpState)
{
    b32 Result = true;
    
    u32 RequestCount = 0.75f*HttpState->RequestCount;
    
    for(; RequestCount > 0;
        --RequestCount)
    {
        http_io_request *IoRequest = AllocateHttpIoRequest(HttpState);
        if(IoRequest)
        {
            Assert(IoRequest->Arena.TempCount == 2);
            Win32StartThreadpoolIo(HttpState->IoThreadpool);
            
            umm BufferMaxSize = (IoRequest->Arena.Size - IoRequest->Arena.Used);
            Result = Win32HttpReceiveHttpRequest(HttpState->RequestQueue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                                 IoRequest->HttpRequest, BufferMaxSize, 0,
                                                 &IoRequest->IoContext.Overlapped);
            
            if((Result != ERROR_IO_PENDING) &&
               (Result != NO_ERROR))
            {
                Win32CancelThreadpoolIo(HttpState->IoThreadpool);
                
                if(Result == ERROR_MORE_DATA)
                {
                    ProcessReceiveAndPostResponse(IoRequest, HttpState->IoThreadpool, ERROR_MORE_DATA);
                }
                
                Assert(IoRequest->Arena.TempCount == 1);
                EndTemporaryMemory(IoRequest->MemoryFlush);
                Assert(IoRequest->Arena.TempCount == 0);
                CheckArena(&IoRequest->Arena);
                CompletePreviousWritesBeforeFutureWrites;
                IoRequest->InUse = false;
                
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
Win32StopHttpServer(win32_http_state *HttpState)
{
    LogVerbose("Stopping http server");
    
    if(HttpState->RequestQueue != 0)
    {
        LogVerbose("Shutting down request queue");
        HttpState->StopServer = true;
        u32 Result = Win32HttpShutdownRequestQueue(HttpState->RequestQueue);
        if(Result != NO_ERROR)
        {
            LogError("HttpShutdownRequestQueue failed with error code %u", Result);
        }
    }
    
    if(HttpState->IoThreadpool != 0)
    {
        LogVerbose("Waiting for all IO completion threads to complete");
        b32 CancelPendingCallbacks = false;
        Win32WaitForThreadpoolIoCallbacks(HttpState->IoThreadpool, CancelPendingCallbacks);
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
    
#define OUTSTANDING_REQUESTS 16
#define REQUESTS_PER_PROCESSOR 4
    
    u16 RequestCount;
    
    HANDLE ProcessHandle = Win32GetCurrentProcess();
    DWORD_PTR ProcessAffintyMask;
    DWORD_PTR SystemAffinityMask;
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
    
#define REQUEST_ARENA_SIZE 128
#define ARENA_SIZE 4096
    
    u64 StorageSize = Kilobytes(ARENA_SIZE);
    StorageSize += (Kilobytes(REQUEST_ARENA_SIZE) * RequestCount);
    LogVerbose("Allocating %u bytes", StorageSize);
    void *Storage = Win32VirtualAlloc(BaseAddress, StorageSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    
    memory_arena Arena_;
    ZeroStruct(Arena_);
    memory_arena *Arena = &Arena_;
    InitializeArena(Arena, StorageSize, Storage);
    
    win32_http_state HttpState_;
    ZeroStruct(HttpState_);
    win32_http_state *HttpState = &HttpState_;
    
    HttpState->ResponseCount = RequestCount;
    HttpState->NextResponseIndex = 0;
    HttpState->Responses = PushArray(Arena, RequestCount, http_io_response);
    
    HttpState->RequestCount = RequestCount;
    HttpState->NextRequestIndex = 0;
    HttpState->Requests = PushArray(Arena, RequestCount, http_io_request);
    
    for(u32 RequestIndex = 0;
        RequestIndex < RequestCount;
        ++RequestIndex)
    {
        http_io_request *Request = HttpState->Requests + RequestIndex;
        umm Size = Kilobytes(REQUEST_ARENA_SIZE);
        SubArena(&Request->Arena, Arena, Size);
    }
    
    u32 Result = Win32InitializeHttpServer(HttpState);
    if(Result == NO_ERROR)
    {
        if(Win32InitializeIoThreadPool(HttpState))
        {
            if(Win32StartHttpServer(HttpState))
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
            Win32UninitializeIoCompletionContext(HttpState);
        }
        
        Win32StopHttpServer(HttpState);
        Win32UnitializeHttpServer(HttpState);
        Win32UninitializeIoCompletionContext(HttpState);
    }
    else
    {
        LogError("Failed to initialize http server");
        Win32UnitializeHttpServer(HttpState);
    }
    
    LogVerbose("Ending telemetry thread");
    EndTelemetryThread();
    Win32ExitProcess(0);
    
    InvalidCodePath;
}
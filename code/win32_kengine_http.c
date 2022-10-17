typedef void win32_http_completion_function(struct win32_http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult);

typedef struct win32_http_io_context
{
    OVERLAPPED Overlapped;
    win32_http_completion_function *CompletionFunction;
    struct win32_http_state *HttpState;
    
} win32_http_io_context;

typedef struct win32_http_io_request
{
    b32 InUse;
    memory_arena Arena;
    
    temporary_memory MemoryFlush;
    
    win32_http_io_context IoContext;
    
    u8 *Buffer;
    HTTP_REQUEST *HttpRequest;
} win32_http_io_request;

typedef struct win32_http_io_response
{
    win32_http_io_context IoContext;
    
    HTTP_RESPONSE HttpResponse;
    
    HTTP_DATA_CHUNK HttpDataChunk;
    
} win32_http_io_response;

typedef struct win32_http_state
{
    HTTPAPI_VERSION Version;
    HTTP_SERVER_SESSION_ID SessionId;
    HTTP_URL_GROUP_ID UrlGroupId;
    HANDLE RequestQueue;
    PTP_IO IoThreadpool;
    
    b32 HttpInit;
    b32 StopServer;
    
    u32 RequestCount;
    u32 volatile NextRequestIndex;
    win32_http_io_request *Requests;
    
    u32 ResponseCount;
    u32 volatile NextResponseIndex;
    win32_http_io_response *Responses;
} win32_http_state;


internal void Win32ReceiveCompletionCallback(win32_http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);
internal void Win32SendCompletionCallback(win32_http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);

inline win32_http_io_request *
Win32SafeGetNextRequest(win32_http_state *HttpState)
{
    win32_http_io_request *Result = 0;
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
        }
        else
        {
            _mm_pause();
        }
    }
    return Result;
}

inline win32_http_io_request *
Win32AllocateHttpIoRequest(win32_http_state *HttpState)
{
    win32_http_io_request *Result = Win32SafeGetNextRequest(HttpState);
    Result->IoContext.HttpState = HttpState;
    Result->IoContext.CompletionFunction = Win32ReceiveCompletionCallback;
    Result->MemoryFlush = BeginTemporaryMemory(&Result->Arena);
    Result->Buffer = BeginPushSize(Result->MemoryFlush.Arena);
    Result->HttpRequest = (HTTP_REQUEST *)Result->Buffer;
    
    Assert(Result->Arena.TempCount == 2);
    
    return Result;
}

inline win32_http_io_response *
Win32SafeGetNextResponse(win32_http_state *HttpState)
{
    win32_http_io_response *Result = 0;
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
Win32SendCompletionCallback(win32_http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult)
{
    win32_http_io_response *IoResponse = CONTAINING_RECORD(pIoContext, win32_http_io_response, IoContext);
}

inline win32_http_io_response *
Win32AllocateHttpIoResponse(win32_http_state *HttpState)
{
    win32_http_io_response *Result = Win32SafeGetNextResponse(HttpState);
    Result->IoContext.HttpState = HttpState;
    Result->IoContext.CompletionFunction = Win32SendCompletionCallback;
    
    Result->HttpResponse.EntityChunkCount = 1;
    Result->HttpResponse.pEntityChunks = &Result->HttpDataChunk;
    
    char *ContentType = "application/json";
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].pRawValue = ContentType;
    Result->HttpResponse.Headers.KnownHeaders[HttpHeaderContentType].RawValueLength = GetNullTerminiatedStringLength(ContentType);
    
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
    
    win32_http_io_context *IoContext = CONTAINING_RECORD(pOverlapped, win32_http_io_context, Overlapped);
    
    HttpState = IoContext->HttpState;
    
    IoContext->CompletionFunction(IoContext, IoThreadpool, IoResult);
    
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

inline win32_http_io_response *
Win32CreateMessageResponse(win32_http_state *HttpState, u16 StatusCode, string Reason, string Message)
{
    win32_http_io_response *Result = Win32AllocateHttpIoResponse(HttpState);
    
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
Win32ProcessReceiveAndPostResponse(win32_http_io_request *IoRequest, PTP_IO IoThreadpool, u32 IoResult)
{
    Assert(IoRequest->Arena.TempCount == 2);
    win32_http_state *HttpState = IoRequest->IoContext.HttpState;
    win32_http_io_response *IoResponse = 0;
    
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
            if(GlobalWin32State.AppHandleHttpRequest)
            {
                if(Request->Verb == HttpVerbGET)
                {
                    
                    
                    platform_http_request PlatformRequest;
                    PlatformRequest.Endpoint = String_(Request->RawUrlLength,
                                                       (u8 *)Request->pRawUrl);
                    PlatformRequest.Payload = (Request->EntityChunkCount == 0) 
                        ? String("") : String_(Request->pEntityChunks->FromMemory.BufferLength, 
                                               Request->pEntityChunks->FromMemory.pBuffer);
                    platform_http_response PlatformResponse = GlobalWin32State.AppHandleHttpRequest(&GlobalAppMemory, &IoRequest->Arena,
                                                                                                    PlatformRequest);
                    IoResponse = Win32CreateMessageResponse(HttpState, 
                                                            PlatformResponse.Code, 
                                                            String("OK"),  // TODO(kstandbridge): Code to string?
                                                            PlatformResponse.Payload);
                }
                else
                {
                    IoResponse = Win32CreateMessageResponse(HttpState, 501, String("Not Implemented"), 
                                                            String("{ \"Error\": \"Not Implemented\" }"));
                }
            }
            else
            {
                IoResponse = Win32CreateMessageResponse(HttpState, 200, String("OK"), 
                                                        String("{ \"Message\": \"Server starting...\" }"));
            }
        } break;
        
        case ERROR_MORE_DATA:
        {
            EndPushSize(IoRequest->MemoryFlush.Arena, 0);
            IoResponse = Win32CreateMessageResponse(HttpState, 413, String("Payload Too Large"), 
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
Win32PostNewReceive(win32_http_state *HttpState, PTP_IO IoThreadpool)
{
    win32_http_io_request *IoRequest = Win32AllocateHttpIoRequest(HttpState);
    Assert(IoRequest->Arena.TempCount == 2);
    
    if(IoRequest != 0)
    {
        Win32StartThreadpoolIo(IoThreadpool);
        
        umm BufferMaxSize = (IoRequest->Arena.CurrentBlock->Size - IoRequest->Arena.CurrentBlock->Used);
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
                Win32ProcessReceiveAndPostResponse(IoRequest, IoThreadpool, ERROR_MORE_DATA);
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
Win32ReceiveCompletionCallback(win32_http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult)
{
    win32_http_io_request *IoRequest = CONTAINING_RECORD(pIoContext, win32_http_io_request, IoContext);
    Assert(IoRequest->Arena.TempCount == 2);
    
    win32_http_state *HttpState = IoRequest->IoContext.HttpState;
    
    if(HttpState->StopServer == false)
    {
        Win32ProcessReceiveAndPostResponse(IoRequest, IoThreadpool, IoResult);
        
        Win32PostNewReceive(HttpState, IoThreadpool);
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
        win32_http_io_request *IoRequest = Win32AllocateHttpIoRequest(HttpState);
        if(IoRequest)
        {
            Assert(IoRequest->Arena.TempCount == 2);
            Win32StartThreadpoolIo(HttpState->IoThreadpool);
            
            umm BufferMaxSize = (IoRequest->Arena.CurrentBlock->Size - IoRequest->Arena.CurrentBlock->Used);
            Result = Win32HttpReceiveHttpRequest(HttpState->RequestQueue, HTTP_NULL_ID, HTTP_RECEIVE_REQUEST_FLAG_COPY_BODY,
                                                 IoRequest->HttpRequest, BufferMaxSize, 0,
                                                 &IoRequest->IoContext.Overlapped);
            
            if((Result != ERROR_IO_PENDING) &&
               (Result != NO_ERROR))
            {
                Win32CancelThreadpoolIo(HttpState->IoThreadpool);
                
                if(Result == ERROR_MORE_DATA)
                {
                    Win32ProcessReceiveAndPostResponse(IoRequest, HttpState->IoThreadpool, ERROR_MORE_DATA);
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
internal u32
Win32InitializeHttpServer_(win32_http_state *HttpState)
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

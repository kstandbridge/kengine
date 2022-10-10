#ifndef WIN32_KENGINE_HTTP_H


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

typedef void http_completion_function(struct http_io_context *pIoContext, PTP_IO IoThreadpool, u32 IoResult);

// TODO(kstandbridge): Rename http_io_state?
typedef struct http_io_context
{
    OVERLAPPED Overlapped;
    http_completion_function *CompletionFunction;
    struct server_context *ServerContext;
    
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

// TODO(kstandbridge): Should this be app state?
typedef struct server_context
{
    //memory_arena *Arena;
    
    HTTPAPI_VERSION Version;
    HTTP_SERVER_SESSION_ID SessionId;
    HTTP_URL_GROUP_ID UrlGroupId;
    HANDLE RequestQueue;
    PTP_IO IoThreadpool;
    // TODO(kstandbridge): Rename to is initalized?
    b32 HttpInit;
    b32 StopServer;
    
    u32 RequestCount;
    u32 volatile NextRequestIndex;
    http_io_request *Requests;
    
    u32 ResponseCount;
    u32 volatile NextResponseIndex;
    http_io_response *Responses;
} server_context;

internal void ReceiveCompletionCallback(http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);
internal void SendCompletionCallback(http_io_context *IoContext, PTP_IO IoThreadpool, u32 IoResult);

#define WIN32_KENGINE_HTTP_H
#endif //WIN32_KENGINE_HTTP_H

#ifndef KENGINE_HTTP_H

typedef struct http_client
{
    string Hostname;
    
    u32 Socket;
    void *Creds;
    void *Context;
    
} http_client;

typedef enum http_method_type
{
    HttpMethod_Unknown,
    
    HttpMethod_GET,
} http_method_type;

inline string
HttpMethodToString(http_method_type Method)
{
    string Result;
    ZeroStruct(Result);
    
    switch(Method)
    {
        case HttpMethod_GET: { Result = String("GET"); } break;
        
        case HttpMethod_Unknown:
        InvalidDefaultCase;
    }
    
    return Result;
}

typedef enum http_accept_type
{
    HttpAccept_Unknown, 
    
    HttpAccept_Any
} http_accept_type;

inline string
HttpAcceptToString(http_accept_type Accept)
{
    string Result;
    ZeroStruct(Result);
    
    switch(Accept)
    {
        case HttpAccept_Any: { Result = String("*/*"); } break;
        
        case HttpAccept_Unknown:
        InvalidDefaultCase;
    }
    
    return Result;
}

typedef struct http_request
{
    http_client *Client;
    http_method_type Method;
    string Endpoint;
    http_accept_type Accept;
    
    string Raw;
    
} http_request;

typedef enum http_response_code_type
{
    HttpResponseCode_SocketError = -1,
    
    HttpResponseCode_NotFound = 404,
} http_response_code_type;

typedef enum http_response_content_type
{
    HttpResponseContent_Unknown,
    
    HttpResponseContent_HTML,
} http_response_content_type;

typedef enum http_transfer_encoding_type
{
    HttpTransferEncoding_Unknown,
    
    HttpTransferEncoding_Chunked,
} http_transfer_encoding_type;

typedef struct http_response
{
    u8 Major;
    u8 Minor;
    
    // TODO(kstandbridge): Make this u16?
    http_response_code_type Code;
    string Message;
    
    http_transfer_encoding_type TransferEncoding;
    
    http_response_content_type ContentType;
    u32 ContentLength;
    string Content;
    
} http_response;

#define KENGINE_HTTP_H
#endif //KENGINE_HTTP_H

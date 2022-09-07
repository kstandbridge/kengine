#ifndef KENGINE_HTTP_H

typedef struct http_client
{
    string Hostname;
    
    b32 IsSsl;
    u32 Socket;
    void *Creds;
    void *Context;
    
} http_client;

typedef enum http_method_type
{
    HttpMethod_GET,
} http_method_type;

typedef enum http_accept_type
{
    HttpAccept_Any
} http_accept_type;

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

typedef struct http_response
{
    u8 Major;
    u8 Minor;
    
    // TODO(kstandbridge): Make this u16?
    http_response_code_type Code;
    string Message;
    
    http_response_content_type ContentType;
    u32 ContentLength;
    string Content;
    
} http_response;

inline string
GenerateHttpMessage_(http_request *Request)
{
    // TODO(kstandbridge): Actually generate the raw message
    string Result = String("GET /generic/upi/firebird/ HTTP/1.1\r\nHost: artifactory.ubisoft.org\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36\r\nAccept: */*\r\n\r\n");
    
    return Result;
}


#define KENGINE_HTTP_H
#endif //KENGINE_HTTP_H

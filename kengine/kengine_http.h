#ifndef KENGINE_HTTP_H

typedef enum http_verb_type
{
    HttpVerb_Post,
    HttpVerb_Get,
    HttpVerb_Put,
    HttpVerb_Patch,
    HttpVerb_Delete,
} http_verb_type;

typedef struct platform_http_request
{
    void *Handle;
    b32 NoErrors;
    string Payload;
    
    string Hostname;
    u32 Port;
    string Verb;
    string Template;
    string Endpoint;
    umm RequestLength;
    umm ResponseLength;
    u32 StatusCode;
    
} platform_http_request;

typedef struct platform_http_response
{
    u16 StatusCode;
    string Payload;
} platform_http_response;

typedef struct platform_http_client
{
    void *Handle;
    
    string Hostname;
    u32 Port;
    b32 IsHttps;
    b32 NoErrors;
} platform_http_client;

#define PlatformNoErrors(Handle) ((Handle).NoErrors)


#define KENGINE_HTTP_H
#endif //KENGINE_HTTP_H

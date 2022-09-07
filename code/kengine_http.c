// TODO(kstandbridge): there should be NO win32 in this file, replace with platform invocations

typedef struct http_client
{
    string Hostname;
    
    u32 Socket;
    
} http_client;

inline http_client
BeginHttpClient(string Hostname)
{
    http_client Result;
    ZeroStruct(Result);
    
    Result.Hostname = Hostname;
    Result.Socket = Win32SocketConnect(Hostname);
    
    return Result;
}

inline void
EndHttpClient(http_client *Client)
{
    Win32SocketClose(Client->Socket);
}

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

inline http_request
BeginHttpRequest(http_client *Client, http_method_type Method, string Endpoint, http_accept_type Accept)
{
    http_request Result;
    ZeroStruct(Result);
    
    Result.Client = Client;
    Result.Method = Method;
    Result.Endpoint;
    Result.Accept;
    
    return Result;
}

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

inline http_response
EndHttpRequest(http_request *Request, memory_arena *Arena)
{
    http_response Result;
    ZeroStruct(Result);
    
    http_client *Client = Request->Client;
    
    Request->Raw = GenerateHttpMessage_(Request);
    
    if(Win32SocketSendData(Client->Socket, Request->Raw.Data, Request->Raw.Size) >= 0)
    {
        // TODO(kstandbridge): Make buffer larger, this is just stress testing
        char Buffer[64];
        u32 BufferSize = sizeof(Buffer);
        s32 At = 0;
        for(;;)
        {
            s32 Recieved = Win32Recv(Client->Socket, Buffer + At, 1, 0);
            if(Recieved == -1)
            {
                // TODO(kstandbridge): Socket error
                Result.Code = HttpResponseCode_SocketError;
                break;
            }
            else if(Recieved == 0)
            {
                // TODO(kstandbridge): Server closed socket
                Result.Code = HttpResponseCode_SocketError;
                break;
            }
            else
            {
                if((Buffer[At - 1] == '\r') &&
                   (Buffer[At] == '\n'))
                {
                    if((Buffer[0] == '\r') &&
                       (Buffer[1] == '\n'))
                    {
                        // NOTE(kstandbridge): Header is finished.
                        u32 LengthRemaining = Result.ContentLength;
                        format_string_state StringState = BeginFormatString(Arena);
                        while(LengthRemaining > 0)
                        {
                            u32 LengthToGet = (LengthRemaining > BufferSize) ? BufferSize : LengthRemaining;
                            Recieved = Win32Recv(Client->Socket, Buffer, LengthToGet, 0);
                            if(Recieved == -1)
                            {
                                // TODO(kstandbridge): Socket error
                                Result.Code = HttpResponseCode_SocketError;
                                break;
                            }
                            else if(Recieved == 0)
                            {
                                // TODO(kstandbridge): Server closed socket
                                Result.Code = HttpResponseCode_SocketError;
                                break;
                            }
                            else
                            {
                                Buffer[Recieved] = '\0';
                                AppendStringFormat(&StringState, "%s", Buffer);
                                LengthRemaining-= Recieved;
                            }
                        }
                        Result.Content = EndFormatString(&StringState);
                        break;
                    }
                    else
                    {
                        Buffer[At - 1] = '\0';
                        // NOTE(kstandbridge): We recieved a line
                        if(StringBeginsWith(String_(At, (u8 *)Buffer), String("HTTP/")))
                        {
                            string ResponseMessage;
                            ZeroStruct(ResponseMessage);
                            ParseFromString(Buffer, "HTTP/%d.%d %d %S",
                                            &Result.Major, &Result.Minor, &Result.Code, &ResponseMessage);
                            
                            Result.Message = PushString_(Arena, ResponseMessage.Size, ResponseMessage.Data);
                        }
                        else if(StringBeginsWith(String_(At, (u8 *)Buffer), String("Content-Type")))
                        {
                            string ContentType = String_(At - 15, (u8 *)(Buffer + 14));
                            if(StringsAreEqual(ContentType, String("text/html")))
                            {
                                Result.ContentType = HttpResponseContent_HTML;
                            }
                            else
                            {
                                InvalidCodePath;
                            }
                        }
                        else if(StringBeginsWith(String_(At, (u8 *)Buffer), String("Content-Length")))
                        {
                            Result.ContentLength = U32FromString(Buffer + 16);
                        }
                        else
                        {
                            // TODO(kstandbridge): Process this line
                            int x = 5;
                        }
                    }
                    At = 0;
                }
                else
                {
                    ++At;
                }
            }
        }
    }
    else
    {
        Result.Code = HttpResponseCode_SocketError;
    }
    
    
    return Result;
}


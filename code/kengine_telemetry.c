typedef enum telemetry_field_type
{
    TelemetryField_String,
    TelemetryField_Number
} telemetry_field_type;

typedef struct telemetry_field
{
    struct telemetry_field *Next;
    
    telemetry_field_type Type;
    
    string Key;
    union 
    {
        string StringValue;
        f64 NumberValue;
    };
    
} telemetry_field;

typedef struct telemetry_message
{
    struct telemetry_message *Next;
    
    u64 TimeStamp;
    
    telemetry_field *Fields;
} telemetry_message;

typedef enum telemetry_state_type
{
    TelemetryState_Uninitialized,
    TelemetryState_Initializing,
    TelemetryState_Idle,
    TelemetryState_AddingHost,
    TelemetryState_AddingToQueue,
    TelemetryState_SwappingQueues
} telemetry_state_type;

typedef struct telemetry_host
{
    struct telemetry_host *Next;
    string Hostname;
} telemetry_host;

typedef struct telemetry_queue
{
    memory_arena Arena;
    temporary_memory MemoryFlush;
    
    telemetry_message *Messages;
} telemetry_queue;

typedef struct telemetry_state
{
    telemetry_state_type CurrentState;
    
    telemetry_host *Hosts;
    
    string TeamId;
    string ProductName;
    string MachineName;
    string Username;
    u32 ProcessId;
    
    telemetry_queue *AddingQueue;
    telemetry_queue *ProcessingQueue;
    
} telemetry_state;

global telemetry_state GlobalTelemetryState__;
telemetry_state *GlobalTelemetryState = &GlobalTelemetryState__;

internal void
PostTelemetryThread(void *Data)
{
    Data;
    
    // NOTE(kstandbridge): Swap queues
    if((GlobalTelemetryState->CurrentState != TelemetryState_Uninitialized) &&
       (GlobalTelemetryState->CurrentState != TelemetryState_Initializing))
    {
        for(;;)
        {        
            u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState->CurrentState, TelemetryState_SwappingQueues, TelemetryState_Idle);
            if(StateType == TelemetryState_Idle)
            {
                telemetry_queue *Temp = GlobalTelemetryState->AddingQueue;
                GlobalTelemetryState->AddingQueue = GlobalTelemetryState->ProcessingQueue;
                GlobalTelemetryState->ProcessingQueue = Temp;
                
                CompletePreviousWritesBeforeFutureWrites;
                GlobalTelemetryState->CurrentState = TelemetryState_Idle;
                
                telemetry_queue *Queue = GlobalTelemetryState->ProcessingQueue;
                
                // NOTE(kstandbridge): Post all telemetry messages
                if(GlobalTelemetryState->ProcessingQueue->Messages)
                {
                    for(telemetry_host *Host = GlobalTelemetryState->Hosts;
                        Host;
                        Host = Host->Next)
                    {
                        platform_http_client Client = Platform.BeginHttpClient(Host->Hostname, 0);
                        Platform.SkipHttpMetrics(&Client);
                        Platform.SetHttpClientTimeout(&Client, 3000);
                        
                        u64 LastTimeStamp = U64Max;
                        for(telemetry_message *Message = Queue->Messages;
                            Message;
                            Message = Message->Next)
                            
                        {
                            // NOTE(kstandbridge): We don't want two telemtry messages to have the same timestamp as the index will be the same
                            // Linked list adds messages to the head so they are technically in reserve, so we subtract time not add
                            u64 NewTimeStamp = Message->TimeStamp;
                            while(NewTimeStamp >= LastTimeStamp)
                            {
                                NewTimeStamp -= 1000;
                            }
                            LastTimeStamp = NewTimeStamp;
                            date_time DateTime = Platform.GetDateTimeFromTimestamp(LastTimeStamp);
                            
                            platform_http_request Request = Platform.BeginHttpRequest(&Client, HttpVerb_Post, 
                                                                                      "/%S_%S_%d-%02d/_doc/%S_%S_%d_%d%02d%02d%02d%02d%02d%03d?pretty",
                                                                                      GlobalTelemetryState->TeamId,
                                                                                      GlobalTelemetryState->ProductName,
                                                                                      DateTime.Year, DateTime.Month,
                                                                                      GlobalTelemetryState->ProductName,
                                                                                      GlobalTelemetryState->MachineName,
                                                                                      GlobalTelemetryState->ProcessId,
                                                                                      DateTime.Year, DateTime.Month, DateTime.Day,
                                                                                      DateTime.Hour, DateTime.Minute, DateTime.Second,
                                                                                      DateTime.Milliseconds);
                            Platform.SetHttpRequestHeaders(&Request, String("Content-Type: application/json;\r\n"));
                            if(PlatformNoErrors(Request))
                            {                                
                                u8 CPayload[4096];
                                format_string_state StringState = BeginFormatString();
                                
                                AppendFormatString(&StringState, "{\n    \"@timestamp\": \"%d-%02d-%02dT%02d:%02d:%02d.%03dZ\"",
                                                   DateTime.Year, DateTime.Month, DateTime.Day,
                                                   DateTime.Hour, DateTime.Minute, DateTime.Second,
                                                   DateTime.Milliseconds);
                                
                                for(telemetry_field *Field = Message->Fields;
                                    Field;
                                    Field = Field->Next)
                                {
                                    if((Field->Key.Data) && 
                                       (Field->Key.Data != '\0'))
                                    {
                                        if(Field->Type == TelemetryField_Number)
                                        {
                                            AppendFormatString(&StringState, ",\n    \"%S\": %.03f", Field->Key, Field->NumberValue);
                                        }
                                        else
                                        {
                                            Assert(Field->Type == TelemetryField_String);
                                            AppendFormatString(&StringState, ",\n    \"%S\": \"%S\"", Field->Key, Field->StringValue);
                                        }
                                    }
                                    else
                                    {
                                        Assert(!"Blank key");
                                    }
                                }
                                
                                AppendFormatString(&StringState, ",\n    \"product_name\": \"%S\"", GlobalTelemetryState->ProductName);
                                AppendFormatString(&StringState, ",\n    \"machine_name\": \"%S\"", GlobalTelemetryState->MachineName);
                                AppendFormatString(&StringState, ",\n    \"user_name\": \"%S\"", GlobalTelemetryState->Username);
                                AppendFormatString(&StringState, ",\n    \"pid\": %.03f", GlobalTelemetryState->ProcessId);
                                
                                AppendFormatString(&StringState, "\n}");
                                
                                Request.Payload = EndFormatStringToBuffer(&StringState, CPayload, sizeof(CPayload));
                                
                                u32 StatusCode = Platform.SendHttpRequest(&Request);
                                if(StatusCode != 201)
                                {
                                    // NOTE(kstandbridge): Any problems with the host we skip.
                                    break;
                                }
#if KENGINE_INTERNAL
                                string Response = Platform.GetHttpResponse(&Request);
                                Response;
#endif
                            }
                            Platform.EndHttpRequest(&Request);
                        }
                        
                        
                        Platform.EndHttpClient(&Client);
                    }
                }
                
                EndTemporaryMemory(Queue->MemoryFlush);
                CheckArena(&Queue->Arena);
                
                Queue->MemoryFlush = BeginTemporaryMemory(&Queue->Arena);
                Queue->Messages = 0;
                break;
            }
            else
            {
                _mm_pause();
            }
        }
    }
    
}

internal void
BeginTelemetryMessage()
{
    for(;;)
    {
        u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState->CurrentState, TelemetryState_AddingToQueue, TelemetryState_Idle);
        if(StateType == TelemetryState_Idle)
        {
            telemetry_message *Message = GlobalTelemetryState->AddingQueue->Messages;
            if(Message != 0)
            {
                Message = PushStruct(&GlobalTelemetryState->AddingQueue->Arena, telemetry_message);
                Message->Next = GlobalTelemetryState->AddingQueue->Messages;
            }
            else
            {
                Message = PushStruct(&GlobalTelemetryState->AddingQueue->Arena, telemetry_message);
            }
            GlobalTelemetryState->AddingQueue->Messages = Message;
            
            break;
        }
        else
        {
            _mm_pause();
        }
    }
}

internal void
AppendTelemetryMessageNumberField(string Key, f32 Value)
{
    Assert(GlobalTelemetryState->CurrentState == TelemetryState_AddingToQueue);
    Assert(Key.Data);
    
    telemetry_queue *Queue = GlobalTelemetryState->AddingQueue;
    
    telemetry_message *Message = Queue->Messages;
    
    telemetry_field *Field = Message->Fields;
    if(Field != 0)
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
        Field->Next = Message->Fields;
    }
    else
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
    }
    Message->Fields = Field;
    
    Field->Type = TelemetryField_Number;
    Field->Key = PushString_(&Queue->Arena, Key.Size, Key.Data);
    Field->NumberValue = Value;
}


internal void
AppendTelemetryMessageStringField(string Key, string Value)
{
    Assert(GlobalTelemetryState->CurrentState == TelemetryState_AddingToQueue);
    Assert(Key.Data);
    Assert(Value.Data);
    
    telemetry_queue *Queue = GlobalTelemetryState->AddingQueue;
    
    telemetry_message *Message = Queue->Messages;
    
    telemetry_field *Field = Message->Fields;
    if(Field != 0)
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
        Field->Next = Message->Fields;
    }
    else
    {
        Field = PushStruct(&Queue->Arena, telemetry_field);
    }
    Message->Fields = Field;
    
    Field->Type = TelemetryField_String;
    Field->Key = PushString_(&Queue->Arena, Key.Size, Key.Data);
    
    // NOTE(kstandbridge): Sanitize json
    Assert(Value.Size < 4096);
    char Buffer[4096];
    ZeroArray(4096, Buffer);
    char *At = Buffer;
    for(umm Index = 0;
        Index < Value.Size;
        ++Index)
    {
        if(Value.Data[Index] == '\\')
        {
            *At++ = '\\';
            *At++ = '\\';
        }
        else if(Value.Data[Index] == '\n')
        {
            *At++ = '\\';
            *At++ = 'n';
        }
        else if(Value.Data[Index] == '\r')
        {
            *At++ = '\\';
            *At++ = 'r';
        }
        else if(Value.Data[Index] == '"')
        {
            *At++ = '\\';
            *At++ = '"';
        }
        else
        {
            *At++ = Value.Data[Index];
        }
    }
    
    Field->StringValue = FormatString(&Queue->Arena, "%s", Buffer);
}

internal void
EndTelemetryMessage()
{
    Assert(GlobalTelemetryState->CurrentState == TelemetryState_AddingToQueue);
    
    telemetry_message *Message = GlobalTelemetryState->AddingQueue->Messages;
    Message->TimeStamp = Platform.GetSystemTimestamp();
    
    CompletePreviousWritesBeforeFutureWrites;
    GlobalTelemetryState->CurrentState = TelemetryState_Idle;
}

internal void
AddTelemetryHost(memory_arena *Arena, string Hostname)
{
    
    for(;;)
    {
        u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState->CurrentState, TelemetryState_AddingHost, TelemetryState_Idle);
        if(StateType == TelemetryState_Idle)
        {
            
            telemetry_host *Host = GlobalTelemetryState->Hosts;
            if(Host != 0)
            {
                Host = PushStruct(Arena, telemetry_host);
                Host->Next = GlobalTelemetryState->Hosts;
            }
            else
            {
                Host = PushStruct(Arena, telemetry_host);
            }
            GlobalTelemetryState->Hosts = Host;
            Host->Hostname = PushString_(Arena, Hostname.Size, Hostname.Data);
            
            CompletePreviousWritesBeforeFutureWrites;
            GlobalTelemetryState->CurrentState = TelemetryState_Idle;
            break;
        }
        else
        {
            _mm_pause();
        }
    }
}

internal void
InitializeTelemetry(memory_arena *Arena, string TeamId, string ProductName, string MachineName, string Username, u32 ProcessId)
{
    if(GlobalTelemetryState->CurrentState == TelemetryState_Uninitialized)
    {
        for(;;)
        {
            u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState->CurrentState, TelemetryState_Initializing, TelemetryState_Uninitialized);
            if(StateType == TelemetryState_Uninitialized)
            {
                GlobalTelemetryState->AddingQueue = BootstrapPushStruct(telemetry_queue, Arena);
                GlobalTelemetryState->AddingQueue->MemoryFlush = BeginTemporaryMemory(&GlobalTelemetryState->AddingQueue->Arena);
                
                GlobalTelemetryState->ProcessingQueue = BootstrapPushStruct(telemetry_queue, Arena);
                GlobalTelemetryState->ProcessingQueue->MemoryFlush = BeginTemporaryMemory(&GlobalTelemetryState->ProcessingQueue->Arena);
                
                GlobalTelemetryState->TeamId = PushString_(Arena, TeamId.Size, TeamId.Data);
                GlobalTelemetryState->ProductName = PushString_(Arena, ProductName.Size, ProductName.Data);
                GlobalTelemetryState->MachineName = PushString_(Arena, MachineName.Size, MachineName.Data);
                GlobalTelemetryState->Username = PushString_(Arena, Username.Size, Username.Data);
                GlobalTelemetryState->ProcessId = ProcessId;
                
                GlobalTelemetryState->CurrentState = TelemetryState_Idle;
                
                break;
            }
            else
            {
                _mm_pause();
            }
        }
    }
    else
    {
        Assert(!"Invalid telemetry state");
    }
}

inline void
SendHeartbeatTelemetry()
{
    if((GlobalTelemetryState->CurrentState != TelemetryState_Uninitialized) &&
       (GlobalTelemetryState->CurrentState != TelemetryState_Initializing))
    {
        BeginTelemetryMessage();
        AppendTelemetryMessageNumberField(String("cl"), VERSION);
        AppendTelemetryMessageStringField(String("category"), String("/heartbeat"));
        AppendTelemetryMessageStringField(String("message"), String("heartbeat"));
        EndTelemetryMessage();
    }
}

inline void
SendTimedHttpTelemetry(string Hostname, u32 Port, string Verb, string Template, string Endpoint, 
                       umm RequestLength, umm ResponseLength, u32 StatusCode, f32 DurationInSeconds)
{
    if((GlobalTelemetryState->CurrentState != TelemetryState_Uninitialized) &&
       (GlobalTelemetryState->CurrentState != TelemetryState_Initializing))
    {
        BeginTelemetryMessage();
        AppendTelemetryMessageNumberField(String("cl"), VERSION);
        AppendTelemetryMessageStringField(String("category"), String("/timed_http"));
        AppendTelemetryMessageStringField(String("message"), String("timed_http"));
        AppendTelemetryMessageStringField(String("hostname"), Hostname);
        AppendTelemetryMessageNumberField(String("port"), (f32)Port);
        AppendTelemetryMessageStringField(String("template"), Template);
        AppendTelemetryMessageStringField(String("endpoint"), Endpoint);
        AppendTelemetryMessageStringField(String("verb"), Verb);
        AppendTelemetryMessageNumberField(String("request_length"), (f32)RequestLength);
        AppendTelemetryMessageNumberField(String("response_length"), (f32)ResponseLength);
        AppendTelemetryMessageNumberField(String("status_code"), (f32)StatusCode);
        AppendTelemetryMessageNumberField(String("duration_in_seconds"), DurationInSeconds);
        AppendTelemetryMessageNumberField(String("duration_in_milliseconds"), (f32)(DurationInSeconds * 1000.0f));
        EndTelemetryMessage();
    }
    
}

// TODO(kstandbridge): Perhaps move these to a general logger that calls into ConsoleLog/FilesystemLog/TelemetryLog

#define SendLogTelemetry___(SourceFilePlusLine, Level, Format, ...) SendLogTelemetry____(String(SourceFilePlusLine), Level, Format, __VA_ARGS__)
#define SendLogTelemetry__(File, Line, Level, Format, ...) SendLogTelemetry___(File "(" #Line ")", Level, Format, __VA_ARGS__)
#define SendLogTelemetry_(File, Line, Level, Format, ...) SendLogTelemetry__(File, Line, Level, Format, __VA_ARGS__)

typedef enum log_level_type
{
    LogLevel_Debug,
    LogLevel_Verbose,
    LogLevel_Info,
    LogLevel_Warning,
    LogLevel_Error
} log_level_type;

#if KENGINE_INTERNAL
#define LogDebug(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Debug, Format, __VA_ARGS__)
#else
#define LogDebug(...)
#endif
#define LogVerbose(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Verbose, Format, __VA_ARGS__)
#define LogInfo(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Info, Format, __VA_ARGS__)
#define LogWarning(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Warning, Format, __VA_ARGS__)
#if KENGINE_INTERNAL
#define LogError(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Error, Format, __VA_ARGS__); __debugbreak();
#else
#define LogError(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Error, Format, __VA_ARGS__);
#endif


typedef void custom_logger(void *Context, date_time Date, log_level_type LogLevel, string Message);
global custom_logger *CustomLoggerFunc_;
global void *CustomLoggerContext_;

internal void
InitCustomLogger(void *CustomContext, custom_logger *AppCustomLogger)
{
    CustomLoggerFunc_ = AppCustomLogger;
    CustomLoggerContext_ = CustomContext;
}

internal void
SendLogTelemetry_____(string SourceFilePlusLine, log_level_type LogLevel, string Message)
{
    string Level = String("info");
    switch(LogLevel)
    {
        case LogLevel_Debug: { Level = String("debug"); } break;
        case LogLevel_Verbose: { Level = String("verbose"); } break;
        case LogLevel_Info: { Level = String("info"); } break;
        case LogLevel_Warning: { Level = String("warning"); } break;
        case LogLevel_Error: { Level = String("error"); } break;
        InvalidDefaultCase;
    }
    
    date_time Date = GetDateTime();
#if KENGINE_INTERNAL
    u32 ThreadId = GetThreadID();
    Platform.ConsoleOut("[%02d/%02d/%04d %02d:%02d:%02d] <%5u> (%S)\t%S\n", 
                        Date.Day, Date.Month, Date.Year, Date.Hour, Date.Minute, Date.Second,
                        ThreadId, Level, Message);
#else
    if(LogLevel > LogLevel_Verbose)
    {
        Platform.ConsoleOut("%S\n", Message);
    }
#endif
    
    if(CustomLoggerFunc_)
    {
        CustomLoggerFunc_(CustomLoggerContext_, Date, LogLevel, Message);
    }
    
    if(LogLevel > LogLevel_Verbose)
    {
        SourceFilePlusLine.Data += SourceFilePlusLine.Size;
        SourceFilePlusLine.Size = 0;
        while(SourceFilePlusLine.Data[-1] != '\\')
        {
            ++SourceFilePlusLine.Size;
            --SourceFilePlusLine.Data;
        }
        
        if((GlobalTelemetryState->CurrentState != TelemetryState_Uninitialized) &&
           (GlobalTelemetryState->CurrentState != TelemetryState_Initializing))
        {
            BeginTelemetryMessage();
            AppendTelemetryMessageNumberField(String("cl"), VERSION);
            AppendTelemetryMessageStringField(String("source_file_plus_line"), SourceFilePlusLine);
            AppendTelemetryMessageStringField(String("category"), String("/log"));
            AppendTelemetryMessageStringField(String("log_level"), Level);
            AppendTelemetryMessageStringField(String("message"), Message);
            EndTelemetryMessage();
        }
    }
}

internal void
SendLogTelemetry____(string FileLine, log_level_type LogLevel, char *Format, ...)
{
    u8 Buffer[2048];
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Message = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    
    SendLogTelemetry_____(FileLine, LogLevel, Message);
}

inline void
LogDownloadProgress(u64 CurrentBytes, u64 TotalBytes)
{
    f32 Current = 0;
    f32 Total = 0;
    
    if(TotalBytes > Gigabytes(1))
    {
        Total = (f32)TotalBytes / 1024.0f / 1024.0f / 1024.0f;
        Current = (f32)CurrentBytes / 1024.0f / 1024.0f / 1024.0f;
        LogVerbose("Downloaded (%.02f%%) %.02f / %.02f GB", 
                   (Current / Total)*100.0f, Current, Total);
    }
    else if(TotalBytes > Megabytes(1))
    {
        Total = (f32)TotalBytes / 1024.0f / 1024.0f;
        Current = (f32)CurrentBytes / 1024.0f / 1024.0f;
        LogVerbose("Downloaded (%.02f%%) %.02f / %.02f MB", 
                   (Current / Total)*100.0f, Current, Total);
    }
    else if(TotalBytes > Kilobytes(1))
    {
        Total = (f32)TotalBytes / 1024.0f;
        Current = (f32)CurrentBytes / 1024.0f;
        LogVerbose("Downloaded (%.02f%%) %u / %u KB", 
                   (Current / Total)*100.0f, (u32)Current, (u32)Total);
    }
    else if(TotalBytes > 0)
    {
        Total = (f32)TotalBytes;
        Current = (f32)CurrentBytes;
        LogVerbose("Downloaded (%.02f%%) %u / %u Bytes", 
                   (Current / Total)*100.0f, (u32)Current, (u32)Total);
    }
    else
    {
        // NOTE(kstandbridge): Unknown remaining
        
        if(CurrentBytes > Gigabytes(1))
        {
            LogVerbose("Downloaded %.02f GB", CurrentBytes / 1024.0 / 1024.0f / 1024.0f);
        }
        else if(CurrentBytes > Megabytes(1))
        {
            LogVerbose("Downloaded %.02f MB", CurrentBytes / 1024.0 / 1024.0f);
        }
        else if(CurrentBytes > Kilobytes(1))
        {
            LogVerbose("Downloaded %u KB", CurrentBytes / 1024);
        }
        else if(CurrentBytes > 0)
        {
            LogVerbose("Downloaded %u Bytes", CurrentBytes);
        }
    }
}

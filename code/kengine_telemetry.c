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

telemetry_state GlobalTelemetryState_;

internal void
PostTelemetryThread(void *Data)
{
    // NOTE(kstandbridge): Swap queues
    for(;;)
    {
        u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_SwappingQueues, TelemetryState_Idle);
        if(StateType == TelemetryState_Idle)
        {
            telemetry_queue *Temp = GlobalTelemetryState_.AddingQueue;
            GlobalTelemetryState_.AddingQueue = GlobalTelemetryState_.ProcessingQueue;
            GlobalTelemetryState_.ProcessingQueue = Temp;
            
            CompletePreviousWritesBeforeFutureWrites;
            GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
            
            telemetry_queue *Queue = GlobalTelemetryState_.ProcessingQueue;
            
            // NOTE(kstandbridge): Post all telemetry messages
            if(GlobalTelemetryState_.ProcessingQueue->Messages)
            {
                for(telemetry_host *Host = GlobalTelemetryState_.Hosts;
                    Host;
                    Host = Host->Next)
                {
                    
                    platform_http_client Client = Platform.BeginHttpClient(Host->Hostname, 0);
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
                                                                                  GlobalTelemetryState_.TeamId,
                                                                                  GlobalTelemetryState_.ProductName,
                                                                                  DateTime.Year, DateTime.Month,
                                                                                  GlobalTelemetryState_.ProductName,
                                                                                  GlobalTelemetryState_.MachineName,
                                                                                  GlobalTelemetryState_.ProcessId,
                                                                                  DateTime.Year, DateTime.Month, DateTime.Day,
                                                                                  DateTime.Hour, DateTime.Minute, DateTime.Second,
                                                                                  DateTime.Milliseconds);
                        if(PlatformNoErrors(Request))
                        {
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
                                
                                AppendFormatString(&StringState, ",\n    \"product_name\": \"%S\"", GlobalTelemetryState_.ProductName);
                                AppendFormatString(&StringState, ",\n    \"machine_name\": \"%S\"", GlobalTelemetryState_.MachineName);
                                AppendFormatString(&StringState, ",\n    \"user_name\": \"%S\"", GlobalTelemetryState_.Username);
                                AppendFormatString(&StringState, ",\n    \"pid\": %.03f", GlobalTelemetryState_.ProcessId);
                                
                                AppendFormatString(&StringState, "\n}");
                                
                                Request.Payload = EndFormatStringToBuffer(&StringState, CPayload, sizeof(CPayload));
                                
                                u32 StatusCode = Platform.SendHttpRequest(&Request);
                                if(StatusCode == 0)
                                {
                                    // NOTE(kstandbridge): Likely offline host so skip rest of messages
                                    break;
                                }
#if KENGINE_INTERNAL
                                string Response = Platform.GetHttpResponse(&Request);
                                b32 BreakHere = true;
#endif
                            }
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

internal void
BeginTelemetryMessage()
{
    for(;;)
    {
        u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_AddingToQueue, TelemetryState_Idle);
        if(StateType == TelemetryState_Idle)
        {
            telemetry_message *Message = GlobalTelemetryState_.AddingQueue->Messages;
            if(Message != 0)
            {
                Message = PushStruct(&GlobalTelemetryState_.AddingQueue->Arena, telemetry_message);
                Message->Next = GlobalTelemetryState_.AddingQueue->Messages;
            }
            else
            {
                Message = PushStruct(&GlobalTelemetryState_.AddingQueue->Arena, telemetry_message);
            }
            GlobalTelemetryState_.AddingQueue->Messages = Message;
            
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
    Assert(GlobalTelemetryState_.CurrentState == TelemetryState_AddingToQueue);
    Assert(Key.Data);
    
    telemetry_queue *Queue = GlobalTelemetryState_.AddingQueue;
    
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
    Assert(GlobalTelemetryState_.CurrentState == TelemetryState_AddingToQueue);
    Assert(Key.Data);
    Assert(Value.Data);
    
    telemetry_queue *Queue = GlobalTelemetryState_.AddingQueue;
    
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
    Assert(GlobalTelemetryState_.CurrentState == TelemetryState_AddingToQueue);
    
    telemetry_message *Message = GlobalTelemetryState_.AddingQueue->Messages;
    Message->TimeStamp = Platform.GetSystemTimestamp();
    
    CompletePreviousWritesBeforeFutureWrites;
    GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
}

internal void
AddTelemetryHost(memory_arena *Arena, string Hostname)
{
    
    for(;;)
    {
        u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_AddingHost, TelemetryState_Idle);
        if(StateType == TelemetryState_Idle)
        {
            
            telemetry_host *Host = GlobalTelemetryState_.Hosts;
            if(Host != 0)
            {
                Host = PushStruct(Arena, telemetry_host);
                Host->Next = GlobalTelemetryState_.Hosts;
            }
            else
            {
                Host = PushStruct(Arena, telemetry_host);
            }
            GlobalTelemetryState_.Hosts = Host;
            Host->Hostname = PushString_(Arena, Hostname.Size, Hostname.Data);
            
            CompletePreviousWritesBeforeFutureWrites;
            GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
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
    if(GlobalTelemetryState_.CurrentState == TelemetryState_Uninitialized)
    {
        for(;;)
        {
            u32 StateType = AtomicCompareExchangeU32((u32 *)&GlobalTelemetryState_.CurrentState, TelemetryState_Initializing, TelemetryState_Uninitialized);
            if(StateType == TelemetryState_Uninitialized)
            {
                GlobalTelemetryState_.AddingQueue = BootstrapPushStruct(telemetry_queue, Arena);
                GlobalTelemetryState_.AddingQueue->MemoryFlush = BeginTemporaryMemory(&GlobalTelemetryState_.AddingQueue->Arena);
                
                GlobalTelemetryState_.ProcessingQueue = BootstrapPushStruct(telemetry_queue, Arena);
                GlobalTelemetryState_.ProcessingQueue->MemoryFlush = BeginTemporaryMemory(&GlobalTelemetryState_.ProcessingQueue->Arena);
                
                GlobalTelemetryState_.TeamId = PushString_(Arena, TeamId.Size, TeamId.Data);
                GlobalTelemetryState_.ProductName = PushString_(Arena, ProductName.Size, ProductName.Data);
                GlobalTelemetryState_.MachineName = PushString_(Arena, MachineName.Size, MachineName.Data);
                GlobalTelemetryState_.Username = PushString_(Arena, Username.Size, Username.Data);
                GlobalTelemetryState_.ProcessId = ProcessId;
                
                GlobalTelemetryState_.CurrentState = TelemetryState_Idle;
                
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
    BeginTelemetryMessage();
    AppendTelemetryMessageNumberField(String("cl"), VERSION);
    AppendTelemetryMessageStringField(String("category"), String("/heartbeat"));
    AppendTelemetryMessageStringField(String("message"), String("heartbeat"));
    EndTelemetryMessage();
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
#define LogError(Format, ...) __debugbreak(); SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Error, Format, __VA_ARGS__);
#else
#define LogError(Format, ...) SendLogTelemetry_(__FILE__, __LINE__, LogLevel_Error, Format, __VA_ARGS__);
#endif

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
    
#if KENGINE_INTERNAL
    date_time Date = GetDateTime();
    u32 ThreadId = GetThreadID();
    Platform.ConsoleOut("[%02d/%02d/%04d %02d:%02d:%02d] <Thread:%5u> (%S)\t%S\n", 
                        Date.Day, Date.Month, Date.Year, Date.Hour, Date.Minute, Date.Second,
                        ThreadId, Level, Message);
#else
    Platform.ConsoleOut("%S\n", Message);
#endif
    
    
    if(LogLevel > LogLevel_Verbose)
    {
        SourceFilePlusLine.Data += SourceFilePlusLine.Size;
        SourceFilePlusLine.Size = 0;
        while(SourceFilePlusLine.Data[-1] != '\\')
        {
            ++SourceFilePlusLine.Size;
            --SourceFilePlusLine.Data;
        }
        
        if((GlobalTelemetryState_.CurrentState != TelemetryState_Uninitialized) &&
           (GlobalTelemetryState_.CurrentState != TelemetryState_Initializing))
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
    u8 Buffer[4096];
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Message = EndFormatStringToBuffer(&StringState, Buffer, BufferSize);
    
    SendLogTelemetry_____(FileLine, LogLevel, Message);
}

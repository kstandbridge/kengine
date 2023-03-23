#ifndef KENGINE_LOG_H


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

void
SendLogTelemetry____(string FileLine, log_level_type LogLevel, char *Format, ...);

void
InitializeTelemetry(memory_arena *Arena, string TeamId, string ProductName, string MachineName, string Username, u32 ProcessId);

void
AddTelemetryHost(memory_arena *Arena, string Hostname);

void
PostTelemetryThread(void *Data);

typedef void custom_logger(void *Context, date_time Date, log_level_type LogLevel, string Message);
void
InitCustomLogger(void *CustomContext, custom_logger *AppCustomLogger);

void
BeginTelemetryMessage();

void
AppendTelemetryMessageNumberField(string Key, f32 Value);

void
AppendTelemetryMessageStringField(string Key, string Value);

void
EndTelemetryMessage();

void
LogDownloadProgress(u64 CurrentBytes, u64 TotalBytes);

void
SendTimedHttpTelemetry(string Hostname, u32 Port, string Verb, string Template, string Endpoint, 
                       umm RequestLength, umm ResponseLength, u32 StatusCode, f32 DurationInSeconds);

#define KENGINE_LOG_H
#endif //KENGINE_LOG_H

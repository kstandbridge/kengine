#ifndef KENGINE_PLATFORM_H

#include <stdarg.h>
#include <immintrin.h>

#define introspect(...)
#include "kengine_types.h"


typedef struct app_button_state
{
    s32 HalfTransitionCount;
    b32 EndedDown;
} app_button_state;

typedef enum
{
    MouseButton_Left,
    MouseButton_Middle,
    MouseButton_Right,
    MouseButton_Extended0,
    MouseButton_Extended1,
    
    MouseButton_Count,
} mouse_button_type;

typedef struct app_input
{
    app_button_state MouseButtons[MouseButton_Count];
    f32 MouseX;
    f32 MouseY;
    f32 MouseZ;
    
    b32 ShiftDown, AltDown, ControlDown;
    b32 FKeyPressed[13];
} app_input;

inline b32 
WasPressed(app_button_state State)
{
    b32 Result = ((State.HalfTransitionCount > 1) ||
                  ((State.HalfTransitionCount == 1) && (State.EndedDown)));
    
    return Result;
}

typedef enum platform_memory_block_flags
{
    PlatformMemoryBlockFlag_OverflowCheck = 0x1,
    PlatformMemoryBlockFlag_UnderflowCheck = 0x2
} platform_memory_block_flags;

typedef struct platform_memory_block
{
    u64 Flags;
    u64 Size;
    u8 *Base;
    umm Used;
    
    struct platform_memory_block *Prev;
} platform_memory_block;

typedef void platform_work_queue_callback(void *Data);

typedef struct platform_work_queue_entry
{
    platform_work_queue_callback *Callback;
    void *Data;
} platform_work_queue_entry;

typedef struct platform_work_queue 
{
    platform_work_queue_entry Entries[256];
} platform_work_queue;

typedef platform_work_queue *platform_make_work_queue(struct memory_arena *Arena, u32 ThreadCount);
typedef void platform_add_work_entry(platform_work_queue *Queue, platform_work_queue_callback *Callback, void *Data);
typedef void platform_complete_all_work(platform_work_queue *Queue);

#if KENGINE_CONSOLE
typedef void platform_init_console_command_loop(platform_work_queue *Queue);
#endif

typedef platform_memory_block *platform_allocate_memory(umm Size, u64 Flags);
typedef void platform_deallocate_memory(platform_memory_block *Block);

typedef struct platform_memory_stats
{
    u32 BlockCount;
    umm TotalSize;
    umm TotalUsed;
} platform_memory_stats;

typedef platform_memory_stats platform_get_memory_stats();

// TODO(kstandbridge): platform_
typedef loaded_glyph get_glyph_for_code_point(struct memory_arena *Arena, u32 CodePoint);
typedef f32 get_horizontal_advance(u32 PrevCodePoint, u32 CodePoint);
typedef f32 get_verticle_advance();


// TODO(kstandbridge): move this somewhere?
typedef enum http_verb_type
{
    HttpVerb_Post,
    HttpVerb_Get,
    HttpVerb_Put,
    HttpVerb_Patch,
    HttpVerb_Delete,
} http_verb_type;

typedef struct platform_http_client
{
    void *Handle;
    
    b32 NoErrors;
} platform_http_client;

typedef void platform_end_http_client(platform_http_client *PlatformClient);
typedef platform_http_client platform_begin_http_client_with_creds(string Hostname, u32 Port, string Username, string Password);
typedef platform_http_client platform_begin_http_client(string Hostname, u32 Port);

typedef struct platform_http_request
{
    void *Handle;
    b32 NoErrors;
    
    string Endpoint;
    string Payload;
    
} platform_http_request;

typedef void platform_end_http_request(platform_http_request *PlatformRequest);
typedef platform_http_request platform_begin_http_request(platform_http_client *PlatformClient,  http_verb_type Verb, char *Format, ...);

typedef struct platform_http_response
{
    u16 StatusCode;
    string Payload;
} platform_http_response;

typedef platform_http_response platform_get_http_response(platform_http_request *PlatformRequest);

#define PlatformNoErrors(Handle) ((Handle).NoErrors)

typedef b32 platform_file_exists(string Path);
typedef b32 platform_kill_process_by_name(string Name);
typedef void platform_execute_process(string Path, string Args, string WorkingDirectory);
typedef umm platform_get_hostname(u8 *Buffer, umm BufferMaxSize);
typedef umm platform_get_username(u8 *Buffer, umm BufferMaxSize);
typedef umm platform_get_home_directory(u8 *Buffer, umm BufferMaxSize);
typedef u32 platform_get_process_id();
typedef u64 platform_get_system_timestamp();
typedef date_time platform_get_date_time_for_timestamp(u64 Timestamp);
typedef void platform_console_out(char *Format, ...);

typedef struct platform_api
{
    platform_make_work_queue *MakeWorkQueue;
    platform_add_work_entry *AddWorkEntry;
    platform_complete_all_work *CompleteAllWork;
    
#if KENGINE_CONSOLE
    platform_init_console_command_loop *InitConsoleCommandLoop;
#endif
    
    platform_allocate_memory *AllocateMemory;
    platform_deallocate_memory *DeallocateMemory;
    platform_get_memory_stats *GetMemoryStats;
    
    // TODO(kstandbridge): platform_
    get_glyph_for_code_point *GetGlyphForCodePoint;
    get_horizontal_advance *GetHorizontalAdvance;
    get_verticle_advance *GetVerticleAdvance;
    
    platform_end_http_client *EndHttpClient;
    platform_begin_http_client_with_creds *BeginHttpClientWithCreds;
    platform_begin_http_client *BeginHttpClient;
    platform_end_http_request *EndHttpRequest;
    platform_begin_http_request *BeginHttpRequest;
    platform_get_http_response *GetHttpResonse;
    
    platform_file_exists *FileExists;
    platform_kill_process_by_name *KillProcessByName;
    platform_execute_process *ExecuteProcess;
    platform_get_hostname *GetHostname;
    platform_get_username *GetUsername;
    platform_get_home_directory *GetHomeDirectory;
    platform_get_process_id *GetProcessId;
    platform_get_system_timestamp *GetSystemTimestamp;
    platform_get_date_time_for_timestamp *GetDateTimeFromTimestamp;
    platform_console_out *ConsoleOut;
    
} platform_api;
extern platform_api Platform;

// TODO(kstandbridge): Rename to platform_state ???
typedef struct app_memory
{
    // TODO(kstandbridge): App state maybe, UI state hard no
    struct app_state *AppState;
    struct ui_state *UIState;
    
#if KENGINE_INTERNAL
    struct debug_state *DebugState;
    struct debug_event_table *DebugEventTable;
#endif
    u32 ArgCount;
    string *Args;
    b32 IsRunning;
    
    platform_api PlatformAPI;
} app_memory;

typedef void app_handle_command(app_memory *Memory, string Command, u32 ArgCount, string *Args);

#if defined(KENGINE_CONSOLE) || defined(KENGINE_HEADLESS)
typedef void app_tick_(app_memory *Memory, f32 dtForFrame);
#else
typedef struct render_commands render_commands;
typedef void app_tick_(app_memory *Memory, render_commands *Commands, app_input *Input, f32 dtForFrame);
#endif

#if KENGINE_HTTP
typedef platform_http_response app_handle_http_request(app_memory *Memory, struct memory_arena *Arena, platform_http_request Request);
#endif

#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

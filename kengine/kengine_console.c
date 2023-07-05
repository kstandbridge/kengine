typedef struct app_memory
{
    struct app_state *AppState;

#if KENGINE_CONSOLE
    u32 ArgCount;
    char **Args;
#endif

} app_memory;
global app_memory GlobalAppMemory;

s32
MainLoop(app_memory *AppMemory);

s32
main(u32 ArgCount, char *Args[])
{
    s32 Result = 0;

    GlobalAppMemory.ArgCount = ArgCount - 1;
    GlobalAppMemory.Args = Args + 1;

#if KENGINE_WIN32
    GlobalWin32State.MemorySentinel.Prev = &GlobalWin32State.MemorySentinel;
    GlobalWin32State.MemorySentinel.Next = &GlobalWin32State.MemorySentinel;
#elif KENGINE_LINUX
    GlobalLinuxState.MemorySentinel.Prev = &GlobalLinuxState.MemorySentinel;
    GlobalLinuxState.MemorySentinel.Next = &GlobalLinuxState.MemorySentinel;
#endif

    Result = MainLoop(&GlobalAppMemory);

    return Result;
}
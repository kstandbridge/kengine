typedef struct app_memory
{
    struct app_state *AppState;

    u32 ArgCount;
    char **Args;

} app_memory;
global app_memory GlobalAppMemory;

string_list *
PlatformGetCommandLineArgs(memory_arena *Arena)
{
    string_list *Result = 0;
    
    for(u32 ArgIndex = 0;
        ArgIndex < GlobalAppMemory.ArgCount;
        ++ArgIndex)
    {
        char *Arg = GlobalAppMemory.Args[ArgIndex];
        
        string Entry = 
        {
            .Size = GetNullTerminiatedStringLength(Arg),
            .Data = (u8 *)Arg
        };
        
        string_list *New = PushbackStringList(&Result, Arena);
        New->Entry = Entry;
    }
    
    return Result;
}

s32
MainLoop(app_memory *AppMemory);

s32
main(int ArgCount, char *Args[])
{
    s32 Result = 0;

    GlobalAppMemory.ArgCount = ArgCount - 1;
    GlobalAppMemory.Args = Args + 1;

    InitGlobalMemory();

    Result = MainLoop(&GlobalAppMemory);

    return Result;
}
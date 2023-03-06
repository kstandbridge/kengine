
internal void
PushStringToStringList(string_list **HeadRef, memory_arena *Arena, string Text)
{
    string_list *Result = PushbackStringList(HeadRef, Arena);
    Result->Entry = Text;
}

internal string *
GetStringFromStringList(string_list *Head, string Text)
{
    string *Result = 0;
    
    string_list Match = { .Entry = Text };
    string_list *Found = GetStringList(Head, StringListMatch, 0, &Match);
    if(Found)
    {
        Result = &Found->Entry;
    }
    
    return Result;
}

internal string *
GetStringFromStringListByIndex(string_list *Head, s32 Index)
{
    string *Result = 0;
    
    string_list *Found = GetStringListByIndex(Head, Index);
    if(Found)
    {
        Result = &Found->Entry;
    }
    
    return Result;
}
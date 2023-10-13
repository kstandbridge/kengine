
typedef struct app_memory
{
    memory_arena Arena;

    u32 ArgCount;
    char **Args;

} app_memory;
global app_memory GlobalAppMemory;

internal void
GenerateCtor(c_struct Struct)
{
    PlatformConsoleOut("\n#define %S(", Struct.Name);
    
    b32 First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        PlatformConsoleOut("%S", Member->Name);
        First = false;
    }
    
    PlatformConsoleOut(") (%S){", Struct.Type);
    First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        if(Member->Type == C_Custom)
        {
            PlatformConsoleOut("(%S)", Member->Name);
        }
        else
        {
            PlatformConsoleOut("(%S)(%S)", Member->TypeName, Member->Name);
        }
        First = false;
    }
    PlatformConsoleOut("}");
}

internal void
GenerateSet1(c_struct Struct)
{
    PlatformConsoleOut("\n#define %SSet1(Value) (%S){", Struct.Name, Struct.Type);
    
    b32 First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        PlatformConsoleOut("(%S)(Value)", Member->TypeName);
        First = false;
    }
    PlatformConsoleOut("}");
}

internal void
GenerateMath_(c_struct Struct, string Name, string Op)
{
    PlatformConsoleOut("\n#define %S%S(A, B) (%S){", Struct.Name, Name, Struct.Type);
    
    b32 First = true;
    for(c_member *Member = Struct.Members;
        Member;
        Member = Member->Next)
    {
        if(!First)
        {
            PlatformConsoleOut(", ");
        }
        PlatformConsoleOut("(A.%S %S B.%S)", Member->Name, Op, Member->Name);
        First = false;
    }
    PlatformConsoleOut("}");
}

internal void
GenerateMath(c_struct Struct)
{
    GenerateMath_(Struct, String("Add"), String("+"));
    GenerateMath_(Struct, String("Subtract"), String("-"));
    GenerateMath_(Struct, String("Multiply"), String("*"));
}

internal void
GenerateLinkedList(c_struct Struct)
{
    string Name = Struct.Name;
    string Type = Struct.Type;
    
    PlatformConsoleOut("\ninternal u32\nGet%SCount(%S *Head)\n", Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    u32 Result = 0;\n\n");
    PlatformConsoleOut("    while(Head != 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Head = Head->Next;\n");
    PlatformConsoleOut("        ++Result;\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    
    PlatformConsoleOut("\ninternal %S *\nGet%SByIndex(%S *Head, s32 Index)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("        %S *Result = Head;\n\n", Type);
    PlatformConsoleOut("        if(Result != 0)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            while(Result && Index)\n");
    PlatformConsoleOut("            {\n");
    PlatformConsoleOut("                Result = Result->Next;\n");
    PlatformConsoleOut("            --Index;\n");
    PlatformConsoleOut("            }\n");
    PlatformConsoleOut("        }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninternal s32\nGetIndexOf%S(%S *Head, %S *%S)\n", Name, Type, Type, Name);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    s32 Result = -1;\n\n");
    PlatformConsoleOut("    for(s32 Index = 0;\n");
    PlatformConsoleOut("        Head;\n");
    PlatformConsoleOut("        ++Index, Head = Head->Next)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("        if(Head == %S)\n", Name);
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Index;\n");
    PlatformConsoleOut("            break;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ntypedef b32 %S_predicate(void *Context, %S *A, %S *B);\n", Type, Type, Type);
    
    PlatformConsoleOut("\ninternal %S *\nGet%S(%S *Head, %S_predicate *Predicate, void *Context, %S *Match)\n", Type, Name, Type, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = 0;\n\n", Type);
    PlatformConsoleOut("    while(Head)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        if(Predicate(Context, Head, Match))\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Head;\n");
    PlatformConsoleOut("            break;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        else\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Head = Head->Next;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninternal %S *\nGet%STail(%S *Head)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = Head;\n\n", Type);
    PlatformConsoleOut("    if(Result != 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        while(Result->Next != 0)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Result->Next;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninternal %S *\nPush%S(%S **HeadRef, memory_arena *Arena)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    PlatformConsoleOut("    Result->Next = *HeadRef;\n");
    PlatformConsoleOut("    *HeadRef = Result;\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninternal %S *\nPushback%S(%S **HeadRef, memory_arena *Arena)\n", Type, Name, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    PlatformConsoleOut("    Result->Next = 0;\n");
    PlatformConsoleOut("    if(*HeadRef == 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        *HeadRef = Result;\n");
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("    else\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        %S *Tail = Get%STail(*HeadRef);\n", Type, Name);
    PlatformConsoleOut("        Tail->Next = Result;\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    PlatformConsoleOut("\ninternal %S *\n%SMergeSort_(%S *Front, %S *Back, %S_predicate *Predicate, void *Context, sort_type SortType)\n", 
                       Type, Name, Type, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Result = 0;\n", Type);
    PlatformConsoleOut("    if(Front == 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Result = Back;\n");
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("    else if(Back == 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Result = Front;\n");
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("    else\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        b32 PredicateResult = Predicate(Context, Front, Back);\n");
    PlatformConsoleOut("        if(SortType == Sort_Descending)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            PredicateResult = !PredicateResult;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        else\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Assert(SortType == Sort_Ascending);\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        if(PredicateResult)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Front;\n");
    PlatformConsoleOut("            Result->Next = %SMergeSort_(Front->Next, Back, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("        else\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Result = Back;\n");
    PlatformConsoleOut("            Back->Next = %SMergeSort_(Front, Back->Next, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    return Result;\n");
    PlatformConsoleOut("}\n");
    
    
    PlatformConsoleOut("\ninternal void\n%SFrontBackSplit(%S *Head, %S **FrontRef, %S **BackRef)\n", Name, Type, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Fast;\n", Type);
    PlatformConsoleOut("    %S *Slow;\n", Type);
    PlatformConsoleOut("    Slow = Head;\n");
    PlatformConsoleOut("    Fast = Head->Next;\n\n");
    PlatformConsoleOut("    while(Fast != 0)\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        Fast = Fast->Next;\n");
    PlatformConsoleOut("        if(Fast != 0)\n");
    PlatformConsoleOut("        {\n");
    PlatformConsoleOut("            Slow = Slow->Next;\n");
    PlatformConsoleOut("            Fast = Fast->Next;\n");
    PlatformConsoleOut("        }\n");
    PlatformConsoleOut("    }\n\n");
    PlatformConsoleOut("    *FrontRef = Head;\n");
    PlatformConsoleOut("    *BackRef = Slow->Next;\n");
    PlatformConsoleOut("    Slow->Next = 0;\n");
    PlatformConsoleOut("}\n");
    
    
    PlatformConsoleOut("\ninternal void\n%SMergeSort(%S **HeadRef, %S_predicate *Predicate, void *Context, sort_type SortType)\n", Name, Type, Type);
    PlatformConsoleOut("{\n");
    PlatformConsoleOut("    %S *Head = *HeadRef;\n\n", Type);
    PlatformConsoleOut("    if((Head!= 0) &&\n");
    PlatformConsoleOut("       (Head->Next != 0))\n");
    PlatformConsoleOut("    {\n");
    PlatformConsoleOut("        %S *Front;\n", Type);
    PlatformConsoleOut("        %S *Back;\n", Type);
    PlatformConsoleOut("        %SFrontBackSplit(Head, &Front, &Back);\n\n", Name);
    PlatformConsoleOut("        %SMergeSort(&Front, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("        %SMergeSort(&Back, Predicate, Context, SortType);\n\n", Name);
    PlatformConsoleOut("        *HeadRef = %SMergeSort_(Front, Back, Predicate, Context, SortType);\n", Name);
    PlatformConsoleOut("    }\n");
    PlatformConsoleOut("}\n");
}

void
GenerateCodeFor(memory_arena *Arena, c_struct Struct, string_list *Options);

internal void
PreprocessSourceFile(memory_arena *Arena, string FileName, string FileData)
{
    tokenizer Tokenizer_ = Tokenize(FileData, FileName);
    tokenizer *Tokenizer = &Tokenizer_;
    
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_Unknown)
        {
            TokenError(Tokenizer, Token, "Unknown token: \"%S\"", Token.Text);
        }
        else
        {
            if(Token.Type == Token_Identifier)
            {
                if(StringsAreEqual(Token.Text, String("introspect")))
                {
                    Token = RequireToken(Tokenizer, Token_OpenParenthesis);
                    
                    string_list *Options = 0;
                    
                    while(Parsing(Tokenizer))
                    {
                        Token = GetToken(Tokenizer);
                        if(Token.Type == Token_CloseParenthesis)
                        {
                            break;
                        }
                        else if(Token.Type == Token_Comma)
                        {
                        }
                        else if(Token.Type == Token_Identifier)
                        {
                            string Option = Token.Text;
                            PushStringToStringList(&Options, Arena, Option);
                        }
                        else
                        {
                            TokenError(Tokenizer, Token, "Expecting parameter list for introspect");
                        }
                    }
                    
                    if(Parsing(Tokenizer))
                    {
                        RequireIdentifierToken(Tokenizer, String("typedef"));
                        RequireIdentifierToken(Tokenizer, String("struct"));
                        Token = RequireToken(Tokenizer, Token_Identifier);
                        
                        if(Parsing(Tokenizer))
                        {                    
                            c_struct Struct = ParseStruct(Arena, Tokenizer, Token.Text);
                            
                            string_list *CustomOptions = 0;
                            
                            for(string_list *Option = Options;
                                Option;
                                Option = Option->Next)
                            {
                                if(StringsAreEqual(String("ctor"), Option->Entry))
                                {
                                    GenerateCtor(Struct);
                                }
                                else if(StringsAreEqual(String("set1"), Option->Entry))
                                {
                                    GenerateSet1(Struct);
                                }
                                else if(StringsAreEqual(String("math"), Option->Entry))
                                {
                                    GenerateMath(Struct);
                                }
                                else if(StringsAreEqual(String("linked_list"), Option->Entry))
                                {
                                    GenerateLinkedList(Struct);
                                }
                                else
                                {
                                    PushStringToStringList(&CustomOptions, Arena, Option->Entry);
                                }
                            }
                            
                            GenerateCodeFor(Arena, Struct, CustomOptions);
                        }
                        
                        PlatformConsoleOut("\n");
                    }
                }
            }
        }
    }
}

s32
main(int ArgCount, char **Args)
{
    s32 Result = 0;

    GlobalAppMemory.ArgCount = ArgCount - 1;
    GlobalAppMemory.Args = Args + 1;

    GlobalMemory.MemorySentinel.Prev = &GlobalMemory.MemorySentinel;
    GlobalMemory.MemorySentinel.Next = &GlobalMemory.MemorySentinel;
    
    if(ArgCount == 1)
    {
        PlatformConsoleOut("#error No input file specified\n", 0);
    }
    else
    {    
        for(u32 ArgIndex = 1;
            ArgIndex < ArgCount;
            ++ArgIndex)
        {
            char *Arg = Args[ArgIndex];
            string FileName = String_(GetNullTerminiatedStringLength(Arg), (u8 *)Arg);
            if(PlatformFileExists(FileName))
            {
                PlatformConsoleOut("\n// %S\n", FileName);
                
                temporary_memory MemoryFlush = BeginTemporaryMemory(&GlobalAppMemory.Arena);
                
                string FileData = PlatformReadEntireFile(MemoryFlush.Arena, FileName);
                
                PreprocessSourceFile(MemoryFlush.Arena, FileName, FileData);
                
                EndTemporaryMemory(MemoryFlush);
            }
            else
            {
                PlatformConsoleOut("\n#error invalid filename specified: %S\n", FileName);
            }
        }
    }
    
    return Result;
}
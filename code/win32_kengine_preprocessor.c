#include "win32_kengine_preprocessor.h"
#include "win32_kengine_kernel.c"

internal HANDLE
Win32CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
    HANDLE Result;
    
    Assert(Kernel32);
    local_persist create_file_a *Func = 0;
    if(!Func);
    {
        Func = (create_file_a *)Win32GetProcAddressA(Kernel32, "CreateFileA");
    }
    Assert(Func);
    Result = Func(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
    
    return Result;
}

internal BOOL
Win32GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER lpFileSize)
{
    BOOL Result;
    
    Assert(Kernel32);
    local_persist get_file_size_ex *Func = 0;
    if(!Func);
    {
        Func = (get_file_size_ex *)Win32GetProcAddressA(Kernel32, "GetFileSizeEx");
    }
    Assert(Func);
    Result = Func(hFile, lpFileSize);
    
    return Result;
}

internal BOOL
Win32ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped)
{
    BOOL Result;
    
    Assert(Kernel32);
    local_persist read_file *Func = 0;
    if(!Func);
    {
        Func = (read_file *)Win32GetProcAddressA(Kernel32, "ReadFile");
    }
    Assert(Func);
    Result = Func(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
    
    return Result;
}

internal BOOL
Win32CloseHandle(HANDLE hObject)
{
    BOOL Result;
    
    Assert(Kernel32);
    local_persist close_handle *Func = 0;
    if(!Func);
    {
        Func = (close_handle *)Win32GetProcAddressA(Kernel32, "CloseHandle");
    }
    Assert(Func);
    Result = Func(hObject);
    
    return Result;
}

internal HANDLE
Win32GetStdHandle(DWORD nStdHandle)
{
    HANDLE Result;
    
    Assert(Kernel32);
    local_persist get_std_handle *Func = 0;
    if(!Func);
    {
        Func = (get_std_handle *)Win32GetProcAddressA(Kernel32, "GetStdHandle");
    }
    Assert(Func);
    Result = Func(nStdHandle);
    
    return Result;
}

internal BOOL
Win32WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped)
{
    BOOL Result;
    
    Assert(Kernel32);
    local_persist write_file *Func = 0;
    if(!Func);
    {
        Func = (write_file *)Win32GetProcAddressA(Kernel32, "WriteFile");
    }
    Assert(Func);
    Result = Func(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
    
    return Result;
}

internal LPVOID
Win32VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
{
    LPVOID Result;
    
    Assert(Kernel32);
    local_persist virtual_alloc *Func = 0;
    if(!Func);
    {
        Func = (virtual_alloc *)Win32GetProcAddressA(Kernel32, "VirtualAlloc");
    }
    Assert(Func);
    Result = Func(lpAddress, dwSize, flAllocationType, flProtect);
    
    return Result;
}

internal LPSTR
Win32GetCommandLineA()
{
    LPSTR Result;
    
    Assert(Kernel32);
    local_persist get_command_line_a *Func = 0;
    if(!Func);
    {
        Func = (get_command_line_a *)Win32GetProcAddressA(Kernel32, "GetCommandLineA");
    }
    Assert(Func);
    Result = Func();
    
    return Result;
}

internal void
Win32ExitProcess(UINT uExitCode)
{
    Assert(Kernel32);
    local_persist exit_process *Func = 0;
    if(!Func);
    {
        Func = (exit_process *)Win32GetProcAddressA(Kernel32, "ExitProcess");
    }
    Assert(Func);
    Func(uExitCode);
}

internal string
Win32ReadEntireFile(memory_arena *Arena, char *FilePath)
{
    string Result;
    ZeroStruct(Result);
    
    HANDLE FileHandle = Win32CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    Assert(FileHandle != INVALID_HANDLE_VALUE);
    
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        b32 ReadResult = Win32GetFileSizeEx(FileHandle, &FileSize);
        Assert(ReadResult);
        if(ReadResult)
        {    
            Result.Size = FileSize.QuadPart;
            Result.Data = PushSize(Arena, Result.Size);
            Assert(Result.Data);
            
            if(Result.Data)
            {
                u32 BytesRead;
                ReadResult = Win32ReadFile(FileHandle, Result.Data, (u32)Result.Size, (LPDWORD)&BytesRead, 0);
                Assert(ReadResult);
                Assert(BytesRead == Result.Size);
            }
        }
        
        Win32CloseHandle(FileHandle);
    }
    
    return Result;
}

internal b32
Win32PrintOut(memory_arena *Arena, char *Format, ...)
{
    u8 Buffer[4096];
    umm BufferSize = sizeof(Buffer);
    format_string_state StringState = BeginFormatStringToBuffer(Buffer);
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, BufferSize);
    
    u32 Result = 0;
    
    HANDLE OutputHandle = Win32GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    Win32WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&Result, 0);
    Assert(Result == Text.Size);
    
    return Result;
}

internal void
CTokenizerEatWhitespaceAndComments(c_tokenizer *Tokenizer)
{
    for(;;)
    {
        if(IsWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '/'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] && !IsEndOfLine(Tokenizer->At[0]))
            {
                ++Tokenizer->At;
            }
        }
        else if((Tokenizer->At[0] == '/') &&
                (Tokenizer->At[1] == '*'))
        {
            Tokenizer->At += 2;
            while(Tokenizer->At[0] &&
                  !((Tokenizer->At[0] == '*') &&
                    (Tokenizer->At[1] == '/')))
            {
                ++Tokenizer->At;
            }
            
            if(Tokenizer->At[0] == '*')
            {
                Tokenizer->At += 2;
            }
        }
        else
        {
            break;
        }
    }
}

internal c_token
GetNextCToken(c_tokenizer *Tokenizer)
{
    CTokenizerEatWhitespaceAndComments(Tokenizer);
    
    c_token Result;
    ZeroStruct(Result);
    
    Result.Str = String_(1, (u8 *)Tokenizer->At);
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    switch(C)
    {
        case '\0': { Result.Type = CToken_EndOfStream; } break;
        
        case '(': {Result.Type = CToken_OpenParen;} break;
        case ')': {Result.Type = CToken_CloseParen;} break;
        case ':': {Result.Type = CToken_Colon;} break;
        case ';': {Result.Type = CToken_Semicolon;} break;
        case '*': {Result.Type = CToken_Asterisk;} break;
        case '[': {Result.Type = CToken_OpenBracket;} break;
        case ']': {Result.Type = CToken_CloseBracket;} break;
        case '{': {Result.Type = CToken_OpenBrace;} break;
        case '}': {Result.Type = CToken_CloseBrace;} break;
        
        case '"':
        {
            Result.Type = CToken_String;
            
            Result.Str.Data = (u8 *)Tokenizer->At;
            
            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '"')
            {
                if((Tokenizer->At[0] == '\\') &&
                   Tokenizer->At[1])
                {
                    ++Tokenizer->At;
                }                
                ++Tokenizer->At;
            }
            
            Result.Str.Size = Tokenizer->At - (char *)Result.Str.Data;
            if(Tokenizer->At[0] == '"')
            {
                ++Tokenizer->At;
            }
        } break;
        
        default:
        {
            if(IsAlpha(C))
            {
                Result.Type = CToken_Identifier;
                
                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumber(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '_'))
                {
                    ++Tokenizer->At;
                }
                
                Result.Str.Size = Tokenizer->At - (char *)Result.Str.Data;
            }
#if 0
            else if(IsNumber(C))
            {
                ParseNumber();
            }
#endif
            else
            {
                Result.Type = CToken_Unknown;
            }
        } break;        
        
    }
    return Result;
}

inline b32
RequireCToken(c_tokenizer *Tokenizer, c_token_type DesiredType)
{
    c_token Token = GetNextCToken(Tokenizer);
    b32 Result = (Token.Type == DesiredType);
    return Result;
}

typedef enum
{
    GenerateMethod_Ctor,
    GenerateMethod_Set1,
    GenerateMethod_MathAdd,
    GenerateMethod_MathSubtract,
    GenerateMethod_MathMultiply,
    GenerateMethod_MathDivide,
} generate_method_op;

internal void
GenerateMethod(c_tokenizer *Tokenizer, generate_method_op Op)
{
    char *Start = Tokenizer->At;
    
    format_string_state StringState = BeginFormatString(Tokenizer->Arena);
    
    b32 FirstParam = true;
    c_token Token;
    for(;;)
    {
        Token = GetNextCToken(Tokenizer);
        if((Token.Type == CToken_EndOfStream) ||
           (Token.Type == CToken_CloseBrace))
        {
            Token = GetNextCToken(Tokenizer);
            break;
        }
        else if(Token.Type == CToken_Identifier)
        {
            b32 PrefixStruct = false;
            if(StringsAreEqual(String("struct"), Token.Str))
            {
                PrefixStruct = true;
                Token = GetNextCToken(Tokenizer);
            }
            string Type = Token.Str;
            Token = GetNextCToken(Tokenizer);
            b32 IsPointer = false;
            if(Token.Type == CToken_Asterisk)
            {
                IsPointer = true;
                Token = GetNextCToken(Tokenizer);
            }
            
            switch(Op)
            {
                case GenerateMethod_Ctor:
                {
                    if(!FirstParam)
                    {
                        AppendStringFormat(&StringState, ", ");
                    }
                    FirstParam = false;
                    if(IsPointer)
                    {
                        if(PrefixStruct)
                        {
                            AppendStringFormat(&StringState, "struct %S *%S", Type, Token.Str);
                        }
                        else
                        {
                            AppendStringFormat(&StringState, "%S *%S", Type, Token.Str);
                        }
                    }
                    
                    else
                    {
                        if(PrefixStruct)
                        {
                            AppendStringFormat(&StringState, "struct %S %S", Type, Token.Str);
                        }
                        else
                        {
                            AppendStringFormat(&StringState, "%S %S", Type, Token.Str);
                        }
                    }
                } break;
                case GenerateMethod_Set1:
                {
                    if(FirstParam)
                    {
                        FirstParam = false;
                        AppendStringFormat(&StringState, "%S Value", Type);
                    }
                } break;
                case GenerateMethod_MathAdd:
                case GenerateMethod_MathSubtract:
                case GenerateMethod_MathMultiply:
                case GenerateMethod_MathDivide:
                {
                } break;
                InvalidDefaultCase;
            }
        }
    }
    
    string Types = EndFormatString(&StringState);
    string SnakeStruct = Token.Str;
    string UpperCamelStruct = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    ToUpperCamelCase(&UpperCamelStruct);
    
    switch(Op)
    {
        case GenerateMethod_Ctor:
        {
            Win32PrintOut(Tokenizer->Arena, "inline %S\n%S(", SnakeStruct, UpperCamelStruct);
        } break;
        case GenerateMethod_Set1:
        {
            Win32PrintOut(Tokenizer->Arena, "inline %S\n%SSet1(", SnakeStruct, UpperCamelStruct);
        } break;
        case GenerateMethod_MathAdd:
        {
            Win32PrintOut(Tokenizer->Arena, "inline %S\n%SAdd(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathSubtract:
        {
            Win32PrintOut(Tokenizer->Arena, "inline %S\n%SSubtract(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathMultiply:
        {
            Win32PrintOut(Tokenizer->Arena, "inline %S\n%SMultiply(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathDivide:
        {
            Win32PrintOut(Tokenizer->Arena, "inline %S\n%SDivide(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        InvalidDefaultCase;
    }
    
    if((Op == GenerateMethod_Ctor) || 
       (Op == GenerateMethod_Set1))
    {
        Win32PrintOut(Tokenizer->Arena, "%S", Types);
    }
    
    Win32PrintOut(Tokenizer->Arena, ")\n{\n    %S Result;\n\n", SnakeStruct);
    
    Tokenizer->At = Start;
    
    for(;;)
    {
        Token = GetNextCToken(Tokenizer);
        if((Token.Type == CToken_EndOfStream) ||
           (Token.Type == CToken_CloseBrace))
        {
            Win32PrintOut(Tokenizer->Arena, "\n    return Result;\n}\n\n");
            break;
        }
        else if(Token.Type == CToken_Identifier)
        {
            if(StringsAreEqual(String("struct"), Token.Str))
            {
                Token = GetNextCToken(Tokenizer);
            }
            Token = GetNextCToken(Tokenizer);
            b32 IsPointer = false;
            if(Token.Type == CToken_Asterisk)
            {
                IsPointer = true;
                Token = GetNextCToken(Tokenizer);
            }
            
            string Var = Token.Str;
            switch(Op)
            {
                case GenerateMethod_Ctor:
                {
                    Win32PrintOut(Tokenizer->Arena, "    Result.%S = %S;\n", Var, Var);
                } break;
                case GenerateMethod_Set1:
                {
                    Win32PrintOut(Tokenizer->Arena, "    Result.%S = Value;\n", Var);
                } break;
                case GenerateMethod_MathAdd:
                {
                    Win32PrintOut(Tokenizer->Arena, "    Result.%S = A.%S + B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathSubtract:
                {
                    Win32PrintOut(Tokenizer->Arena, "    Result.%S = A.%S - B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathMultiply:
                {
                    Win32PrintOut(Tokenizer->Arena, "    Result.%S = A.%S * B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathDivide:
                {
                    Win32PrintOut(Tokenizer->Arena, "    Result.%S = A.%S / B.%S;\n", Var, Var, Var);
                } break;
                InvalidDefaultCase;
            }
        }
    }
}

internal void
GenerateLinkedList(c_tokenizer *Tokenizer)
{
    c_token Token = GetNextCToken(Tokenizer);
    Assert(StringsAreEqual(String("typedef"), Token.Str));
    Token = GetNextCToken(Tokenizer);
    Assert(StringsAreEqual(String("struct"), Token.Str));
    Token = GetNextCToken(Tokenizer);
    string Type = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    string FunctionName = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    ToUpperCamelCase(&FunctionName);
    
    Win32PrintOut(Tokenizer->Arena, "\ninline %S *\nGet%STail(%S *Head)\n", Type, FunctionName, Type);
    Win32PrintOut(Tokenizer->Arena, "{\n");
    Win32PrintOut(Tokenizer->Arena, "    %S *Result = Head;\n\n", Type);
    Win32PrintOut(Tokenizer->Arena, "    if(Result != 0)\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        while(Result->Next != 0)\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            Result = Result->Next;\n");
    Win32PrintOut(Tokenizer->Arena, "        }\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n\n");
    Win32PrintOut(Tokenizer->Arena, "    return Result;");
    Win32PrintOut(Tokenizer->Arena, "}\n");
    
    Win32PrintOut(Tokenizer->Arena, "\ninline %S *\n%SPush(%S **HeadRef, memory_arena *Arena)\n", Type, FunctionName, Type);
    Win32PrintOut(Tokenizer->Arena, "{\n");
    Win32PrintOut(Tokenizer->Arena, "    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    Win32PrintOut(Tokenizer->Arena, "    Result->Next = *HeadRef;\n");
    Win32PrintOut(Tokenizer->Arena, "    *HeadRef = Result;\n\n");
    Win32PrintOut(Tokenizer->Arena, "    return Result;");
    Win32PrintOut(Tokenizer->Arena, "}\n");
    
    Win32PrintOut(Tokenizer->Arena, "\ninline %S *\n%SPushBack(%S **HeadRef, memory_arena *Arena)\n", Type, FunctionName, Type);
    Win32PrintOut(Tokenizer->Arena, "{\n");
    Win32PrintOut(Tokenizer->Arena, "    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    Win32PrintOut(Tokenizer->Arena, "    Result->Next = 0;\n");
    Win32PrintOut(Tokenizer->Arena, "    if(*HeadRef == 0)\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        *HeadRef = Result;\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n");
    Win32PrintOut(Tokenizer->Arena, "    else\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        %S *Tail = Get%STail(*HeadRef);\n", Type, FunctionName);
    Win32PrintOut(Tokenizer->Arena, "        Tail->Next = Result;\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n\n");
    Win32PrintOut(Tokenizer->Arena, "    return Result;");
    Win32PrintOut(Tokenizer->Arena, "}\n");
    
    Win32PrintOut(Tokenizer->Arena, "\ntypedef b32 %S_predicate(%S *A, %S *B);\n", Type, Type, Type);
    
    
    Win32PrintOut(Tokenizer->Arena, "\ninline %S *\n%SMergeSort_(%S *Front, %S *Back, %S_predicate *Predicate, sort_type SortType)\n", 
                  Type, FunctionName, Type, Type, Type);
    Win32PrintOut(Tokenizer->Arena, "{\n");
    Win32PrintOut(Tokenizer->Arena, "    %S *Result = 0;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "    if(Front == 0)\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        Result = Back;\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n");
    Win32PrintOut(Tokenizer->Arena, "    else if(Back == 0)\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        Result = Front;\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n");
    Win32PrintOut(Tokenizer->Arena, "    else\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        b32 PredicateResult = Predicate(Front, Back);\n");
    Win32PrintOut(Tokenizer->Arena, "        if(SortType == Sort_Descending)\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            PredicateResult = !PredicateResult;\n");
    Win32PrintOut(Tokenizer->Arena, "        }\n");
    Win32PrintOut(Tokenizer->Arena, "        else\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            Assert(SortType == Sort_Ascending);\n");
    Win32PrintOut(Tokenizer->Arena, "        }\n");
    Win32PrintOut(Tokenizer->Arena, "        if(PredicateResult)\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            Result = Front;\n");
    Win32PrintOut(Tokenizer->Arena, "            Result->Next = %SMergeSort_(Front->Next, Back, Predicate, SortType);\n", FunctionName);
    Win32PrintOut(Tokenizer->Arena, "        }\n");
    Win32PrintOut(Tokenizer->Arena, "        else\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            Result = Back;\n");
    Win32PrintOut(Tokenizer->Arena, "            Back->Next = %SMergeSort_(Front, Back->Next, Predicate, SortType);\n", FunctionName);
    Win32PrintOut(Tokenizer->Arena, "        }\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n\n");
    Win32PrintOut(Tokenizer->Arena, "    return Result;\n");
    Win32PrintOut(Tokenizer->Arena, "}\n");
    
    
    Win32PrintOut(Tokenizer->Arena, "\ninline void\n%SFrontBackSplit(%S *Head, %S **FrontRef, %S **BackRef)\n", FunctionName, Type, Type);
    Win32PrintOut(Tokenizer->Arena, "{\n");
    Win32PrintOut(Tokenizer->Arena, "    %S *Fast;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "    %S *Slow;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "    Slow = Head;\n");
    Win32PrintOut(Tokenizer->Arena, "    Fast = Head->Next;\n\n");
    Win32PrintOut(Tokenizer->Arena, "    while(Fast != 0)\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        Fast = Fast->Next;\n");
    Win32PrintOut(Tokenizer->Arena, "        if(Fast != 0)\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            Slow = Slow->Next;\n");
    Win32PrintOut(Tokenizer->Arena, "            Fast = Fast->Next;\n");
    Win32PrintOut(Tokenizer->Arena, "        }\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n\n");
    Win32PrintOut(Tokenizer->Arena, "    *FrontRef = Head;\n");
    Win32PrintOut(Tokenizer->Arena, "    *BackRef = Slow->Next;\n");
    Win32PrintOut(Tokenizer->Arena, "    Slow->Next = 0;\n");
    Win32PrintOut(Tokenizer->Arena, "}\n");
    
    
    Win32PrintOut(Tokenizer->Arena, "\ninline void\n%SMergeSort(%S **HeadRef, %S_predicate *Predicate, sort_type SortType)\n", FunctionName, Type, Type);
    Win32PrintOut(Tokenizer->Arena, "{\n");
    Win32PrintOut(Tokenizer->Arena, "    %S *Head = *HeadRef;\n\n", Type);
    Win32PrintOut(Tokenizer->Arena, "    if((Head!= 0) &&\n");
    Win32PrintOut(Tokenizer->Arena, "       (Head->Next != 0))\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        %S *Front;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "        %S *Back;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "        %SFrontBackSplit(Head, &Front, &Back);\n\n", FunctionName);
    Win32PrintOut(Tokenizer->Arena, "        %SMergeSort(&Front, Predicate, SortType);\n", FunctionName);
    Win32PrintOut(Tokenizer->Arena, "        %SMergeSort(&Back, Predicate, SortType);\n\n", FunctionName);
    Win32PrintOut(Tokenizer->Arena, "        *HeadRef = %SMergeSort_(Front, Back, Predicate, SortType);\n", FunctionName);
    Win32PrintOut(Tokenizer->Arena, "    }\n");
    Win32PrintOut(Tokenizer->Arena, "}\n");
    
    Token = GetNextCToken(Tokenizer);
    while(Token.Type != CToken_CloseBrace)
    {
        Token = GetNextCToken(Tokenizer);
    }
    Token = GetNextCToken(Tokenizer);
}

internal void
GenerateFunctionPointer(c_tokenizer *Tokenizer, string Library, string Parameter)
{
    c_token Token = GetNextCToken(Tokenizer);
    Assert(StringsAreEqual(String("typedef"), Token.Str));
    Token = GetNextCToken(Tokenizer);
    string ReturnType = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    Token = GetNextCToken(Tokenizer);
    if(StringsAreEqual(Token.Str, String("WINAPI")))
    {
        Token = GetNextCToken(Tokenizer);
    }
    b32 TrimWsa = false;
    if(StringsAreEqual(Token.Str, String("WSAAPI")))
    {
        TrimWsa = true;
        Token = GetNextCToken(Tokenizer);
    }
    if(Token.Type == CToken_Asterisk)
    {
        ReturnType = FormatString(Tokenizer->Arena, "%S *", ReturnType);
        Token = GetNextCToken(Tokenizer);
    }
    string FunctionType = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    string FunctionName = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    string MethodName = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    if(TrimWsa)
    {
        FunctionName.Data += 4;
        FunctionName.Size -= 4;
        
        MethodName.Data += 4;
        MethodName.Size -= 4;
    }
    
    if(StringsAreEqual(String("lowercase"), Parameter))
    {
        SnakeToLowerCase(&MethodName);
    }
    else
    {
        ToUpperCamelCase(&MethodName);
    }
    ToUpperCamelCase(&FunctionName);
    if(StringsAreEqual(String("lowerCamelCase"), Parameter))
    {
        MethodName.Data[0] = CToLowercase(FunctionName.Data[0]);
        FunctionName.Data[0] = CToLowercase(FunctionName.Data[0]);
    }
    
    
    Token = GetNextCToken(Tokenizer);
    Assert(Token.Type == CToken_OpenParen);
    Win32PrintOut(Tokenizer->Arena, "\ninternal %S\n", ReturnType);
    Win32PrintOut(Tokenizer->Arena, "Win32%S(", FunctionName);
    Token = GetNextCToken(Tokenizer);
    b32 FirstParamFound = false;
    string ParametersWithoutTypes;
    ZeroStruct(ParametersWithoutTypes);
    if(Token.Type == CToken_CloseParen)
    {
        Token = GetNextCToken(Tokenizer);
    }
    else
    {
        format_string_state StringState = BeginFormatString(Tokenizer->Arena);
        while(Token.Type != CToken_CloseParen)
        {
            if(Token.Type == CToken_Identifier)
            {
                b32 IsConst = false;
                if(StringsAreEqual(Token.Str, String("const")))
                {
                    IsConst = true;
                    Token = GetNextCToken(Tokenizer);
                }
                b32 IsStruct = false;
                if(StringsAreEqual(Token.Str, String("struct")))
                {
                    IsStruct = true;
                    Token = GetNextCToken(Tokenizer);
                }
                string Type = Token.Str;
                Token = GetNextCToken(Tokenizer);
                b32 IsPointer = false;
                b32 IsPointerToPointer = false;
                b32 IsVolatile = false;
                if(StringsAreEqual(Token.Str, String("volatile")))
                {
                    IsVolatile = true;
                    Token = GetNextCToken(Tokenizer);
                }
                if(Token.Type == CToken_Asterisk)
                {
                    IsPointer = true;
                    Token = GetNextCToken(Tokenizer);
                }
                if(Token.Type == CToken_Asterisk)
                {
                    IsPointerToPointer = true;
                    Token = GetNextCToken(Tokenizer);
                }
                string Name = Token.Str;
                
                if(FirstParamFound)
                {
                    Win32PrintOut(Tokenizer->Arena, ", ");
                    AppendStringFormat(&StringState, ", ");
                }
                else
                {
                    FirstParamFound = true;
                }
                Win32PrintOut(Tokenizer->Arena, "%s%s%S%s %s%S", 
                              IsConst ? "const " : "",
                              IsStruct ? "struct " : "",
                              Type, 
                              IsVolatile ? " volatile" : "", 
                              IsPointer ? (IsPointerToPointer ? "**" : "*") : "", 
                              Name);
                
                AppendStringFormat(&StringState, "%S", Token.Str);
                
            }
            Token = GetNextCToken(Tokenizer);
        }
        ParametersWithoutTypes = EndFormatString(&StringState);
    }
    
    b32 HasResult = !StringsAreEqual(String("void"), ReturnType);
    
    Win32PrintOut(Tokenizer->Arena, ")\n{\n");
    if(HasResult)
    {
        Win32PrintOut(Tokenizer->Arena, "    %S Result;\n\n", ReturnType);
    }
    if(!StringsAreEqual(Library, String("Kernel32")))
    {
        Win32PrintOut(Tokenizer->Arena, "    if(!%S)\n", Library);
        Win32PrintOut(Tokenizer->Arena, "    {\n");
        Win32PrintOut(Tokenizer->Arena, "        %S = Win32LoadLibraryA(\"%S.dll\");\n", Library, Library);
        Win32PrintOut(Tokenizer->Arena, "    }\n");
    }
    Win32PrintOut(Tokenizer->Arena, "    Assert(%S);\n", Library);
    Win32PrintOut(Tokenizer->Arena, "    local_persist %S *Func = 0;\n", FunctionType);
    Win32PrintOut(Tokenizer->Arena, "    if(Func == 0)\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "         Func = (%S *)Win32GetProcAddressA(%S, \"%S\");\n", FunctionType, Library, MethodName);
    Win32PrintOut(Tokenizer->Arena, "    }\n");
    Win32PrintOut(Tokenizer->Arena, "    Assert(Func);\n");
    if(HasResult)
    {
        Win32PrintOut(Tokenizer->Arena, "    Result = Func(%S);\n\n", ParametersWithoutTypes);
        Win32PrintOut(Tokenizer->Arena, "    return Result;\n");
    }
    else
    {
        Win32PrintOut(Tokenizer->Arena, "    Func(%S);\n", ParametersWithoutTypes);
    }
    Win32PrintOut(Tokenizer->Arena, "}\n");
}

internal void
GenerateRadixSort(c_tokenizer *Tokenizer, string SortKey)
{
    c_token Token = GetNextCToken(Tokenizer);
    Assert(StringsAreEqual(String("typedef"), Token.Str));
    Token = GetNextCToken(Tokenizer);
    Assert(StringsAreEqual(String("struct"), Token.Str));
    Token = GetNextCToken(Tokenizer);
    string Type = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    string FunctionName = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
    ToUpperCamelCase(&FunctionName);
    
    Win32PrintOut(Tokenizer->Arena, "\ninternal void\n%SRadixSort(u32 Count, %S *First, %S *Temp, sort_type SortType)\n", FunctionName, Type, Type);
    Win32PrintOut(Tokenizer->Arena, "{\n");
    Win32PrintOut(Tokenizer->Arena, "    %S *Source = First;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "    %S *Dest = Temp;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "    for(u32 ByteIndex = 0;\n");
    Win32PrintOut(Tokenizer->Arena, "        ByteIndex < 32;\n");
    Win32PrintOut(Tokenizer->Arena, "        ByteIndex += 8)\n");
    Win32PrintOut(Tokenizer->Arena, "    {\n");
    Win32PrintOut(Tokenizer->Arena, "        u32 SortKeyOffsets[256];\n");
    Win32PrintOut(Tokenizer->Arena, "        ZeroArray(ArrayCount(SortKeyOffsets), SortKeyOffsets);\n\n");
    Win32PrintOut(Tokenizer->Arena, "        for(u32 Index = 0;\n");
    Win32PrintOut(Tokenizer->Arena, "            Index < Count;\n");
    Win32PrintOut(Tokenizer->Arena, "            ++Index)\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            u32 RadixValue;\n");
    Win32PrintOut(Tokenizer->Arena, "            if(SortType == Sort_Descending)\n");
    Win32PrintOut(Tokenizer->Arena, "            {\n");
    Win32PrintOut(Tokenizer->Arena, "                RadixValue = F32ToRadixValue(-Source[Index].%S);\n", SortKey);
    Win32PrintOut(Tokenizer->Arena, "            }\n");
    Win32PrintOut(Tokenizer->Arena, "            else\n");
    Win32PrintOut(Tokenizer->Arena, "            {\n");
    Win32PrintOut(Tokenizer->Arena, "                Assert(SortType == Sort_Ascending);\n");
    Win32PrintOut(Tokenizer->Arena, "                RadixValue = F32ToRadixValue(Source[Index].%S);\n", SortKey);
    Win32PrintOut(Tokenizer->Arena, "            }\n");
    Win32PrintOut(Tokenizer->Arena, "            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;\n");
    Win32PrintOut(Tokenizer->Arena, "            ++SortKeyOffsets[RadixPiece];\n");
    Win32PrintOut(Tokenizer->Arena, "        }\n\n");
    Win32PrintOut(Tokenizer->Arena, "        u32 Total = 0;\n");
    Win32PrintOut(Tokenizer->Arena, "        for(u32 SortKeyIndex = 0;\n");
    Win32PrintOut(Tokenizer->Arena, "            SortKeyIndex < ArrayCount(SortKeyOffsets);\n");
    Win32PrintOut(Tokenizer->Arena, "            ++SortKeyIndex)\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            u32 OffsetCount = SortKeyOffsets[SortKeyIndex];\n");
    Win32PrintOut(Tokenizer->Arena, "            SortKeyOffsets[SortKeyIndex] = Total;\n");
    Win32PrintOut(Tokenizer->Arena, "            Total += OffsetCount;\n");
    Win32PrintOut(Tokenizer->Arena, "        }\n\n");
    Win32PrintOut(Tokenizer->Arena, "        \n");
    Win32PrintOut(Tokenizer->Arena, "        for(u32 Index = 0;\n");
    Win32PrintOut(Tokenizer->Arena, "            Index < Count;\n");
    Win32PrintOut(Tokenizer->Arena, "            ++Index)\n");
    Win32PrintOut(Tokenizer->Arena, "        {\n");
    Win32PrintOut(Tokenizer->Arena, "            u32 RadixValue;\n");
    Win32PrintOut(Tokenizer->Arena, "            if(SortType == Sort_Descending)\n");
    Win32PrintOut(Tokenizer->Arena, "            {\n");
    Win32PrintOut(Tokenizer->Arena, "                RadixValue = F32ToRadixValue(-Source[Index].%S);\n", SortKey);
    Win32PrintOut(Tokenizer->Arena, "            }\n");
    Win32PrintOut(Tokenizer->Arena, "            else\n");
    Win32PrintOut(Tokenizer->Arena, "            {\n");
    Win32PrintOut(Tokenizer->Arena, "                Assert(SortType == Sort_Ascending);\n");
    Win32PrintOut(Tokenizer->Arena, "                RadixValue = F32ToRadixValue(Source[Index].%S);\n", SortKey);
    Win32PrintOut(Tokenizer->Arena, "            }\n");
    Win32PrintOut(Tokenizer->Arena, "            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;\n");
    Win32PrintOut(Tokenizer->Arena, "            Dest[SortKeyOffsets[RadixPiece]++] = Source[Index];\n");
    Win32PrintOut(Tokenizer->Arena, "        }\n\n");
    Win32PrintOut(Tokenizer->Arena, "        %S *SwapTemp = Dest;\n", Type);
    Win32PrintOut(Tokenizer->Arena, "        Dest = Source;\n");
    Win32PrintOut(Tokenizer->Arena, "        Source = SwapTemp;\n");
    Win32PrintOut(Tokenizer->Arena, "    }\n");
    Win32PrintOut(Tokenizer->Arena, "}\n");
    
    Token = GetNextCToken(Tokenizer);
    while(Token.Type != CToken_CloseBrace)
    {
        Token = GetNextCToken(Tokenizer);
    }
    Token = GetNextCToken(Tokenizer);
}

internal void
ParseIntrospectable_(c_tokenizer *Tokenizer)
{
    if(RequireCToken(Tokenizer, CToken_OpenParen))
    {
        // TODO(kstandbridge): This is pretty horrible, surely theres a better way to pickup keywords?
        b32 IsCtor = false;
        b32 IsSet1 = false;
        b32 IsMath = false;
        b32 IsWin32 = false;
        b32 IsLinkedList = false;
        b32 IsRadix = false;
        string Parameter;
        ZeroStruct(Parameter);
        string Parameter2;
        ZeroStruct(Parameter2);
        
        // NOTE(kstandbridge): Parse params
        c_token Token;
        ZeroStruct(Token);
        while((Token.Type != CToken_EndOfStream) &&
              (Token.Type != CToken_CloseParen))
        {
            Token = GetNextCToken(Tokenizer);
            if(Token.Type == CToken_Identifier)
            {
                if(StringsAreEqual(String("ctor"), Token.Str))
                {
                    IsCtor = true;
                }
                else if(StringsAreEqual(String("set1"), Token.Str))
                {
                    IsSet1 = true;
                }
                else if(StringsAreEqual(String("math"), Token.Str))
                {
                    IsMath = true;
                }
                else if(StringsAreEqual(String("win32"), Token.Str))
                {
                    IsWin32 = true;
                }
                else if(StringsAreEqual(String("linked_list"), Token.Str))
                {
                    IsLinkedList = true;
                }
                else if(StringsAreEqual(String("radix"), Token.Str))
                {
                    IsRadix = true;
                }
                else
                {
                    if(!Parameter.Data)
                    {
                        Parameter = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
                    }
                    else
                    {
                        Parameter2 = PushString_(Tokenizer->Arena, Token.Str.Size, Token.Str.Data);
                    }
                }
            }
        }
        
        if(IsWin32)
        {
            GenerateFunctionPointer(Tokenizer, Parameter, Parameter2);
        }
        else if(IsLinkedList)
        {
            GenerateLinkedList(Tokenizer);
        }
        else if(IsRadix)
        {
            GenerateRadixSort(Tokenizer, Parameter);
        }
        else
        {
            char *Start = 0;
            Token = GetNextCToken(Tokenizer);
            Assert(StringsAreEqual(String("typedef"), Token.Str));
            Token = GetNextCToken(Tokenizer);
            Assert(StringsAreEqual(String("struct"), Token.Str));
            Token = GetNextCToken(Tokenizer);
            if(Token.Type != CToken_OpenBrace)
            {
                Token = GetNextCToken(Tokenizer);
            }
            if(Token.Type == CToken_OpenBrace)
            {
                Start = Tokenizer->At;
                
                if(IsCtor)
                {
                    GenerateMethod(Tokenizer, GenerateMethod_Ctor);
                }
                
                if(IsSet1)
                {
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_Set1);
                }
                
                if(IsMath)
                {
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathAdd);
                    
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathSubtract);
                    
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathMultiply);
                    
                    Tokenizer->At = Start;
                    GenerateMethod(Tokenizer, GenerateMethod_MathDivide);
                }
            }
            else
            {
                // TODO(kstandbridge): Error missing open brace on introspect
            }
        }
    }
    else
    {
        // TODO(kstandbridge): Error missing paren on introspect
    }
}

internal void
ParseIntrospectable(memory_arena *Arena, string File)
{
    c_tokenizer Tokenizer;
    Tokenizer.Arena = Arena;
    Tokenizer.At = (char *)File.Data;
    
    b32 Parsing = true;
    while(Parsing)
    {
        c_token Token = GetNextCToken(&Tokenizer);
        switch(Token.Type)
        {
            case CToken_EndOfStream:
            {
                Parsing = false;
            } break;
            
            case CToken_Unknown:
            {
            } break;
            
            case CToken_Identifier:
            {
                if(StringsAreEqual(String("introspect"), Token.Str))
                {
                    ParseIntrospectable_(&Tokenizer);
                }
            } break;
        }
    }
    
}

s32 __stdcall
mainCRTStartup()
{
    Kernel32 = FindModuleBase(_ReturnAddress());
    Assert(Kernel32);
    
#if KENGINE_INTERNAL
    void *BaseAddress = (void *)Terabytes(2);
#else
    void *BaseAddress = 0;
#endif
    
    u64 MemoryBlockSize = Megabytes(16);
    void *MemoryBlock = Win32VirtualAlloc(BaseAddress, MemoryBlockSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    Assert(MemoryBlock);
    
    memory_arena Arena_;
    memory_arena *Arena = &Arena_;
    InitializeArena(Arena, MemoryBlockSize, MemoryBlock);
    
    char *CommandLingArgs = Win32GetCommandLineA();
    Assert(CommandLingArgs);
    
    char *At = CommandLingArgs;
    char *FileName = At;
    u32 FileNameLength = 0;
    b32 Parsing = true;
    b32 ExeNameFound = false;
    while(Parsing)
    {
        if((*At == '\0') || IsWhitespace(*At))
        {
            if(*At != '\0')
            {
                while(IsWhitespace(*At))
                {
                    ++At;
                }
            }
            else
            {
                Parsing = false;
            }
            
            if(ExeNameFound)
            {            
                char FilePath[256];
                Copy(FileNameLength, FileName, FilePath);
                FilePath[FileNameLength] = '\0';
                
                string SourceFile = Win32ReadEntireFile(Arena, FilePath);
                Assert(SourceFile.Data);
                if(SourceFile.Data)
                {
                    ParseIntrospectable(Arena, SourceFile);
                }
            }
            else
            {
                ExeNameFound = true;
            }
            
            FileName = At;
            FileNameLength = 1;
            ++At;
        }
        else
        {
            ++FileNameLength;
            ++At;
        }
        
    }
    
    Win32ExitProcess(0);
    
    InvalidCodePath;
    
    return 0;
}
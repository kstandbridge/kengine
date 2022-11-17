#include "win32_kengine_preprocessor.h"
#include "win32_kengine_kernel.c"

platform_api Platform;

internal b32
Win32PrintOutput(char *Format, ...)
{
    u8 Buffer[4096];
    format_string_state StringState = BeginFormatString();
    
    va_list ArgList;
    va_start(ArgList, Format);
    AppendFormatString_(&StringState, Format, ArgList);
    va_end(ArgList);
    
    string Text = EndFormatStringToBuffer(&StringState, Buffer, sizeof(Buffer));
    
    u32 Result = 0;
    
    HANDLE OutputHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    Assert(OutputHandle != INVALID_HANDLE_VALUE);
    
    WriteFile(OutputHandle, Text.Data, (DWORD)Text.Size, (LPDWORD)&Result, 0);
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
    
    u8 Buffer[4096];
    format_string_state StringState = BeginFormatString();
    
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
                        AppendFormatString(&StringState, ", ");
                    }
                    FirstParam = false;
                    if(IsPointer)
                    {
                        if(PrefixStruct)
                        {
                            AppendFormatString(&StringState, "struct %S *%S", Type, Token.Str);
                        }
                        else
                        {
                            AppendFormatString(&StringState, "%S *%S", Type, Token.Str);
                        }
                    }
                    
                    else
                    {
                        if(PrefixStruct)
                        {
                            AppendFormatString(&StringState, "struct %S %S", Type, Token.Str);
                        }
                        else
                        {
                            AppendFormatString(&StringState, "%S %S", Type, Token.Str);
                        }
                    }
                } break;
                case GenerateMethod_Set1:
                {
                    if(FirstParam)
                    {
                        FirstParam = false;
                        AppendFormatString(&StringState, "%S Value", Type);
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
    
    string Types = EndFormatStringToBuffer(&StringState, Buffer, sizeof(Buffer));
    string SnakeStruct = Token.Str;
    u8 UpperCamelStructBuffer[256];
    string UpperCamelStruct = FormatStringToBuffer(UpperCamelStructBuffer, sizeof(UpperCamelStructBuffer), "%S", Token.Str);
    ToUpperCamelCase(&UpperCamelStruct);
    
    switch(Op)
    {
        case GenerateMethod_Ctor:
        {
            Win32PrintOutput("inline %S\n%S(", SnakeStruct, UpperCamelStruct);
        } break;
        case GenerateMethod_Set1:
        {
            Win32PrintOutput("inline %S\n%SSet1(", SnakeStruct, UpperCamelStruct);
        } break;
        case GenerateMethod_MathAdd:
        {
            Win32PrintOutput("inline %S\n%SAdd(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathSubtract:
        {
            Win32PrintOutput("inline %S\n%SSubtract(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathMultiply:
        {
            Win32PrintOutput("inline %S\n%SMultiply(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        case GenerateMethod_MathDivide:
        {
            Win32PrintOutput("inline %S\n%SDivide(%S A, %S B", SnakeStruct, UpperCamelStruct, SnakeStruct, SnakeStruct);
        } break;
        InvalidDefaultCase;
    }
    
    if((Op == GenerateMethod_Ctor) || 
       (Op == GenerateMethod_Set1))
    {
        Win32PrintOutput("%S", Types);
    }
    
    Win32PrintOutput(")\n{\n    %S Result;\n\n", SnakeStruct);
    
    Tokenizer->At = Start;
    
    for(;;)
    {
        Token = GetNextCToken(Tokenizer);
        if((Token.Type == CToken_EndOfStream) ||
           (Token.Type == CToken_CloseBrace))
        {
            Win32PrintOutput("\n    return Result;\n}\n\n");
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
                    Win32PrintOutput("    Result.%S = %S;\n", Var, Var);
                } break;
                case GenerateMethod_Set1:
                {
                    Win32PrintOutput("    Result.%S = Value;\n", Var);
                } break;
                case GenerateMethod_MathAdd:
                {
                    Win32PrintOutput("    Result.%S = A.%S + B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathSubtract:
                {
                    Win32PrintOutput("    Result.%S = A.%S - B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathMultiply:
                {
                    Win32PrintOutput("    Result.%S = A.%S * B.%S;\n", Var, Var, Var);
                } break;
                case GenerateMethod_MathDivide:
                {
                    Win32PrintOutput("    Result.%S = A.%S / B.%S;\n", Var, Var, Var);
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
    u8 TypeBuffer[256];
    string Type = FormatStringToBuffer(TypeBuffer, sizeof(TypeBuffer), "%S", Token.Str);
    u8 FunctionNameBuffer[256];
    string FunctionName = FormatStringToBuffer(FunctionNameBuffer, sizeof(FunctionNameBuffer), "%S", Token.Str);
    ToUpperCamelCase(&FunctionName);
    
    Win32PrintOutput("\ninline %S *\nGet%STail(%S *Head)\n", Type, FunctionName, Type);
    Win32PrintOutput("{\n");
    Win32PrintOutput("    %S *Result = Head;\n\n", Type);
    Win32PrintOutput("    if(Result != 0)\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        while(Result->Next != 0)\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            Result = Result->Next;\n");
    Win32PrintOutput("        }\n");
    Win32PrintOutput("    }\n\n");
    Win32PrintOutput("    return Result;\n");
    Win32PrintOutput("}\n");
    
    Win32PrintOutput("\ninline %S *\n%SPush(%S **HeadRef, memory_arena *Arena)\n", Type, FunctionName, Type);
    Win32PrintOutput("{\n");
    Win32PrintOutput("    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    Win32PrintOutput("    Result->Next = *HeadRef;\n");
    Win32PrintOutput("    *HeadRef = Result;\n\n");
    Win32PrintOutput("    return Result;\n");
    Win32PrintOutput("}\n");
    
    Win32PrintOutput("\ninline %S *\n%SPushBack(%S **HeadRef, memory_arena *Arena)\n", Type, FunctionName, Type);
    Win32PrintOutput("{\n");
    Win32PrintOutput("    %S *Result = PushStruct(Arena, %S);\n\n", Type, Type);
    Win32PrintOutput("    Result->Next = 0;\n");
    Win32PrintOutput("    if(*HeadRef == 0)\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        *HeadRef = Result;\n");
    Win32PrintOutput("    }\n");
    Win32PrintOutput("    else\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        %S *Tail = Get%STail(*HeadRef);\n", Type, FunctionName);
    Win32PrintOutput("        Tail->Next = Result;\n");
    Win32PrintOutput("    }\n\n");
    Win32PrintOutput("    return Result;\n");
    Win32PrintOutput("}\n");
    
    Win32PrintOutput("\ntypedef b32 %S_predicate(%S *A, %S *B);\n", Type, Type, Type);
    
    
    Win32PrintOutput("\ninline %S *\n%SMergeSort_(%S *Front, %S *Back, %S_predicate *Predicate, sort_type SortType)\n", 
                     Type, FunctionName, Type, Type, Type);
    Win32PrintOutput("{\n");
    Win32PrintOutput("    %S *Result = 0;\n", Type);
    Win32PrintOutput("    if(Front == 0)\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        Result = Back;\n");
    Win32PrintOutput("    }\n");
    Win32PrintOutput("    else if(Back == 0)\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        Result = Front;\n");
    Win32PrintOutput("    }\n");
    Win32PrintOutput("    else\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        b32 PredicateResult = Predicate(Front, Back);\n");
    Win32PrintOutput("        if(SortType == Sort_Descending)\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            PredicateResult = !PredicateResult;\n");
    Win32PrintOutput("        }\n");
    Win32PrintOutput("        else\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            Assert(SortType == Sort_Ascending);\n");
    Win32PrintOutput("        }\n");
    Win32PrintOutput("        if(PredicateResult)\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            Result = Front;\n");
    Win32PrintOutput("            Result->Next = %SMergeSort_(Front->Next, Back, Predicate, SortType);\n", FunctionName);
    Win32PrintOutput("        }\n");
    Win32PrintOutput("        else\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            Result = Back;\n");
    Win32PrintOutput("            Back->Next = %SMergeSort_(Front, Back->Next, Predicate, SortType);\n", FunctionName);
    Win32PrintOutput("        }\n");
    Win32PrintOutput("    }\n\n");
    Win32PrintOutput("    return Result;\n");
    Win32PrintOutput("}\n");
    
    
    Win32PrintOutput("\ninline void\n%SFrontBackSplit(%S *Head, %S **FrontRef, %S **BackRef)\n", FunctionName, Type, Type);
    Win32PrintOutput("{\n");
    Win32PrintOutput("    %S *Fast;\n", Type);
    Win32PrintOutput("    %S *Slow;\n", Type);
    Win32PrintOutput("    Slow = Head;\n");
    Win32PrintOutput("    Fast = Head->Next;\n\n");
    Win32PrintOutput("    while(Fast != 0)\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        Fast = Fast->Next;\n");
    Win32PrintOutput("        if(Fast != 0)\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            Slow = Slow->Next;\n");
    Win32PrintOutput("            Fast = Fast->Next;\n");
    Win32PrintOutput("        }\n");
    Win32PrintOutput("    }\n\n");
    Win32PrintOutput("    *FrontRef = Head;\n");
    Win32PrintOutput("    *BackRef = Slow->Next;\n");
    Win32PrintOutput("    Slow->Next = 0;\n");
    Win32PrintOutput("}\n");
    
    
    Win32PrintOutput("\ninline void\n%SMergeSort(%S **HeadRef, %S_predicate *Predicate, sort_type SortType)\n", FunctionName, Type, Type);
    Win32PrintOutput("{\n");
    Win32PrintOutput("    %S *Head = *HeadRef;\n\n", Type);
    Win32PrintOutput("    if((Head!= 0) &&\n");
    Win32PrintOutput("       (Head->Next != 0))\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        %S *Front;\n", Type);
    Win32PrintOutput("        %S *Back;\n", Type);
    Win32PrintOutput("        %SFrontBackSplit(Head, &Front, &Back);\n\n", FunctionName);
    Win32PrintOutput("        %SMergeSort(&Front, Predicate, SortType);\n", FunctionName);
    Win32PrintOutput("        %SMergeSort(&Back, Predicate, SortType);\n\n", FunctionName);
    Win32PrintOutput("        *HeadRef = %SMergeSort_(Front, Back, Predicate, SortType);\n", FunctionName);
    Win32PrintOutput("    }\n");
    Win32PrintOutput("}\n");
    
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
    u8 ReturnTypeBuffer[256];
    string ReturnType = FormatStringToBuffer(ReturnTypeBuffer, sizeof(ReturnTypeBuffer), "%S", Token.Str);
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
        ReturnType = FormatStringToBuffer(ReturnTypeBuffer, sizeof(ReturnTypeBuffer), "%S *", ReturnType);
        Token = GetNextCToken(Tokenizer);
    }
    u8 FunctionTypeBuffer[256];
    string FunctionType = FormatStringToBuffer(FunctionTypeBuffer, sizeof(FunctionTypeBuffer), "%S", Token.Str);
    u8 FunctionNameBuffer[256];
    string FunctionName = FormatStringToBuffer(FunctionNameBuffer, sizeof(FunctionNameBuffer), "%S", Token.Str);
    u8 MethodNameBuffer[256];
    string MethodName = FormatStringToBuffer(MethodNameBuffer, sizeof(MethodNameBuffer), "%S", Token.Str);
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
    Win32PrintOutput("\ninternal %S\n", ReturnType);
    Win32PrintOutput("Win32%S(", FunctionName);
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
        u8 Buffer[4096];
        format_string_state StringState = BeginFormatString();
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
                    Win32PrintOutput(", ");
                    AppendFormatString(&StringState, ", ");
                }
                else
                {
                    FirstParamFound = true;
                }
                Win32PrintOutput("%s%s%S%s %s%S", 
                                 IsConst ? "const " : "",
                                 IsStruct ? "struct " : "",
                                 Type, 
                                 IsVolatile ? " volatile" : "", 
                                 IsPointer ? (IsPointerToPointer ? "**" : "*") : "", 
                                 Name);
                
                AppendFormatString(&StringState, "%S", Token.Str);
                
            }
            Token = GetNextCToken(Tokenizer);
        }
        ParametersWithoutTypes = EndFormatStringToBuffer(&StringState, Buffer, sizeof(Buffer));
    }
    
    b32 HasResult = !StringsAreEqual(String("void"), ReturnType);
    
    Win32PrintOutput(")\n{\n");
    if(HasResult)
    {
        Win32PrintOutput("    %S Result;\n\n", ReturnType);
    }
    if(!StringsAreEqual(Library, String("Kernel32")))
    {
        Win32PrintOutput("    if(!%S)\n", Library);
        Win32PrintOutput("    {\n");
        Win32PrintOutput("        %S = LoadLibraryA(\"%S.dll\");\n", Library, Library);
        Win32PrintOutput("    }\n");
    }
    Win32PrintOutput("    Assert(%S);\n", Library);
    Win32PrintOutput("    local_persist %S *Func = 0;\n", FunctionType);
    Win32PrintOutput("    if(!Func)\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("         Func = (%S *)GetProcAddress(%S, \"%S\");\n", FunctionType, Library, MethodName);
    Win32PrintOutput("    }\n");
    Win32PrintOutput("    Assert(Func);\n");
    if(HasResult)
    {
        Win32PrintOutput("    Result = Func(%S);\n\n", ParametersWithoutTypes);
        Win32PrintOutput("    return Result;\n");
    }
    else
    {
        Win32PrintOutput("    Func(%S);\n", ParametersWithoutTypes);
    }
    Win32PrintOutput("}\n");
}

internal void
GenerateRadixSort(c_tokenizer *Tokenizer, string SortKey)
{
    c_token Token = GetNextCToken(Tokenizer);
    Assert(StringsAreEqual(String("typedef"), Token.Str));
    Token = GetNextCToken(Tokenizer);
    Assert(StringsAreEqual(String("struct"), Token.Str));
    Token = GetNextCToken(Tokenizer);
    u8 TypeBuffer[256];
    string Type = FormatStringToBuffer(TypeBuffer, sizeof(TypeBuffer), "%S", Token.Str);
    u8 FunctionNameBuffer[256];
    string FunctionName = FormatStringToBuffer(FunctionNameBuffer, sizeof(FunctionNameBuffer), "%S", Token.Str);
    ToUpperCamelCase(&FunctionName);
    
    Win32PrintOutput("\ninternal void\n%SRadixSort(u32 Count, %S *First, %S *Temp, sort_type SortType)\n", FunctionName, Type, Type);
    Win32PrintOutput("{\n");
    Win32PrintOutput("    %S *Source = First;\n", Type);
    Win32PrintOutput("    %S *Dest = Temp;\n", Type);
    Win32PrintOutput("    for(u32 ByteIndex = 0;\n");
    Win32PrintOutput("        ByteIndex < 32;\n");
    Win32PrintOutput("        ByteIndex += 8)\n");
    Win32PrintOutput("    {\n");
    Win32PrintOutput("        u32 SortKeyOffsets[256];\n");
    Win32PrintOutput("        ZeroArray(ArrayCount(SortKeyOffsets), SortKeyOffsets);\n\n");
    Win32PrintOutput("        for(u32 Index = 0;\n");
    Win32PrintOutput("            Index < Count;\n");
    Win32PrintOutput("            ++Index)\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            u32 RadixValue;\n");
    Win32PrintOutput("            if(SortType == Sort_Descending)\n");
    Win32PrintOutput("            {\n");
    Win32PrintOutput("                RadixValue = F32ToRadixValue(-Source[Index].%S);\n", SortKey);
    Win32PrintOutput("            }\n");
    Win32PrintOutput("            else\n");
    Win32PrintOutput("            {\n");
    Win32PrintOutput("                Assert(SortType == Sort_Ascending);\n");
    Win32PrintOutput("                RadixValue = F32ToRadixValue(Source[Index].%S);\n", SortKey);
    Win32PrintOutput("            }\n");
    Win32PrintOutput("            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;\n");
    Win32PrintOutput("            ++SortKeyOffsets[RadixPiece];\n");
    Win32PrintOutput("        }\n\n");
    Win32PrintOutput("        u32 Total = 0;\n");
    Win32PrintOutput("        for(u32 SortKeyIndex = 0;\n");
    Win32PrintOutput("            SortKeyIndex < ArrayCount(SortKeyOffsets);\n");
    Win32PrintOutput("            ++SortKeyIndex)\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            u32 OffsetCount = SortKeyOffsets[SortKeyIndex];\n");
    Win32PrintOutput("            SortKeyOffsets[SortKeyIndex] = Total;\n");
    Win32PrintOutput("            Total += OffsetCount;\n");
    Win32PrintOutput("        }\n\n");
    Win32PrintOutput("        \n");
    Win32PrintOutput("        for(u32 Index = 0;\n");
    Win32PrintOutput("            Index < Count;\n");
    Win32PrintOutput("            ++Index)\n");
    Win32PrintOutput("        {\n");
    Win32PrintOutput("            u32 RadixValue;\n");
    Win32PrintOutput("            if(SortType == Sort_Descending)\n");
    Win32PrintOutput("            {\n");
    Win32PrintOutput("                RadixValue = F32ToRadixValue(-Source[Index].%S);\n", SortKey);
    Win32PrintOutput("            }\n");
    Win32PrintOutput("            else\n");
    Win32PrintOutput("            {\n");
    Win32PrintOutput("                Assert(SortType == Sort_Ascending);\n");
    Win32PrintOutput("                RadixValue = F32ToRadixValue(Source[Index].%S);\n", SortKey);
    Win32PrintOutput("            }\n");
    Win32PrintOutput("            u32 RadixPiece = (RadixValue >> ByteIndex) & 0xFF;\n");
    Win32PrintOutput("            Dest[SortKeyOffsets[RadixPiece]++] = Source[Index];\n");
    Win32PrintOutput("        }\n\n");
    Win32PrintOutput("        %S *SwapTemp = Dest;\n", Type);
    Win32PrintOutput("        Dest = Source;\n");
    Win32PrintOutput("        Source = SwapTemp;\n");
    Win32PrintOutput("    }\n");
    Win32PrintOutput("}\n");
    
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
        u8 ParameterBuffer[256];
        string Parameter;
        ZeroStruct(Parameter);
        u8 Parameter2Buffer[256];
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
                        Parameter = FormatStringToBuffer(ParameterBuffer, sizeof(ParameterBuffer), "%S", Token.Str);
                    }
                    else
                    {
                        Parameter2 = FormatStringToBuffer(Parameter2Buffer, sizeof(Parameter2Buffer), "%S", Token.Str);
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
                Assert(!"Missing open brace on introspect");
                
            }
        }
    }
    else
    {
        Assert(!"Missing paren on introspect");
    }
}

internal void
ParseIntrospectable(string File)
{
    c_tokenizer Tokenizer;
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
    memory_arena Arena_;
    ZeroStruct(Arena_);
    memory_arena *Arena = &Arena_;
    
    char *CommandLingArgs = GetCommandLineA();
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
                
                string SourceFile;
                ZeroStruct(SourceFile);
                
                HANDLE FileHandle = CreateFileA(FilePath, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
                Assert(FileHandle != INVALID_HANDLE_VALUE);
                
                if(FileHandle != INVALID_HANDLE_VALUE)
                {
                    LARGE_INTEGER FileSize;
                    b32 ReadResult = GetFileSizeEx(FileHandle, &FileSize);
                    Assert(ReadResult);
                    if(ReadResult)
                    {    
                        SourceFile.Size = FileSize.QuadPart;
                        SourceFile.Data = VirtualAlloc(0, SourceFile.Size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
                        Assert(SourceFile.Data);
                        
                        if(SourceFile.Data)
                        {
                            u32 BytesRead;
                            ReadResult = ReadFile(FileHandle, SourceFile.Data, (u32)SourceFile.Size, (LPDWORD)&BytesRead, 0);
                            Assert(ReadResult);
                            Assert(BytesRead == SourceFile.Size);
                        }
                    }
                    
                    CloseHandle(FileHandle);
                }
                
                Assert(SourceFile.Data);
                if(SourceFile.Data)
                {
                    ParseIntrospectable(SourceFile);
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
    
    return 0;
}
#ifndef KENGINE_XML_TOKENIZER_H

typedef enum token_type
{
    Token_Unknown,
    
    Token_Spacing,
    Token_EndOfLine,
    Token_Comment,
    
    Token_OpenAngleBracket,
    Token_CloseAngleBracket,
    Token_Equals,
    Token_ForwardSlash,
    Token_QuestionMark,
    Token_ExcalationMark,
    Token_Dash,
    
    Token_Identifier,
    
    Token_String,
    
    Token_EndOfStream
} token_type;

inline string
GetTokenTypeName(token_type Type)
{
    string Result = {0};
    
    switch(Type)
    {
        case Token_Unknown:           {Result = String("unknown");} break;
        
        case Token_Spacing:           {Result = String("spacing");} break;
        case Token_EndOfLine:         {Result = String("end of line");} break;
        case Token_Comment:           {Result = String("comment");} break;
        
        case Token_OpenAngleBracket:  {Result = String("open angle bracket");} break;
        case Token_CloseAngleBracket: {Result = String("close angle bracket");} break;
        case Token_Equals:            {Result = String("equals");} break;
        case Token_ForwardSlash:      {Result = String("forward slash");} break;
        case Token_QuestionMark:      {Result = String("question mark");} break;
        case Token_ExcalationMark:    {Result = String("excalation mark");} break;
        case Token_Dash:              {Result = String("dash");} break;
        
        case Token_Identifier:        {Result = String("identifier");} break;
        
        case Token_String:            {Result = String("string");} break;
        
        case Token_EndOfStream:       {Result = String("end of stream");} break;
        
        InvalidDefaultCase;
    }
    
    return Result;
}

typedef struct tokenizer
{
    string FileName;
    s32 ColumnNumber;
    s32 LineNumber;
    
    string FileData;
    char At[2];
    
    b32 HasErrors;
} tokenizer;

typedef struct token
{
    string FileName;
    s32 ColumnNumber;
    s32 LineNumber;
    
    token_type Type;
    string Text;
    
#if 0    
    // TODO(kstandbridge): Parse numbers
    f32 F32;
    s32 S32;
#endif
    
} token;

inline void
TokenizerAdvance(tokenizer *Tokenizer, u32 Count)
{
    Tokenizer->ColumnNumber += Count;
    StringAdvance(&Tokenizer->FileData, Count);
    
    if(Tokenizer->FileData.Size == 0)
    {
        Tokenizer->At[0] = 0;
        Tokenizer->At[1] = 0;
    }
    else if(Tokenizer->FileData.Size == 1)
    {
        Tokenizer->At[0] = Tokenizer->FileData.Data[0];
        Tokenizer->At[1] = 0;
    }
    else
    {
        Tokenizer->At[0] = Tokenizer->FileData.Data[0];
        Tokenizer->At[1] = Tokenizer->FileData.Data[1];
    }
}

internal token
GetToken_(tokenizer *Tokenizer)
{
    token Result =
    {
        .FileName = Tokenizer->FileName,
        .ColumnNumber = Tokenizer->ColumnNumber,
        .LineNumber = Tokenizer->LineNumber,
        .Text = Tokenizer->FileData
    };
    
    char C = Tokenizer->At[0];
    TokenizerAdvance(Tokenizer, 1);
    switch(C)
    {
        case '\0':{Result.Type = Token_EndOfStream;} break;
        case '<': {Result.Type = Token_OpenAngleBracket;} break;
        case '>': {Result.Type = Token_CloseAngleBracket;} break;
        case '=': {Result.Type = Token_Equals;} break;
        case '/': {Result.Type = Token_ForwardSlash;} break;
        case '?': {Result.Type = Token_QuestionMark;} break;
        case '!': {Result.Type = Token_ExcalationMark;} break;
        case '-': {Result.Type = Token_Dash;} break;
        
        case '"': 
        {
            Result.Type = Token_String;
            
            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '"')
            {
                if((Tokenizer->At[0] == '\\') &&
                   Tokenizer->At[1])
                {
                    TokenizerAdvance(Tokenizer, 1);
                }
                TokenizerAdvance(Tokenizer, 1);
            }
            
            if(Tokenizer->At[0] == '"')
            {
                TokenizerAdvance(Tokenizer, 1);
            }
            
        } break;
        
        default:
        {
            
            if(IsSpacing(C))
            {
                Result.Type = Token_Spacing;
                while(IsSpacing(Tokenizer->At[0]))
                {
                    TokenizerAdvance(Tokenizer, 1);
                }
            }
            else if(IsEndOfLine(C))
            {
                Result.Type = Token_EndOfLine;
                if(((C == '\r') &&
                    (Tokenizer->At[0] == '\n')) ||
                   ((C == '\n') &&
                    (Tokenizer->At[0] == '\r')))
                {
                    TokenizerAdvance(Tokenizer, 1);
                }
                
                Tokenizer->ColumnNumber = 1;
                ++Tokenizer->LineNumber;
            }
            else if(IsAlpha(C))
            {
                Result.Type = Token_Identifier;
                
                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumber(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '_'))
                {
                    TokenizerAdvance(Tokenizer, 1);
                }
            }
            else
            {
                Result.Type = Token_Unknown;
            }
            
        } break;
    }
    
    Result.Text.Size = (Tokenizer->FileData.Data - Result.Text.Data);
    
    return Result;
    
}

internal token
GetToken(tokenizer *Tokenizer)
{
    token Result;
    
    for(;;)
    {
        Result = GetToken_(Tokenizer);
        if((Result.Type == Token_Spacing) ||
           (Result.Type == Token_EndOfLine) ||
           (Result.Type == Token_Comment))
        {
            // NOTE(kstandbridge): Ignore these tokens
        }
        else
        {
            if(Result.Type == Token_String)
            {
                if(Result.Text.Size &&
                   (Result.Text.Data[0] == '"'))
                {
                    ++Result.Text.Data;
                    --Result.Text.Size;
                }
                
                if(Result.Text.Size &&
                   (Result.Text.Data[Result.Text.Size - 1] == '"'))
                {
                    --Result.Text.Size;
                }
            }
            
            
            break;
        }
    }
    
    return Result;
}

internal void
TokenError_(tokenizer *Tokenizer, token OnToken, char *Format, va_list ArgList)
{
    Win32ConsoleOut("%S(%d): Column(%d): ", OnToken.FileName, OnToken.LineNumber, OnToken.ColumnNumber);
    Win32ConsoleOut_(Format, ArgList);
    Win32ConsoleOut("\n");
    
    Tokenizer->HasErrors = true;
}

internal void
TokenError(tokenizer *Tokenizer, token OnToken, char *Format, ...)
{
    va_list ArgList;
    va_start(ArgList, Format);
    
    TokenError_(Tokenizer, OnToken, Format, ArgList);
    
    va_end(ArgList);
}

internal token
RequireToken(tokenizer *Tokenizer, token_type DesiredType)
{
    token Result = GetToken(Tokenizer);
    if(Result.Type != DesiredType)
    {
        TokenError(Tokenizer, Result, "Expected token type %S but got %S", 
                   GetTokenTypeName(DesiredType), GetTokenTypeName(Result.Type));
    }
    
    return Result;
}

internal token
RequireIdentifierToken(tokenizer *Tokenizer, string Match)
{
    token Result = RequireToken(Tokenizer, Token_Identifier);
    if(!StringsAreEqual(Result.Text, Match))
    {
        TokenError(Tokenizer, Result, "Expected identifier \"%S\" but got \"%S\"", Match, Result.Text);
    }
    
    return Result;
}

internal token
PeekToken(tokenizer *Tokenizer)
{
    tokenizer Clone = *Tokenizer;
    token Result = GetToken(&Clone);
    return Result;
}

internal b32
PeekTokenType(tokenizer *Tokenizer, token_type DesiredType)
{
    b32 Result = false;
    
    token Token = PeekToken(Tokenizer);
    if(Token.Type == DesiredType)
    {
        Result = true;
    }
    
    return Result;
}

internal b32
PeekIdentifierToken(tokenizer *Tokenizer, string Match)
{
    b32 Result = false;
    
    token Token = PeekToken(Tokenizer);
    if(Token.Type == Token_Identifier)
    {
        Result = StringsAreEqual(Token.Text, Match);
    }
    
    return Result;
}

inline b32
Parsing(tokenizer *Tokenizer)
{
    b32 Result = (!Tokenizer->HasErrors && 
                  !PeekTokenType(Tokenizer, Token_EndOfStream));
    
    return Result;
}

internal tokenizer
Tokenize(string FileData, string FileName)
{
    tokenizer Result =
    {
        .FileName = FileName,
        .ColumnNumber = 1,
        .LineNumber = 1,
        .FileData = FileData,
    };
    TokenizerAdvance(&Result, 0);
    
    return Result;
}

#define KENGINE_XML_TOKENIZER_H
#endif //KENGINE_XML_TOKENIZER_H

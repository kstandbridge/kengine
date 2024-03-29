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
        case '(': {Result.Type = Token_OpenParenthesis;} break;
        case ')': {Result.Type = Token_CloseParenthesis;} break;
        case '{': {Result.Type = Token_OpenCurlyBracket;} break;
        case '}': {Result.Type = Token_CloseCurlyBracket;} break;
        case '<': {Result.Type = Token_OpenAngleBracket;} break;
        case '>': {Result.Type = Token_CloseAngleBracket;} break;
        case '[': {Result.Type = Token_OpenBracket;} break;
        case ']': {Result.Type = Token_CloseBracket;} break;
        case '=': {Result.Type = Token_Equals;} break;
        case '/': {Result.Type = Token_ForwardSlash;} break;
        case '?': {Result.Type = Token_QuestionMark;} break;
        case '!': {Result.Type = Token_ExcalationMark;} break;
        case '-': {Result.Type = Token_Dash;} break;
        case '#': {Result.Type = Token_Hash;} break;
        case ',': {Result.Type = Token_Comma;} break;
        case '*': {Result.Type = Token_Asterisk;} break;
        case ';': {Result.Type = Token_SemiColon;} break;
        case ':': {Result.Type = Token_Colon;} break;
        case '+': {Result.Type = Token_Plus;} break;;
        case '&': {Result.Type = Token_Ampersand;} break;;
        case '~': {Result.Type = Token_Tilde;} break;;
        case '.': {Result.Type = Token_Period;} break;;
        case '|': {Result.Type = Token_Pipe;} break;;
        
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
            else if(IsAlpha(C) ||
                    (C == '_'))
            {
                Result.Type = Token_Identifier;
                
                while(IsAlpha(Tokenizer->At[0]) ||
                      IsNumber(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '_'))
                {
                    TokenizerAdvance(Tokenizer, 1);
                }
            }
            else if(IsNumber(C))
            {
                Result.Type = Token_Number;
                
                while(IsNumber(Tokenizer->At[0]) ||
                      IsAlpha(Tokenizer->At[0]) ||
                      (Tokenizer->At[0] == '.'))
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

token
GetToken(tokenizer *Tokenizer)
{
    token Result;
    
    for(;;)
    {
        Result = GetToken_(Tokenizer);
        if((Result.Type == Token_Spacing) ||
           (Result.Type == Token_EndOfLine))
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
    PlatformConsoleOut("%S(%d): Column(%d): ", OnToken.FileName, OnToken.LineNumber, OnToken.ColumnNumber);
    PlatformConsoleOut_(Format, ArgList);
    PlatformConsoleOut("\n", 0);
    
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

token
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

token
RequireIdentifierToken(tokenizer *Tokenizer, string Match)
{
    token Result = RequireToken(Tokenizer, Token_Identifier);
    if(!StringsAreEqual(Result.Text, Match))
    {
        TokenError(Tokenizer, Result, "Expected identifier \"%S\" but got \"%S\"", Match, Result.Text);
    }
    
    return Result;
}

token
PeekToken(tokenizer *Tokenizer)
{
    tokenizer Clone = *Tokenizer;
    token Result = GetToken(&Clone);
    return Result;
}

b32
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

b32
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

tokenizer
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

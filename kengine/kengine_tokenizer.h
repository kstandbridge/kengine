#ifndef KENGINE_TOKENIZER_H

typedef enum token_type
{
    Token_Unknown,
    
    Token_Spacing,
    Token_EndOfLine,
    
    Token_OpenParenthesis,
    Token_CloseParenthesis,
    Token_OpenCurlyBracket,
    Token_CloseCurlyBracket,
    Token_OpenAngleBracket,
    Token_CloseAngleBracket,
    Token_OpenBracket,
    Token_CloseBracket,
    Token_Equals,
    Token_ForwardSlash,
    Token_QuestionMark,
    Token_ExcalationMark,
    Token_Dash,
    Token_Hash,
    Token_Comma,
    Token_Asterisk,
    Token_Colon,
    Token_SemiColon,
    Token_Plus,
    Token_Ampersand,
    Token_Tilde,
    Token_Period,
    Token_Pipe,
    
    Token_Identifier,
    
    Token_String,
    Token_Number,
    
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
        
        case Token_OpenParenthesis:   {Result = String("open parenthesis");} break;
        case Token_CloseParenthesis:  {Result = String("close parenthesis");} break;
        case Token_OpenCurlyBracket:  {Result = String("open curly bracer");} break;
        case Token_CloseCurlyBracket: {Result = String("close curly bracer");} break;
        case Token_OpenAngleBracket:  {Result = String("open angle bracket");} break;
        case Token_CloseAngleBracket: {Result = String("close angle bracket");} break;
        case Token_Equals:            {Result = String("equals");} break;
        case Token_ForwardSlash:      {Result = String("forward slash");} break;
        case Token_QuestionMark:      {Result = String("question mark");} break;
        case Token_ExcalationMark:    {Result = String("excalation mark");} break;
        case Token_Dash:              {Result = String("dash");} break;
        case Token_Hash:              {Result = String("hash");} break;
        case Token_Comma:             {Result = String("comma");} break;
        case Token_Asterisk:          {Result = String("asterisk");} break;
        case Token_Colon:             {Result = String("colon");} break;
        case Token_SemiColon:         {Result = String("semi colon");} break;
        case Token_Plus:              {Result = String("plus");} break;
        case Token_Ampersand:         {Result = String("ampersand");} break;
        case Token_Tilde:             {Result = String("tilde");} break;
        case Token_Period:            {Result = String("period");} break;
        case Token_Pipe:              {Result = String("pipe");} break;
        
        case Token_Identifier:        {Result = String("identifier");} break;
        
        case Token_String:            {Result = String("string");} break;
        case Token_Number:            {Result = String("number");} break;
        
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

token
GetToken(tokenizer *Tokenizer);

token
RequireToken(tokenizer *Tokenizer, token_type DesiredType);

token
RequireIdentifierToken(tokenizer *Tokenizer, string Match);

b32
PeekTokenType(tokenizer *Tokenizer, token_type DesiredType);

b32
PeekIdentifierToken(tokenizer *Tokenizer, string Match);

token
PeekToken(tokenizer *Tokenizer);

inline b32
Parsing(tokenizer *Tokenizer)
{
    b32 Result = (!Tokenizer->HasErrors && 
                  !PeekTokenType(Tokenizer, Token_EndOfStream));
    
    return Result;
}

tokenizer
Tokenize(string FileData, string FileName);



#define KENGINE_TOKENIZER_H
#endif //KENGINE_TOKENIZER_H

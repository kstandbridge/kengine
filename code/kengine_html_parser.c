typedef enum http_token_type
{
    HttpToken_OpenTag,
    HttpToken_CloseTag,
    
    HttpToken_Equals,
    
    HttpToken_String,
    
    HttpToken_EndOfStream,
} http_token_type;

typedef struct http_token
{
    http_token_type Type;
    string Text;
} http_token;

typedef struct http_tokenizer
{
    memory_arena *Arena;
    char *At;
} http_tokenizer;

inline void
HttpTokenizerEatallWhitespace(http_tokenizer *Tokenizer)
{
    for(;;)
    {
        // TODO(kstandbridge): Eat comments?
        if(IsWhitespace(Tokenizer->At[0]))
        {
            ++Tokenizer->At;
        }
        else
        {
            break;
        }
    }
}

internal http_token
GetNextHttpToken(http_tokenizer *Tokenizer)
{
    HttpTokenizerEatallWhitespace(Tokenizer);
    
    http_token Result;
    Result.Text = String_(1, (u8 *)Tokenizer->At);
    char C = Tokenizer->At[0];
    ++Tokenizer->At;
    if(C == '>')
    {
        C = Tokenizer->At[0];
        ++Tokenizer->At;
    }
    switch(C)
    {
        case '\0': {Result.Type = HttpToken_EndOfStream; } break;
        case '=':  {Result.Type = HttpToken_Equals; } break;
        case '<': 
        {
            if(Tokenizer->At[0] == '/')
            {
                Result.Type = HttpToken_CloseTag; 
                ++Tokenizer->At;
            }
            else
            {
                Result.Type = HttpToken_OpenTag; 
            }
            
            Result.Text.Data = (u8 *)Tokenizer->At;
            
            while(!IsWhitespace(Tokenizer->At[0]) &&
                  (Tokenizer->At[0] != '>') &&
                  (Tokenizer->At[0] != '/'))
            {
                ++Tokenizer->At;
            }
            
            Result.Text.Size = Tokenizer->At - (char *)Result.Text.Data;
            
            if(Tokenizer->At[0] == '>')
            {
                ++Tokenizer->At;
            }
            else if((Tokenizer->At[0] == '/') &&
                    (Tokenizer->At[1] == '>'))
            {
                Tokenizer->At += 2;
            }
        } break;
        case '/':
        {
            if(Tokenizer->At[0] == '>')
            {
                ++Tokenizer->At;
                Result.Type = HttpToken_CloseTag; 
            }
            else
            {
                // NOTE(kstandbridge): Something didn't parse correctly, we are expecting a nameless end tag
                // like this <meta name="robots" />
                __debugbreak();
            }
        } break;
        
        default:
        {
            --Tokenizer->At;
            Result.Type = HttpToken_String; 
            if(Tokenizer->At[0] == '"')
            {
                
                ++Tokenizer->At;
            }
            
            Result.Text.Data = (u8 *)Tokenizer->At;
            
            while(Tokenizer->At[0] &&
                  Tokenizer->At[0] != '"' &&
                  Tokenizer->At[0] != '=' &&
                  Tokenizer->At[0] != '<')
            {
                ++Tokenizer->At;
            }
            
            Result.Text.Size = Tokenizer->At - (char *)Result.Text.Data;
            
            if(Tokenizer->At[0] == '"')
            {
                ++Tokenizer->At;
            }
        } break;
    }
    
    return Result;
}

inline b32
RequireHttpToken(http_tokenizer *Tokenizer, http_token_type DesiredType)
{
    http_token Token = GetNextHttpToken(Tokenizer);
    b32 Result = (Token.Type == DesiredType);
    return Result;
}

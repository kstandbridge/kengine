typedef enum html_token_type
{
    HtmlToken_Unknown,
    
    HtmlToken_OpenTag,
    HtmlToken_CloseTag,
    
    HtmlToken_Equals,
    
    HtmlToken_String,
    
    HtmlToken_EndOfStream,
} html_token_type;

typedef struct html_token
{
    html_token_type Type;
    string Value;
} html_token;

typedef struct html_tokenizer
{
    memory_arena *Arena;
    string Html;
    umm Index;
} html_tokenizer;

internal html_token
GetNextHtmlToken(html_tokenizer *Tokenizer)
{
    // NOTE(kstandbridge): Eat all whitespace
    for(;;)
    {
        if(Tokenizer->Index >= Tokenizer->Html.Size)
        {
            break;
        }
        u8 *At = Tokenizer->Html.Data + Tokenizer->Index;
        if(IsWhitespace(At[0]))
        {
            ++Tokenizer->Index;
        }
        else
        {
            break;
        }
    }
    
    u8 *At = Tokenizer->Html.Data + Tokenizer->Index;
    
    html_token Result;
    Result.Type = HtmlToken_Unknown;
    Result.Value.Size = 1;
    Result.Value.Data = At;
    
    if(Tokenizer->Index >= Tokenizer->Html.Size)
    {
        Result.Type = HtmlToken_EndOfStream;
    }
    else
    {
        char C = At[0];
        ++Tokenizer->Index;
        ++At;
        switch(C)
        {
            case '\0': {Result.Type = HtmlToken_EndOfStream; } break;
            case '=':  {Result.Type = HtmlToken_Equals; } break;
            case '<': 
            {
                if(At[0] == '/')
                {
                    Result.Type = HtmlToken_CloseTag; 
                    ++At;
                    ++Tokenizer->Index;
                }
                else
                {
                    Result.Type = HtmlToken_OpenTag; 
                }
                
                Result.Value.Data = At;
                
                while(!IsWhitespace(At[0]) &&
                      (At[0] != '>') &&
                      (At[0] != '/'))
                {
                    ++At;
                    ++Tokenizer->Index;
                }
                
                Result.Value.Size = At - Result.Value.Data;
                
                if(At[0] == '>')
                {
                    ++At;
                    ++Tokenizer->Index;
                }
                else if((At[0] == '/') &&
                        (At[1] == '>'))
                {
                    At += 2;
                    Tokenizer->Index += 2;
                }
            } break;
            case '/':
            {
                if(At[0] == '>')
                {
                    ++At;
                    ++Tokenizer->Index;
                    Result.Type = HtmlToken_CloseTag; 
                }
                else
                {
                    // NOTE(kstandbridge): Something didn't parse correctly, we are expecting a nameless end tag
                    // like this <meta name="robots" />
                }
            } break;
            
            default:
            {
                --At;
                --Tokenizer->Index;
                Result.Type = HtmlToken_String; 
                if(At[0] == '"')
                {
                    
                    ++At;
                    ++Tokenizer->Index;
                }
                
                Result.Value.Data = (u8 *)At;
                
                while(At[0] &&
                      At[0] != '"' &&
                      At[0] != '=' &&
                      At[0] != '<')
                {
                    ++At;
                    ++Tokenizer->Index;
                }
                
                Result.Value.Size = At - Result.Value.Data;
                
                if(At[0] == '"')
                {
                    ++At;
                    ++Tokenizer->Index;
                }
            } break;
        }
    }
    
    return Result;
}

inline b32
RequireHtmlToken(html_tokenizer *Tokenizer, html_token_type DesiredType)
{
    html_token Token = GetNextHtmlToken(Tokenizer);
    b32 Result = (Token.Type == DesiredType);
    return Result;
}

typedef void *parse_html_callback(memory_arena *Arena, html_tokenizer *Tokenizer);

internal void *
ParseHtml(memory_arena *Arena, string Html, parse_html_callback *Callback)
{
    html_tokenizer Tokenizer;
    Tokenizer.Arena = Arena;
    Tokenizer.Html = Html;
    Tokenizer.Index = 0;
    
    // NOTE(kstandbridge): Skip over DOCTYPE
    if(StringBeginsWith(String("<!DOCTYPE"), Tokenizer.Html))
    {       
        ++Tokenizer.Index;
        while(Tokenizer.Html.Data[Tokenizer.Index] &&
              Tokenizer.Html.Data[Tokenizer.Index] != '<')
        {
            ++Tokenizer.Index;
        }
    }
    
    
    void *Result = Callback(Arena, &Tokenizer);
    return Result;
}

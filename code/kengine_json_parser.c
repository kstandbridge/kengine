typedef enum json_token_type
{
    JsonToken_Unknown,
    
    JsonToken_OpenBracer,
    JsonToken_CloseBracer,
    
    JsonToken_OpenBracket,
    JsonToken_CloseBracket,
    
    JsonToken_Colon,
    JsonToken_Comma,
    
    JsonToken_String,
    
    JsonToken_EndOfStream
} json_token_type;

typedef struct json_token
{
    json_token_type Type;
    string Value;
} json_token;

typedef struct json_tokenizer
{
    memory_arena *Arena;
    string Json;
    umm Index;
} json_tokenizer;

internal json_token
GetNextJsonToken(json_tokenizer *Tokenizer)
{
    // NOTE(kstandbridge): Eat all whitespace
    for(;;)
    {
        u8 *At = Tokenizer->Json.Data + Tokenizer->Index;
        if(IsWhitespace(At[0]))
        {
            ++Tokenizer->Index;
        }
        else
        {
            break;
        }
    }
    
    u8 *At = Tokenizer->Json.Data + Tokenizer->Index;
    ++Tokenizer->Index;
    
    json_token Result;
    Result.Type = JsonToken_Unknown;
    Result.Value.Size = 1;
    Result.Value.Data = At;
    
    if(Tokenizer->Index > Tokenizer->Json.Size)
    {
        Result.Type = JsonToken_EndOfStream;
    }
    else
    {    
        char C = At[0];
        switch(C)
        {
            case '{': { Result.Type = JsonToken_OpenBracer; } break;
            case '}': { Result.Type = JsonToken_CloseBracer; } break;
            case '[': { Result.Type = JsonToken_OpenBracket; } break;
            case ']': { Result.Type = JsonToken_CloseBracket; } break;
            case ':': { Result.Type = JsonToken_Colon; } break;
            case ',': { Result.Type = JsonToken_Comma; } break;
            
            
            case '"': 
            { 
                Result.Type = JsonToken_String; 
                ++Result.Value.Data;
                do
                {
                    At = Tokenizer->Json.Data + ++Tokenizer->Index;
                    ++Result.Value.Size;
                } while((Tokenizer->Index < Tokenizer->Json.Size) &&
                        (At[0] != '"'));
                ++Tokenizer->Index;
                --Result.Value.Size;
                
            } break;
            
            InvalidDefaultCase;
        }
    }
    
    return Result;
}

inline b32
RequireJsonToken(json_tokenizer *Tokenizer, json_token_type DesiredType)
{
    json_token Token = GetNextJsonToken(Tokenizer);
    b32 Result = (Token.Type == DesiredType);
    return Result;
}

typedef void *parse_json_callback(memory_arena *Arena, json_tokenizer *Tokenizer);

internal void *
ParseJson(memory_arena *Arena, string Json, parse_json_callback *Callback)
{
    json_tokenizer Tokenizer;
    Tokenizer.Arena = Arena;
    Tokenizer.Json = Json;
    Tokenizer.Index = 0;
    
    void *Result = Callback(Arena, &Tokenizer);
    return Result;
}

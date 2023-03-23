#ifndef KENGINE_JSON_PARSER_H

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

json_token
GetNextJsonToken(json_tokenizer *Tokenizer);

b32
RequireJsonToken(json_tokenizer *Tokenizer, json_token_type DesiredType);

typedef void *parse_json_callback(memory_arena *Arena, json_tokenizer *Tokenizer);

void *
ParseJson(memory_arena *Arena, string Json, parse_json_callback *Callback);

#define KENGINE_JSON_PARSER_H
#endif //KENGINE_JSON_PARSER_H

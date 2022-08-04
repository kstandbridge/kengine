#ifndef WIN32_KENGINE_PREPROCESSOR_H

#include "kengine_platform.h"
#include "win32_kengine_types.h"
#include "kengine_string.h"

typedef enum c_token_type
{
    CTokenType_Unknown,
    
    CTokenType_OpenParen,    
    CTokenType_CloseParen,    
    CTokenType_Colon,    
    CTokenType_Semicolon,    
    CTokenType_Asterisk,    
    CTokenType_OpenBracket,    
    CTokenType_CloseBracket,    
    CTokenType_OpenBrace,    
    CTokenType_CloseBrace,
    
    CTokenType_String,
    CTokenType_Identifier,
    
    CTokenType_EndOfStream
} c_token_type;

typedef struct c_token
{
    c_token_type Type;
    
    string Str;
    
} c_token;

typedef struct c_tokenizer
{
    memory_arena *Arena;
    char *At;
} c_tokenizer;


#define WIN32_KENGINE_PREPROCESSOR_H
#endif //WIN32_KENGINE_PREPROCESSOR_H

#ifndef WIN32_KENGINE_PREPROCESSOR_H

#include "kengine_platform.h"
#include "win32_kengine_types.h"
#include "kengine_string.h"

typedef enum
{
    CToken_Unknown,
    
    CToken_OpenParen,    
    CToken_CloseParen,    
    CToken_Colon,    
    CToken_Semicolon,    
    CToken_Asterisk,    
    CToken_OpenBracket,    
    CToken_CloseBracket,    
    CToken_OpenBrace,    
    CToken_CloseBrace,
    
    CToken_String,
    CToken_Identifier,
    
    CToken_EndOfStream
} c_token_type;

typedef struct
{
    c_token_type Type;
    
    string Str;
    
} c_token;

typedef struct
{
    memory_arena *Arena;
    char *At;
} c_tokenizer;


#define WIN32_KENGINE_PREPROCESSOR_H
#endif //WIN32_KENGINE_PREPROCESSOR_H

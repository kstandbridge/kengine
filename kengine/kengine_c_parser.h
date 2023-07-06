#ifndef KENGINE_C_PARSER_H

typedef enum c_type
{
    C_B32,
    C_F32,
    C_U32,
    C_String,
    C_Custom,
} c_type;

internal string
GetCTypeName(c_type Type)
{
    string Result = {0};
    
    switch(Type)
    {
        case C_B32:    { Result = String("boolean"); } break;
        case C_F32:    { Result = String("32-bit float"); } break;
        case C_U32:    { Result = String("unsigned 32-bit integer"); } break;
        case C_String: { Result = String("string"); } break;
        //NOTE(kstandbridge): ignored
        case C_Custom: {  } break;
    }
    
    return Result;
}

typedef struct c_member
{
    b32 IsPointer;
    string Name;
    string TypeName;
    c_type Type;
    
    struct c_member *Next;
} c_member;

typedef struct c_struct
{
    string Type;
    string Name;
    
    c_member *Members;
} c_struct;

internal c_struct
ParseStruct(memory_arena *Arena, tokenizer *Tokenizer, string Name);

#define KENGINE_C_PARSER_H
#endif //KENGINE_C_PARSER_H

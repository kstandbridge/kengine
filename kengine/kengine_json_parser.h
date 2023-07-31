#ifndef KENGINE_JSON_PARSER_H

typedef enum json_element_type
{
    JsonElement_String,
    JsonElement_Number,
    JsonElement_Object,
    JsonElement_Array,
} json_element_type;

typedef struct json_element
{
    string Name;
    string Value;

    json_element_type Type;
    
    struct json_element *Children;
    struct json_element *Next;
} json_element;

internal json_element *
GetJsonElement(json_element *Parent, string Name)
{
    json_element *Result = 0;
    for(Result = Parent->Children;
        Result;
        Result = Result->Next)
    {
        if(StringsAreEqual(Result->Name, Name))
        {
            break;
        }
    }
    return Result;
}

internal void
ParseJsonElement(memory_arena *Arena, json_element *Element, tokenizer *Tokenizer)
{
    json_element *CurrentChild = 0;
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_String)
        {
            RequireToken(Tokenizer, Token_Colon);
                        
            if(CurrentChild == 0)
            {
                CurrentChild = Element->Children = PushStruct(Arena, json_element);
            }
            else
            {
                CurrentChild->Next = PushStruct(Arena, json_element);
                CurrentChild = CurrentChild->Next;
            }
            CurrentChild->Name = Token.Text;

            token ValueToken = GetToken(Tokenizer);
            if(ValueToken.Type == Token_OpenCurlyBracket)
            {
                CurrentChild->Type = JsonElement_Object;               
                ParseJsonElement(Arena, CurrentChild, Tokenizer);
            }
            else if(ValueToken.Type == Token_OpenBracket)
            {
                CurrentChild->Type = JsonElement_Array;

                json_element *ArrayChild = 0;
              
                while(Parsing(Tokenizer))
                {
                    Token = GetToken(Tokenizer);

                    if(Token.Type == Token_OpenCurlyBracket)
                    {
                        if(ArrayChild == 0)
                        {
                            ArrayChild = CurrentChild->Children = PushStruct(Arena, json_element);
                        }
                        else
                        {
                            ArrayChild->Next = PushStruct(Arena, json_element);
                            ArrayChild = ArrayChild->Next;
                        }
                        ArrayChild->Type = JsonElement_Object;
                        ParseJsonElement(Arena, ArrayChild, Tokenizer);
                    }
                    else if(ValueToken.Type == Token_CloseBracket)
                    {
                        break;
                    }
                }

            }
            else if(ValueToken.Type == Token_String)
            {
                CurrentChild->Type = JsonElement_String;
            }
            else if(ValueToken.Type == Token_Number)
            {
                CurrentChild->Type = JsonElement_Number;
            }
            
            CurrentChild->Value = ValueToken.Text;
        }
        else if(Token.Type == Token_CloseCurlyBracket)
        {
            break;
        }
    }
}

internal json_element *
ParseJsonDocument(memory_arena *Arena, string FileData, string FileName)
{
    json_element *Result = PushStruct(Arena, json_element);
    Result->Name = String("root");
    Result->Type = JsonElement_Object;

    tokenizer Tokenizer = Tokenize(FileData, FileName);
    RequireToken(&Tokenizer, Token_OpenCurlyBracket);
    ParseJsonElement(Arena, Result, &Tokenizer);

    return Result;
}

#define KENGINE_JSON_PARSER_H
#endif //KENGINE_JSON_PARSER_H

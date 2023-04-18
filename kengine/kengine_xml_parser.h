#ifndef KENGINE_XML_PARSER_H

typedef struct xml_attribute
{
    string Key;
    string Value;
    
    struct xml_attribute *Next;
} xml_attribute;

typedef struct xml_element
{
    string Name;
    string Value;
    
    xml_attribute *Attributes;
    
    struct xml_element *Children;
    struct xml_element *Next;
} xml_element;

internal string
GetXmlElementValue(xml_element *Parent)
{
    string Result = Parent->Value;
    return Result;
}

internal xml_attribute *
GetXmlAttribute(xml_element *Element, string Key)
{
    xml_attribute *Result;
    for(Result = Element->Attributes;
        Result;
        Result = Result->Next)
    {
        if(StringsAreEqual(Result->Key, Key))
        {
            break;
        }
    }
    
    return Result;
}

internal xml_element *
GetXmlElement(xml_element *Parent, string Name)
{
    xml_element *Result;
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

internal xml_element *
GetNextXmlElement(xml_element *Parent, string Name)
{
    xml_element *Result;
    for(Result = Parent->Next;
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
ParseXmlElement(memory_arena *Arena, xml_element *Element, tokenizer *Tokenizer)
{
    xml_element *CurrentChild = 0;
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_OpenAngleBracket)
        {
            if(PeekTokenType(Tokenizer, Token_ForwardSlash))
            {
                RequireToken(Tokenizer, Token_ForwardSlash);
                token EndToken = RequireToken(Tokenizer, Token_Identifier);
                if(StringsAreEqual(EndToken.Text, Element->Name))
                {
                    break;
                }
                RequireToken(Tokenizer, Token_CloseAngleBracket);
            }
            else if(PeekTokenType(Tokenizer, Token_QuestionMark))
            {
                Token = GetToken(Tokenizer);
                Token = GetToken(Tokenizer);
                while(Parsing(Tokenizer) &&
                      (Token.Type != Token_QuestionMark))
                {
                    Token = GetToken(Tokenizer);
                }
                RequireToken(Tokenizer, Token_CloseAngleBracket);
            }
            else if(PeekTokenType(Tokenizer, Token_ExcalationMark))
            {
                Token = GetToken(Tokenizer);
                Token = GetToken(Tokenizer);
                if((Token.Type == Token_Identifier) &&
                   (StringsAreEqual(String("DOCTYPE"), Token.Text)))
                {
                    while(Parsing(Tokenizer))
                    {
                        if(Token.Type == Token_CloseAngleBracket)
                        {
                            break;
                        }
                        else
                        {
                            Token = GetToken(Tokenizer);
                        }
                    }
                }
                else
                {
                    RequireToken(Tokenizer, Token_Dash);
                    Token = GetToken(Tokenizer);
                    while(Parsing(Tokenizer))
                    {
                        if((Token.Type == Token_Dash) &&
                           PeekTokenType(Tokenizer, Token_Dash))
                        {
                            break;
                        }
                        else
                        {
                            Token = GetToken(Tokenizer);
                        }
                    }
                    RequireToken(Tokenizer, Token_Dash);
                    RequireToken(Tokenizer, Token_CloseAngleBracket);
                }
            }
            else
            {            
                Token = RequireToken(Tokenizer, Token_Identifier);
                
                if(CurrentChild == 0)
                {
                    CurrentChild = Element->Children = PushStruct(Arena, xml_element);
                }
                else
                {
                    CurrentChild->Next = PushStruct(Arena, xml_element);
                    CurrentChild = CurrentChild->Next;
                }
                CurrentChild->Name = Token.Text;
                
                
                // NOTE(kstandbridge): Parse xml attributes
                {
                    xml_attribute *CurrentAttribute = 0;
                    
                    while(Parsing(Tokenizer) &&
                          !PeekTokenType(Tokenizer, Token_ForwardSlash) &&
                          !PeekTokenType(Tokenizer, Token_CloseAngleBracket))
                    {
                        token Key = RequireToken(Tokenizer, Token_Identifier);
                        RequireToken(Tokenizer, Token_Equals);
                        token Value = RequireToken(Tokenizer, Token_String);
                        
                        if(CurrentAttribute == 0)
                        {
                            CurrentAttribute = CurrentChild->Attributes = PushStruct(Arena, xml_attribute);
                        }
                        else
                        {
                            CurrentAttribute->Next = PushStruct(Arena, xml_attribute);
                            CurrentAttribute = CurrentAttribute->Next;
                        }
                        
                        CurrentAttribute->Key = Key.Text;
                        CurrentAttribute->Value = Value.Text;
                        
                    }
                }
                
                token Closing = GetToken(Tokenizer);
                if(Closing.Type == Token_ForwardSlash)
                {
                    Closing = GetToken(Tokenizer);
                }
                else
                {
                    if(PeekTokenType(Tokenizer, Token_OpenAngleBracket))
                    {
                        ParseXmlElement(Arena, CurrentChild, Tokenizer);
                    }
                    else
                    {
                        u8 *Start = Tokenizer->FileData.Data;
                        
                        while(Parsing(Tokenizer))
                        {
                            Token = GetToken(Tokenizer);
                            if(Token.Type == Token_ForwardSlash)
                            {
                                if(PeekIdentifierToken(Tokenizer, CurrentChild->Name))
                                {
                                    CurrentChild->Value.Data = Start;
                                    CurrentChild->Value.Size = Tokenizer->FileData.Data - Start - 2;
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

internal xml_element *
ParseXmlDocument(memory_arena *Arena, string FileData, string FileName)
{
    xml_element *Result = PushStruct(Arena, xml_element);
    Result->Name = String("<root>");
    
    tokenizer Tokenizer = Tokenize(FileData, FileName);
    
    ParseXmlElement(Arena, Result, &Tokenizer);
    
    return Result;
}

#define KENGINE_XML_PARSER_H
#endif //KENGINE_XML_PARSER_H

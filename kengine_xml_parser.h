#ifndef KENGINE_XML_PARSER_H

typedef struct xml_attribute
{
    string Key;
    string Value;
} xml_attribute;

typedef struct xml_element
{
    memory_arena *Arena;
    
    string Name;
    
    tokenizer Tokenizer;
    
    struct xml_element *Next;
} xml_element;

internal xml_attribute *
GetXmlAttribute(xml_element *Parent, string Name)
{
    xml_attribute *Result = 0;
    
    tokenizer Tokenizer_ = Parent->Tokenizer;
    tokenizer *Tokenizer = &Tokenizer_;
    
    while(Parsing(Tokenizer) &&
          !PeekTokenType(Tokenizer, Token_ForwardSlash) &&
          !PeekTokenType(Tokenizer, Token_CloseAngleBracket))
    {
        token Key = RequireToken(Tokenizer, Token_Identifier);
        RequireToken(Tokenizer, Token_Equals);
        token Value = RequireToken(Tokenizer, Token_String);
        
        if(StringsAreEqual(Key.Text, Name))
        {
            Result = PushStruct(Parent->Arena, xml_attribute);
            Result->Key = Key.Text;
            Result->Value = Value.Text;
        }
        
    }
    
    return Result;
}

internal void
SkipXmlAttributes_(tokenizer *Tokenizer)
{
    while(Parsing(Tokenizer) &&
          !PeekTokenType(Tokenizer, Token_ForwardSlash) &&
          !PeekTokenType(Tokenizer, Token_CloseAngleBracket))
    {
        RequireToken(Tokenizer, Token_Identifier);
        RequireToken(Tokenizer, Token_Equals);
        RequireToken(Tokenizer, Token_String);
    }
}

internal string
GetXmlElementValue(xml_element *Parent)
{
    string Result = String("");
    
    tokenizer Tokenizer_ = Parent->Tokenizer;
    tokenizer *Tokenizer = &Tokenizer_;
    
    SkipXmlAttributes_(Tokenizer);
    
    token Closing = GetToken(Tokenizer);
    if(Closing.Type == Token_ForwardSlash)
    {
        Closing = GetToken(Tokenizer);
    }
    else
    {
        u8 *Start = Tokenizer->FileData.Data;
        
        while(Parsing(Tokenizer))
        {
            token Token = GetToken(Tokenizer);
            if(Token.Type == Token_ForwardSlash)
            {
                if(PeekIdentifierToken(Tokenizer, Parent->Name))
                {
                    Result.Data = Start;
                    Result.Size = Tokenizer->FileData.Data - Start - 2;
                    break;
                }
            }
        }
    }
    
    return Result;
}

internal xml_element *
GetXmlElements_(xml_element *Parent, string Name, b32 FirstOnly)
{
    xml_element *Result = 0;
    
    xml_element *CurrentElement = 0;
    
    tokenizer Clone = Parent->Tokenizer;
    tokenizer *Tokenizer = &Clone;
    
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_OpenAngleBracket)
        {
            if(PeekTokenType(Tokenizer, Token_ForwardSlash))
            {
                RequireToken(Tokenizer, Token_ForwardSlash);
                token EndToken = RequireToken(Tokenizer, Token_Identifier);
                if(StringsAreEqual(EndToken.Text, Parent->Name))
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
                b32 IsMatch = StringsAreEqual(Token.Text, Name);
                
                if(IsMatch)
                {
                    if(CurrentElement == 0)
                    {
                        CurrentElement = Result = PushStruct(Parent->Arena, xml_element);
                    }
                    else
                    {
                        CurrentElement->Next = PushStruct(CurrentElement->Arena, xml_element);
                        CurrentElement = CurrentElement->Next;
                    }
                    CurrentElement->Arena = Parent->Arena;
                    CurrentElement->Name = Name;
                    CurrentElement->Tokenizer = *Tokenizer;
                }
                
                SkipXmlAttributes_(Tokenizer);
                
                Token = GetToken(Tokenizer);
                if(Token.Type == Token_ForwardSlash)
                {
                    Token = GetToken(Tokenizer);
                }
                
                if(IsMatch && FirstOnly)
                {
                    break;
                }
            }
        }
    }
    
    return Result;
}

internal xml_element *
GetXmlElements(xml_element *Parent, string Name)
{
    xml_element *Result = GetXmlElements_(Parent, Name, false);
    
    return Result;
}

internal xml_element *
GetXmlElement(xml_element *Parent, string Name)
{
    xml_element *Result = GetXmlElements_(Parent, Name, true);
    
    return Result;
}

internal xml_element
ParseXmlDocument(memory_arena *Arena, string FileData, string FileName)
{
    xml_element Result =
    {
        .Arena = Arena,
        .Name = String("<root>"),
        .Tokenizer = Tokenize(FileData, FileName),
    };
    
    return Result;
}

#define KENGINE_XML_PARSER_H
#endif //KENGINE_XML_PARSER_H

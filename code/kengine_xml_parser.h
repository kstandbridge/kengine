#ifndef KENGINE_XML_PARSER_H
#define KENGINE_XML_PARSER_H

typedef struct xml_attribute
{
    string Key;
    string Value;
    
    struct xml_attribute *Next;
} xml_attribute;

typedef struct xml_element
{
    memory_arena *Arena;
    
    string Name;
    
    xml_attribute *CurrentAttribute;
    xml_attribute *Attributes;
    
    tokenizer Tokenizer;
    
    struct xml_element *Next;
} xml_element;

internal xml_attribute *
GetXmlAttribute(xml_element *Parent, string Key)
{
    xml_attribute *Result = 0;
    
    for(xml_attribute *Attribute = Parent->Attributes;
        Attribute;
        Attribute = Attribute->Next)
    {
        if(StringsAreEqual(Attribute->Key, Key))
        {
            Result = Attribute;
            break;
        }
    }
    
    return Result;
}

internal string
GetXmlElementValue(xml_element Parent)
{
    string Result = String("");
    
    tokenizer Tokenizer_ = Parent.Tokenizer;
    tokenizer *Tokenizer = &Tokenizer_;
    
    
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_ForwardSlash)
        {
            if(PeekIdentifierToken(Tokenizer, Parent.Name))
            {
                Result.Data = Parent.Tokenizer.FileData.Data;
                Result.Size = Tokenizer->FileData.Data - Parent.Tokenizer.FileData.Data - 2;
                break;
            }
        }
    }
    
    return Result;
}

internal xml_element
GetXmlElement(xml_element Parent, string Name)
{
    xml_element Result =
    {
        .Arena = Parent.Arena,
        .Name = Name,
        .Tokenizer = Parent.Tokenizer
    };
    
    tokenizer *Tokenizer = &Result.Tokenizer;
    
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_OpenAngleBracket)
            
        {        
            if(PeekTokenType(Tokenizer, Token_ForwardSlash))
            {
                RequireToken(Tokenizer, Token_ForwardSlash);
                RequireToken(Tokenizer, Token_Identifier);
                RequireToken(Tokenizer, Token_CloseAngleBracket);
            }
            else if(PeekTokenType(Tokenizer, Token_QuestionMark))
            {
                token HeaderToken = GetToken(Tokenizer);
                HeaderToken = GetToken(Tokenizer);
                while(Parsing(Tokenizer) &&
                      (HeaderToken.Type != Token_QuestionMark))
                {
                    HeaderToken = GetToken(Tokenizer);
                }
                RequireToken(Tokenizer, Token_CloseAngleBracket);
            }
            else if(PeekTokenType(Tokenizer, Token_ExcalationMark))
            {
                token CommentToken = GetToken(Tokenizer);
                RequireToken(Tokenizer, Token_Dash);
                RequireToken(Tokenizer, Token_Dash);
                CommentToken = GetToken(Tokenizer);
                while(Parsing(Tokenizer))
                {
                    if((CommentToken.Type == Token_Dash) &&
                       PeekTokenType(Tokenizer, Token_Dash))
                    {
                        break;
                    }
                    else
                    {
                        CommentToken = GetToken(Tokenizer);
                    }
                }
                RequireToken(Tokenizer, Token_Dash);
                RequireToken(Tokenizer, Token_CloseAngleBracket);
            }
            else
            {            
                token Identifier = RequireToken(Tokenizer, Token_Identifier);
                b32 IsMatch = StringsAreEqual(Identifier.Text, Name);
                
                // NOTE(kstandbridge): Parse attributes
                {
                    while(Parsing(Tokenizer) &&
                          !PeekTokenType(Tokenizer, Token_ForwardSlash) &&
                          !PeekTokenType(Tokenizer, Token_CloseAngleBracket))
                    {
                        token Key = RequireToken(Tokenizer, Token_Identifier);
                        RequireToken(Tokenizer, Token_Equals);
                        token Value = RequireToken(Tokenizer, Token_String);
                        
                        if(IsMatch)
                        {
                            xml_attribute *Attribute = Result.CurrentAttribute;
                            if(Attribute == 0)
                            {
                                Attribute = Result.CurrentAttribute = Result.Attributes = PushStruct(Result.Arena, xml_attribute);
                            }
                            else
                            {
                                Result.CurrentAttribute->Next = PushStruct(Result.Arena, xml_attribute);
                                Attribute = Result.CurrentAttribute = Result.CurrentAttribute->Next;
                                
                            }
                            Attribute->Key = Key.Text;
                            Attribute->Value = Value.Text;
                        }
                        
                    }
                    token Closing = GetToken(Tokenizer);
                    if(Closing.Type == Token_ForwardSlash)
                    {
                        Closing = GetToken(Tokenizer);
                    }
                }
                
                if(IsMatch)
                {
                    break;
                }
            }
        }
    }
    
    return Result;
}

internal xml_element *
GetXmlElements(xml_element Parent, string Name)
{
    xml_element *Result = 0;
    
    xml_element *CurrentElement = 0;
    
    tokenizer Clone = Parent.Tokenizer;
    tokenizer *Tokenizer = &Clone;
    
    while(Parsing(Tokenizer))
    {
        RequireToken(Tokenizer, Token_OpenAngleBracket);
        
        if(PeekTokenType(Tokenizer, Token_ForwardSlash))
        {
            RequireToken(Tokenizer, Token_ForwardSlash);
            token EndToken = RequireToken(Tokenizer, Token_Identifier);
            if(StringsAreEqual(EndToken.Text, Parent.Name))
            {
                break;
            }
            RequireToken(Tokenizer, Token_CloseAngleBracket);
        }
        else if(PeekTokenType(Tokenizer, Token_QuestionMark))
        {
            token HeaderToken = GetToken(Tokenizer);
            HeaderToken = GetToken(Tokenizer);
            while(Parsing(Tokenizer) &&
                  (HeaderToken.Type != Token_QuestionMark))
            {
                HeaderToken = GetToken(Tokenizer);
            }
            RequireToken(Tokenizer, Token_CloseAngleBracket);
        }
        else
        {            
            token Identifier = RequireToken(Tokenizer, Token_Identifier);
            b32 IsMatch = StringsAreEqual(Identifier.Text, Name);
            
            if(IsMatch)
            {
                if(CurrentElement == 0)
                {
                    CurrentElement = Result = PushStruct(Parent.Arena, xml_element);
                }
                else
                {
                    CurrentElement->Next = PushStruct(CurrentElement->Arena, xml_element);
                    CurrentElement = CurrentElement->Next;
                }
                CurrentElement->Arena = Parent.Arena;
                CurrentElement->Name = Name;
                CurrentElement->Tokenizer = Parent.Tokenizer;
            }
            else
            {
                LogDebug("Skipping element %S", Identifier.Text);
            }
            
            // NOTE(kstandbridge): Parse attributes
            {
                while(Parsing(Tokenizer) &&
                      !PeekTokenType(Tokenizer, Token_ForwardSlash) &&
                      !PeekTokenType(Tokenizer, Token_CloseAngleBracket))
                {
                    token Key = RequireToken(Tokenizer, Token_Identifier);
                    RequireToken(Tokenizer, Token_Equals);
                    token Value = RequireToken(Tokenizer, Token_String);
                    
                    if(IsMatch)
                    {
                        xml_attribute *Attribute = CurrentElement->CurrentAttribute;
                        if(Attribute == 0)
                        {
                            Attribute = CurrentElement->CurrentAttribute = CurrentElement->Attributes = PushStruct(CurrentElement->Arena, xml_attribute);
                        }
                        else
                        {
                            CurrentElement->CurrentAttribute->Next = PushStruct(CurrentElement->Arena, xml_attribute);
                            Attribute = CurrentElement->CurrentAttribute = CurrentElement->CurrentAttribute->Next;
                            
                        }
                        Attribute->Key = Key.Text;
                        Attribute->Value = Value.Text;
                    }
                    else
                    {
                        LogDebug("Skipping attribute %S=\"%S\"", Key.Text, Value.Text);
                    }
                    
                }
                token Closing = GetToken(Tokenizer);
                if(Closing.Type == Token_ForwardSlash)
                {
                    Closing = GetToken(Tokenizer);
                }
            }
        }
    }
    
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

#endif //KENGINE_XML_PARSER_H

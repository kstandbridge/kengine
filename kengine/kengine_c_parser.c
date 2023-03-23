
c_struct
ParseStruct(memory_arena *Arena, tokenizer *Tokenizer, string Name)
{
    c_struct Result = 
    {
        .Type = PushString_(Arena, Name.Size, Name.Data),
        .Name = PushString_(Arena, Name.Size, Name.Data),
        .Members = 0
    };
    c_member *CurrentMember = 0;
    ToUpperCamelCase(&Result.Name);
    
    RequireToken(Tokenizer, Token_OpenCurlyBracket);
    while(Parsing(Tokenizer))
    {
        token Token = GetToken(Tokenizer);
        if(Token.Type == Token_CloseCurlyBracket)
        {
            break;
        }
        else if(Token.Type == Token_SemiColon)
        {
        }
        else if(Token.Type == Token_ForwardSlash)
        {
            if(Tokenizer->At[0] == '/')
            {
                while((Tokenizer->At[0]) &&
                      (!IsEndOfLine(Tokenizer->At[0])))
                {
                    TokenizerAdvance(Tokenizer, 1);
                }
            }
        }
        else if(Token.Type == Token_Identifier)
        {
            if(CurrentMember == 0)
            {
                Result.Members = CurrentMember = PushStruct(Arena, c_member);
            }
            else
            {
                CurrentMember->Next = PushStruct(Arena, c_member);
                CurrentMember = CurrentMember->Next;
            }
            CurrentMember->TypeName = Token.Text;
            if(StringsAreEqual(String("b32"), Token.Text))
            {
                CurrentMember->Type = C_B32;
            }
            else if(StringsAreEqual(String("f32"), Token.Text))
            {
                CurrentMember->Type = C_F32;
            }
            else if(StringsAreEqual(String("u32"), Token.Text))
            {
                CurrentMember->Type = C_U32;
            }
            else if(StringsAreEqual(String("string"), Token.Text))
            {
                CurrentMember->Type = C_String;
            }
            else
            {
                if(StringsAreEqual(String("struct"), Token.Text))
                {
                    Token = RequireToken(Tokenizer, Token_Identifier);
                }
                CurrentMember->Type = C_Custom;
            }
            Token = GetToken(Tokenizer);
            if(Token.Type == Token_Asterisk)
            {
                CurrentMember->IsPointer = true;
                Token = GetToken(Tokenizer);
            }
            
            if(Token.Type == Token_Identifier)
            {
                CurrentMember->Name = Token.Text;
            }
            else
            {
                TokenError(Tokenizer, Token, "Expected member name");
            }
            
        }
        else
        {
            TokenError(Tokenizer, Token, "Expecting struct members but got %S \"%S\"", GetTokenTypeName(Token.Type), Token.Text);
        }
    }
    
    return Result;
}

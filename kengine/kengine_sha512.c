/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtom.org
 */

// NOTE(kstandbridge): The follow has been heavily modified, thus does
// not represent code written by the origional author metioned above

typedef struct sha512_state
{
    u64 CurrentLength;
    u64 Length;
    u64 State[8];
    u8 Buffer[128];
    
} sha512_state; 

global u64 GlobalKArray[80] = {
    0x428a2f98d728ae22, 0x7137449123ef65cd, 
    0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
    0x3956c25bf348b538, 0x59f111f1b605d019, 
    0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
    0xd807aa98a3030242, 0x12835b0145706fbe, 
    0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
    0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 
    0x9bdc06a725c71235, 0xc19bf174cf692694,
    0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 
    0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
    0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 
    0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
    0x983e5152ee66dfab, 0xa831c66d2db43210, 
    0xb00327c898fb213f, 0xbf597fc7beef0ee4,
    0xc6e00bf33da88fc2, 0xd5a79147930aa725, 
    0x06ca6351e003826f, 0x142929670a0e6e70,
    0x27b70a8546d22ffc, 0x2e1b21385c26c926, 
    0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
    0x650a73548baf63de, 0x766a0abb3c77b2a8, 
    0x81c2c92e47edaee6, 0x92722c851482353b,
    0xa2bfe8a14cf10364, 0xa81a664bbc423001,
    0xc24b8b70d0f89791, 0xc76c51a30654be30,
    0xd192e819d6ef5218, 0xd69906245565a910, 
    0xf40e35855771202a, 0x106aa07032bbd1b8,
    0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 
    0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
    0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 
    0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
    0x748f82ee5defb2fc, 0x78a5636f43172f60, 
    0x84c87814a1f0ab72, 0x8cc702081a6439ec,
    0x90befffa23631e28, 0xa4506cebde82bde9, 
    0xbef9a3f7b2c67915, 0xc67178f2e372532b,
    0xca273eceea26619c, 0xd186b8c721c0c207, 
    0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
    0x06f067aa72176fba, 0x0a637dc5a2c898a6, 
    0x113f9804bef90dae, 0x1b710b35131c471b,
    0x28db77f523047d84, 0x32caab7b40c72493, 
    0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
    0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 
    0x5fcb6fab3ad6faec, 0x6c44198c4a475817
};

#define ROR64c(x, y) \
( ((((x)&(0xFFFFFFFFFFFFFFFF))>>((u64)(y)&(63))) | \
((x)<<((u64)(64-((y)&(63)))))) & (0xFFFFFFFFFFFFFFFF))

#define STORE64H(x, y)                                                                     \
{ (y)[0] = (u8)(((x)>>56)&255); (y)[1] = (u8)(((x)>>48)&255);     \
(y)[2] = (u8)(((x)>>40)&255); (y)[3] = (u8)(((x)>>32)&255);     \
(y)[4] = (u8)(((x)>>24)&255); (y)[5] = (u8)(((x)>>16)&255);     \
(y)[6] = (u8)(((x)>>8)&255); (y)[7] = (u8)((x)&255); }

#define LOAD64H(x, y)                                                      \
{ x = (((u64)((y)[0] & 255))<<56)|(((u64)((y)[1] & 255))<<48) | \
(((u64)((y)[2] & 255))<<40)|(((u64)((y)[3] & 255))<<32) | \
(((u64)((y)[4] & 255))<<24)|(((u64)((y)[5] & 255))<<16) | \
(((u64)((y)[6] & 255))<<8)|(((u64)((y)[7] & 255))); }


#define Ch(x,y,z)       (z ^ (x & (y ^ z)))
#define Maj(x,y,z)      (((x | y) & z) | (x & y)) 
#define S_(x, n)         ROR64c(x, n)
#define R(x, n)         (((x) &(0xFFFFFFFFFFFFFFFF))>>((u64)n))
#define Sigma0(x)       (S_(x, 28) ^ S_(x, 34) ^ S_(x, 39))
#define Sigma1(x)       (S_(x, 14) ^ S_(x, 18) ^ S_(x, 41))
#define Gamma0(x)       (S_(x, 1) ^ S_(x, 8) ^ R(x, 7))
#define Gamma1(x)       (S_(x, 19) ^ S_(x, 61) ^ R(x, 6))
#ifndef MIN
#define MIN(x, y) ( ((x)<(y))?(x):(y) )
#endif


inline sha512_state
Sha512Create()
{
    sha512_state Result;
    
    Result.CurrentLength = 0;
    Result.Length = 0;
    Result.State[0] = 0x6a09e667f3bcc908;
    Result.State[1] = 0xbb67ae8584caa73b;
    Result.State[2] = 0x3c6ef372fe94f82b;
    Result.State[3] = 0xa54ff53a5f1d36f1;
    Result.State[4] = 0x510e527fade682d1;
    Result.State[5] = 0x9b05688c2b3e6c1f;
    Result.State[6] = 0x1f83d9abfb41bd6b;
    Result.State[7] = 0x5be0cd19137e2179;
    
    return Result;
}

internal b32
Sha512Compress(sha512_state *Sha512State, u8 *Message)
{
    b32 Result = false;
    
    u64 S[8];
    u64 W[80];
    u64 t0;
    u64 t1;
    
    // Copy State
    for(u32 Index = 0;
        Index < 8;
        ++Index)
    {
        S[Index] = Sha512State->State[Index];
    }
    
    // Copy State 1024 bits into W[0...15]
    for(u32 Index = 0;
        Index < 16;
        ++Index)
    {
        LOAD64H(W[Index], Message + (8*Index));
    }
    
    // fill W[16..79]
    for (u32 Index = 16; 
         Index < 80; 
         ++Index) 
    {
        W[Index] = Gamma1(W[Index - 2]) + W[Index - 7] + Gamma0(W[Index - 15]) + W[Index - 16];
    }        
    
    // Compress
#define RANDOM(A, B, C, D, E, F, G, H, I) \
t0 = H + Sigma1(E) + Ch(E,  F,  G) + GlobalKArray[I] + W[I]; \
t1 = Sigma0(A) + Maj(A,  B,  C);\
D += t0; \
H = t0 + t1;
    
    for (u32 Index = 0; 
         Index < 80; 
         Index += 8) 
    {
        RANDOM(S[0], S[1], S[2], S[3], S[4], S[5], S[6], S[7], Index + 0);
        RANDOM(S[7], S[0], S[1], S[2], S[3], S[4], S[5], S[6], Index + 1);
        RANDOM(S[6], S[7], S[0], S[1], S[2], S[3], S[4], S[5], Index + 2);
        RANDOM(S[5], S[6], S[7], S[0], S[1], S[2], S[3], S[4], Index + 3);
        RANDOM(S[4], S[5], S[6], S[7], S[0], S[1], S[2], S[3], Index + 4);
        RANDOM(S[3], S[4], S[5], S[6], S[7], S[0], S[1], S[2], Index + 5);
        RANDOM(S[2], S[3], S[4], S[5], S[6], S[7], S[0], S[1], Index + 6);
        RANDOM(S[1], S[2], S[3], S[4], S[5], S[6], S[7], S[0], Index + 7);
    }
    
#undef RANDOM
    
    // Feedback
    for(u32 Index = 0;
        Index < 8;
        ++Index)
    {
        Sha512State->State[Index] = Sha512State->State[Index] + S[Index];
    }
    
    return Result;
}

internal b32
Sha512Update(sha512_state *State, u8 *Message, u64 Length)
{
    b32 Result = false;
    
    Assert(State);
    Assert(Message);
    Assert(State->CurrentLength <= sizeof(State->Buffer));
    
    if((State != 0) &&
       (Message != 0) &&
       (State->CurrentLength <= sizeof(State->Buffer)))
    {
        while(Length > 0)
        {
            if((State->CurrentLength == 0) &&
               (Length >= 128))
            {
                Result = Sha512Compress(State, Message);
                if(Result)
                {
                    break;
                }
                State->Length += 128 * 8;
                Message += 128;
                Length -= 128;
            }
            else
            {
                u64 Count = Minimum(Length, (128 - State->CurrentLength));
                
                for(u64 Index = 0;
                    Index < Count;
                    ++Index)
                {
                    State->Buffer[Index + State->CurrentLength] = Message[Index];
                }
                
                State->CurrentLength += Count;
                Message += Count;
                Length -= Count;
                
                if(State->CurrentLength == 128)
                {
                    Result = Sha512Compress(State, Message);
                    if(Result)
                    {
                        break;
                    }
                    
                    State->Length += 8*128;
                    State->CurrentLength = 0;
                }
            }
        }
        
        Result = true;
    }
    else
    {
        Result = false;
    }
    
    return Result;
}

internal b32
Sha512Final(sha512_state *State, u8 *Output)
{
    b32 Result = false;
    
    Assert(State);
    Assert(Output);
    Assert(State->CurrentLength <= sizeof(State->Buffer));
    
    if((State != 0) &&
       (Output != 0) &&
       (State->CurrentLength <= sizeof(State->Buffer)))
    {
        // Increase length of the message
        State->Length += State->CurrentLength * 8;
        
        // Append the '1' bit
        State->Buffer[State->CurrentLength++] = 0x80;
        
        // If the length is currently above 1126 bytes we append zeros then 
        // compress. Then we can fall back to padding zeros and length encoding like normal.
        if(State->CurrentLength > 112)
        {
            while(State->CurrentLength < 128)
            {
                State->Buffer[State->CurrentLength++] = 0;
            }
            Sha512Compress(State, State->Buffer);
            State->CurrentLength = 0;
        }
        
        // Pad upto 120 bytes of zeros that from 112 to 120 is the 64 MSB of the length
        // We assume that you won't has > 2^64 bits of data
        while(State->CurrentLength < 120)
        {
            State->Buffer[State->CurrentLength++] = 0;
        }
        
        // Store length
        STORE64H(State->Length, State->Buffer + 120);
        Sha512Compress(State, State->Buffer);
        
        // Copy output
        for(u32 Index = 0;
            Index < 8;
            Index++)
        {
            STORE64H(State->State[Index], Output + (8 * Index));
        }
    }
    
    return Result;
}

b32 
Sha512(u8 *Message, u64 Length, u8 *Output)
{
    sha512_state State = Sha512Create();
    
    b32 Result = Sha512Update(&State, Message, Length);
    if(Result)
    {
        Result = Sha512Final(&State, Output);
    }
    
    return Result;
}

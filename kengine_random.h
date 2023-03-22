#ifndef KENGINE_RANDOM_H

typedef struct random_state
{
    u32 Value;
} random_state;

inline u32
RandomU32(random_state *State)
{
    u32 Result = State->Value;
    
    Result ^= Result << 13;
    Result ^= Result >> 17;
    Result ^= Result << 5;
    
    State->Value = Result;
    
    return Result;
}

#define KENGINE_RANDOM_H
#endif //KENGINE_RANDOM_H

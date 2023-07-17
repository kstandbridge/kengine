#ifndef KENGINE_RANDOM_H

typedef struct random_state
{
    u32 Value;
} random_state;

internal u32
RandomU32(random_state *State)
{
    u32 Result = State->Value;
    
    Result ^= Result << 13;
    Result ^= Result >> 17;
    Result ^= Result << 5;
    
    State->Value = Result;
    
    return Result;
}

internal u32
RandomU32Between(random_state *State, u32 Min, u32 Max)
{
    u32 Result = Min + (RandomU32(State) % ((Max + 1)) - Min);

    return Result;
}

internal f32
RandomUnilateral(random_state *State)
{
    f32 Divisor = 1.0f / (f32)U32Max;
    f32 Result = Divisor*(f32)RandomU32(State);

    return Result;
}

internal f32
RandomBilateral(random_state *State)
{
    f32 Result = 2.0f*RandomUnilateral(State) - 1.0f;

    return Result;
}

internal f32
RandomF32Between(random_state *State, f32 Min, f32 Max)
{
    f32 Result = LerpF32(Min, RandomUnilateral(State), Max);

    return Result;
}

#define KENGINE_RANDOM_H
#endif //KENGINE_RANDOM_H

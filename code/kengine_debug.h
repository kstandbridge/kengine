#ifndef KENGINE_DEBUG_H

#define DEBUG_FRAME_COUNT 8
typedef struct
{
    u64 BeginClock;
    u64 EndClock;
    f32 SecondsElapsed;
    
    u32 FrameIndex;
    
} debug_frame;

typedef struct debug_state
{
    u32 MostRecentFrameOrdinal;
    u32 CollationFrameOrdinal;
    u32 ViewingFrameOrdinal;
    u32 OldestFrameOrdinal;
    
    u32 TotalFrameCount;
    debug_frame Frames[DEBUG_FRAME_COUNT];
} debug_state;

#define KENGINE_DEBUG_H
#endif //KENGINE_DEBUG_H

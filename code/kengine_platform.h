#ifndef KENGINE_PLATFORM_H

#include "kengine_types.h"

#if KENGINE_SLOW
#define Assert(Expression) if(!(Expression)) {__debugbreak();}
#else
#define Assert(...)
#endif

#define InvalidCodePath Assert(!"InvalidCodePath")
#define InvalidDefaultCase default: {InvalidCodePath;} break

#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// NOTE(kstandbridge): CRT stuff

int _fltused = 0x9875;

#pragma function(memset)
void *memset(void *DestInit, int Source, size_t Size)
{
    unsigned char *Dest = (unsigned char *)DestInit;
    while(Size--) *Dest++ = (unsigned char)Source;
    
    return(DestInit);
}

#pragma function(memcpy)
void *memcpy(void *DestInit, void const *SourceInit, size_t Size)
{
    unsigned char *Source = (unsigned char *)SourceInit;
    unsigned char *Dest = (unsigned char *)DestInit;
    while(Size--) *Dest++ = *Source++;
    
    return(DestInit);
}

#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

#ifndef KENGINE_PLATFORM_H

int _fltused = 0x9875;

#pragma function(memset)
void *memset(void *_Dst, int _Val, size_t _Size)
{
    unsigned char Val = *(unsigned char *)&_Val;
    unsigned char *At = (unsigned char *)_Dst;
    while(_Size--)
    {
        *At++ = Val;
    }
    return(_Dst);
}

#define introspect(...)
#include "kengine_types.h"

#define ZeroStruct(Instance) ZeroSize(sizeof(Instance), &(Instance))
#define ZeroArray(Count, Pointer) ZeroSize(Count*sizeof((Pointer)[0]), Pointer)
inline void
ZeroSize(memory_index Size, void *Ptr)
{
    u8 *Byte = (u8 *)Ptr;
    while(Size--)
    {
        *Byte++ = 0;
    }
}


#define KENGINE_PLATFORM_H
#endif //KENGINE_PLATFORM_H

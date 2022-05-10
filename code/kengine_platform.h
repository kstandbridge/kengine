#ifndef KENGINE_PLATFORM_H

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

#include "Config.h"
#include "Core/Memory/Memory.h"

void* DoUpperAllocate()
{
    return lf::LFAlloc(420, 16);
}

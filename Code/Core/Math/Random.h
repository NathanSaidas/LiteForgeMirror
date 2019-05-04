#ifndef LF_CORE_RANDOM_H
#define LF_CORE_RANDOM_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf
{
    namespace Random
    {
        // Thread-safe random number functions:
        LF_CORE_API Int32 Rand(Int32& seed);
        LF_CORE_API Int32 Range(Int32& seed, Int32 min, Int32 max);
        LF_CORE_API UInt32 Mod(Int32& seed, UInt32 value);
        LF_CORE_API Float32 RandF(Int32& seed); // 0-1
        LF_CORE_API Float32 Range(Int32& seed, Float32 min, Float32 max);
    }
}

#endif // LF_CORE_RANDOM_H
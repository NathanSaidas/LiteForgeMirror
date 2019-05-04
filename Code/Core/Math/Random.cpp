#include "Random.h"

namespace lf
{
    namespace Random
    {
        const Int32 LCG_VAR_M = 0x7FFFFFFF; // 2 ^ 32;
        const Int32 LCG_VAR_A = 1103515245;
        const Int32 LCG_VAR_C = 12345;

        Int32 Rand(Int32& seed)
        {
            seed = (LCG_VAR_A * seed + LCG_VAR_C) % LCG_VAR_M;
            return seed;
        }
        Int32 Range(Int32& seed, Int32 min, Int32 max)
        {
            Int32 value = Rand(seed);
            return min + value % ((max + 1) - min);
        }
        UInt32 Mod(Int32& seed, UInt32 value)
        {
            return static_cast<UInt32>(Rand(seed)) % value;
        }
        Float32 RandF(Int32& seed)
        {
            Int32 value = Rand(seed);
            return static_cast<Float32>(value) / static_cast<Float32>(0x7FFFFFFF);
        }
        Float32 Range(Int32& seed, Float32 min, Float32 max)
        {
            Int32 value = Rand(seed);
            return min + static_cast<Float32>(value) / (static_cast<Float32>(0x7FFFFFFF / (max - min)));
        }
    }
}
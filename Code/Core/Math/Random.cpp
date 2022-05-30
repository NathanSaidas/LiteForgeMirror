// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#include "Core/PCH.h"
#include "Random.h"

namespace lf
{
    namespace Random
    {
        const Int32 LCG_VAR_M = 0x7FFFFFFF; // (1 << 31) - 1;
        const Int32 LCG_VAR_A = 1103515245;
        const Int32 LCG_VAR_C = 12345;

        Int32 Rand(Int32& seed)
        {
            seed = (LCG_VAR_A * seed + LCG_VAR_C) & LCG_VAR_M;
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
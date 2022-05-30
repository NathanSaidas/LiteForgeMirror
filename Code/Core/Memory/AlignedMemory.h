// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#pragma once

#include "Core/Common/Types.h"

namespace lf {

// Aligned memory is POD that allows you specify memory on the stack for a given alignment/size.

template<SizeT Size, SizeT ByteAlignment>
struct AlignedMemory
{};

#define LF_DECLARE_ALIGNED_MEMORY(alignment)        \
                                                    \
    template<SizeT Size>                            \
    struct AlignedMemory<Size,alignment>            \
    {                                               \
    public:                                         \
        LF_ALIGN(alignment) UInt8 mData[Size];      \
    };

// Declare the aligned memory for what most compilers support. (4096)

LF_DECLARE_ALIGNED_MEMORY(1);
LF_DECLARE_ALIGNED_MEMORY(2);
LF_DECLARE_ALIGNED_MEMORY(4);
LF_DECLARE_ALIGNED_MEMORY(8);
LF_DECLARE_ALIGNED_MEMORY(16);
LF_DECLARE_ALIGNED_MEMORY(32);
LF_DECLARE_ALIGNED_MEMORY(64);
LF_DECLARE_ALIGNED_MEMORY(128);
LF_DECLARE_ALIGNED_MEMORY(256);
LF_DECLARE_ALIGNED_MEMORY(512);
LF_DECLARE_ALIGNED_MEMORY(1024);
LF_DECLARE_ALIGNED_MEMORY(2048);
LF_DECLARE_ALIGNED_MEMORY(4096);





} // 
// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#ifndef LF_CORE_MEMORY_BUFFER_H
#define LF_CORE_MEMORY_BUFFER_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

class LF_CORE_API MemoryBuffer
{
public:
    MemoryBuffer(const MemoryBuffer&) = delete;
    MemoryBuffer& operator=(const MemoryBuffer&) = delete;

    MemoryBuffer();
    MemoryBuffer(MemoryBuffer&& other);
    ~MemoryBuffer();

    void Swap(MemoryBuffer& other);

    void Allocate(SizeT bytes, SizeT alignment);
    void Reallocate(SizeT bytes, SizeT alignment);
    void Free();

    void SetSize(SizeT size);
    SizeT GetSize() const { return mSize; }
    SizeT GetCapacity() const { return mCapacity; }
    void* GetData() { return mData; }
    const void* GetData() const { return mData; }
private:
    void* mData;
    SizeT mSize;
    SizeT mCapacity;
};
} // namespace lf

#endif // LF_CORE_MEMORY_BUFFER_H
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
#include "MemoryBuffer.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/Utility.h"

#include <utility>

namespace lf {

MemoryBuffer::MemoryBuffer() :
mData(nullptr),
mSize(0),
mCapacity(0)
{
}
MemoryBuffer::MemoryBuffer(MemoryBuffer&& other) :
mData(other.mData),
mSize(other.mSize),
mCapacity(other.mCapacity)
{
    other.mData = nullptr;
    other.mSize = 0;
    other.mCapacity = 0;
}
MemoryBuffer::~MemoryBuffer()
{
    Free();
}

void MemoryBuffer::Swap(MemoryBuffer& other)
{
    std::swap(mData, other.mData);
    std::swap(mSize, other.mSize);
    std::swap(mCapacity, other.mCapacity);
}
void MemoryBuffer::Allocate(SizeT bytes, SizeT alignment)
{
    Free();
    mData = LFAlloc(bytes, alignment);
    mSize = bytes;
    mCapacity = bytes;
}
void MemoryBuffer::Reallocate(SizeT bytes, SizeT alignment)
{
    void* oldData = mData;
    mData = LFAlloc(bytes, alignment);
    mSize = Min(mSize, bytes);
    mCapacity = bytes;
    if (oldData)
    {
        memcpy(mData, oldData, mSize);
        LFFree(oldData);
    }
}
void MemoryBuffer::Free()
{
    if (mData)
    {
        LFFree(mData);
        mData = nullptr;
        mCapacity = 0;
        mSize = 0;
    }
}
void MemoryBuffer::SetSize(SizeT size)
{
    mSize = Min(size, mCapacity);
}

} // namespace lf
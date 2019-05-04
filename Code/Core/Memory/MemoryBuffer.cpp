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
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
#include "MemoryBuffer.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Utility/Utility.h"

#include <cstring>
#include <utility>

namespace lf {

LF_INLINE void* AlignForward(void * address, SizeT alignment)
{
    return (void*)((reinterpret_cast<UIntPtrT>(address) + static_cast<UIntPtrT>(alignment - 1)) &  static_cast<UIntPtrT>(~(alignment - 1)));
}

MemoryBuffer::MemoryBuffer()
: mData(nullptr)
, mPaddedData(nullptr)
, mSize(0)
, mCapacity(0)
, mOwnership(DYNAMIC)
{
}
MemoryBuffer::MemoryBuffer(MemoryBuffer&& other)
: mData(other.mData)
, mPaddedData(other.mPaddedData)
, mSize(other.mSize)
, mCapacity(other.mCapacity)
, mOwnership(other.mOwnership)
{
    other.mData = nullptr;
    other.mPaddedData = nullptr;
    other.mSize = 0;
    other.mCapacity = 0;
    other.mOwnership = DYNAMIC;
}
MemoryBuffer::MemoryBuffer(void* data, SizeT capacity, Ownership ownership)
: mData(data)
, mPaddedData(data)
, mSize(0)
, mCapacity(capacity)
, mOwnership(ownership)
{
    if (mOwnership == STATIC)
    {
        // Static ownership MUST have data
        CriticalAssert(mData != nullptr);
    }
}
MemoryBuffer::MemoryBuffer(const void* data, SizeT capacity, Ownership ownership)
    : mData(const_cast<void*>(data))
    , mPaddedData(const_cast<void*>(data))
    , mSize(0)
    , mCapacity(capacity)
    , mOwnership(ownership)
{
    if (mOwnership == STATIC)
    {
        // Static ownership MUST have data
        CriticalAssert(mData != nullptr);
    }
}
MemoryBuffer::~MemoryBuffer()
{
    Free();
}

MemoryBuffer& MemoryBuffer::operator=(MemoryBuffer&& other)
{
    Free();
    mData = other.mData;
    mPaddedData = other.mPaddedData;
    mSize = other.mSize;
    mCapacity = other.mCapacity;
    mOwnership = other.mOwnership;
    other.mData = nullptr;
    other.mPaddedData = nullptr;
    other.mSize = 0;
    other.mCapacity = 0;
    other.mOwnership = DYNAMIC;
    return *this;
}

void MemoryBuffer::Swap(MemoryBuffer& other)
{
    std::swap(mData, other.mData);
    std::swap(mPaddedData, other.mPaddedData);
    std::swap(mSize, other.mSize);
    std::swap(mCapacity, other.mCapacity);
    std::swap(mOwnership, other.mOwnership);
}

void MemoryBuffer::Copy(const MemoryBuffer& other)
{
    Free();
    Allocate(other.GetSize(), 1); // TODO: We should track alignment
    SetSize(other.GetSize());
    memcpy_s(GetData(), GetSize(), other.GetData(), other.GetSize());
}

bool MemoryBuffer::Allocate(SizeT bytes, SizeT alignment)
{
    bool success = false;
    switch (mOwnership)
    {
        case STATIC:
        {
            void* aligned = AlignForward(mData, alignment);
            const SizeT padded = reinterpret_cast<UIntPtrT>(aligned) - reinterpret_cast<UIntPtrT>(mData);
            const SizeT available = mCapacity - padded;
            if (bytes > available) // not enough memory
            {
                break;
            }
            mPaddedData = aligned;
            mSize = bytes;
            success = true;
        } break;
        case DYNAMIC:
        {
            Free();
            mData = LFAlloc(bytes, alignment);
            if (mData == nullptr)
            {
                break;
            }
            mPaddedData = mData;
            mSize = bytes;
            mCapacity = bytes;
            success = true;
        } break;
        default:
        {
            
        } break;
    }
    
    return success;
}
bool MemoryBuffer::Reallocate(SizeT bytes, SizeT alignment)
{
    bool success = false;
    switch (mOwnership)
    {
        case STATIC:
        {
            if (!mPaddedData)
            {
                success = Allocate(bytes, alignment);
                break;
            }

            void* aligned = AlignForward(mData, alignment);
            const SizeT padded = reinterpret_cast<UIntPtrT>(aligned) - reinterpret_cast<UIntPtrT>(mData);
            const SizeT available = mCapacity - padded;
            if (bytes > available) // not enough memory
            {
                break;
            }

            if (mPaddedData != aligned) // Cannot change alignment on static memory
            {
                break;
            }
            mSize = Min(mSize, bytes);
            success = true;
        } break;
        case DYNAMIC:
        {
            void* oldData = mData;
            mData = LFAlloc(bytes, alignment);
            mPaddedData = mData;
            mSize = Min(mSize, bytes);
            mCapacity = bytes;
            if (oldData)
            {
                memcpy(mData, oldData, mSize);
                LFFree(oldData);
            }
        } break;
        default:
        {
             
        } break;
    }

    return success;

    
}
void MemoryBuffer::Free()
{
    if (mData)
    {
        if (mOwnership == DYNAMIC)
        {
            LFFree(mData);
        }
        mData = nullptr;
        mPaddedData = nullptr;
        mCapacity = 0;
        mSize = 0;
        mOwnership = DYNAMIC;
    }
}
void MemoryBuffer::SetSize(SizeT size)
{
    mSize = Min(size, mCapacity);
}

SizeT MemoryBuffer::GetAlignedCapacity() const
{
    const SizeT padding = reinterpret_cast<UIntPtrT>(mPaddedData) - reinterpret_cast<UIntPtrT>(mData);
    return mCapacity - padding;
}

} // namespace lf
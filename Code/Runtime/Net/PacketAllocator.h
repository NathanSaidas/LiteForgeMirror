#pragma once
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
#include "Core/Memory/DynamicPoolHeap.h"
#include "Core/Net/NetTypes.h"
#include <utility>

namespace lf {

template<typename T>
class TPacketAllocator
{
public:
    TPacketAllocator(const TPacketAllocator& other) = delete;
    TPacketAllocator& operator=(const TPacketAllocator& other) = delete;


    TPacketAllocator();
    TPacketAllocator(TPacketAllocator&& other);
    ~TPacketAllocator();
    TPacketAllocator& operator=(TPacketAllocator&& other);
    
    bool Initialize(SizeT objectCount, SizeT maxHeaps, UInt32 flags);
    void Release();
    void GCCollect();

    T* Allocate();
    void Free(T* packet);

    DynamicPoolHeap& GetHeap() { return mHeap; }
    const DynamicPoolHeap& GetHeap() const { return mHeap; }

private:
    DynamicPoolHeap mHeap;
};

template<typename T>
TPacketAllocator<T>::TPacketAllocator()
: mHeap()
{

}

template<typename T>
TPacketAllocator<T>::TPacketAllocator(TPacketAllocator<T>&& other)
: mHeap(std::move(other.mHeap))
{

}

template<typename T>
TPacketAllocator<T>::~TPacketAllocator()
{

}
template<typename T>
TPacketAllocator<T>& TPacketAllocator<T>::operator=(TPacketAllocator&& other)
{
    if (this != &other)
    {
        mHeap = std::move(other.mHeap);
    }
    return *this;
}

template<typename T>
bool TPacketAllocator<T>::Initialize(SizeT objectCount, SizeT maxHeaps, UInt32 flags)
{
    return mHeap.Initialize(sizeof(T), alignof(T), objectCount, maxHeaps, flags);
}

template<typename T>
void TPacketAllocator<T>::Release()
{
    mHeap.Release();
}

template<typename T>
void TPacketAllocator<T>::GCCollect()
{
    mHeap.GCCollect();
}

template<typename T>
T* TPacketAllocator<T>::Allocate()
{
    void* object = mHeap.Allocate();
    if (object)
    {
        return new(object)T();
    }
    return nullptr;
}

template<typename T>
void TPacketAllocator<T>::Free(T* packet)
{
    if (packet)
    {
        packet->~T();
        PacketData::SetZero(*packet);
        mHeap.Free(packet);
    }
}

} // namespace
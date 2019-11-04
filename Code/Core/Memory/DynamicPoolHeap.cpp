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

#include "DynamicPoolHeap.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"

namespace lf {


void DynamicPoolHeap::Node::SetNext(Node* value) { AtomicStorePointer(&mNext, value); }
DynamicPoolHeap::Node* DynamicPoolHeap::Node::GetNext() { return AtomicLoadPointer(&mNext); }
const DynamicPoolHeap::Node* DynamicPoolHeap::Node::GetNext() const { return AtomicLoadPointer(&mNext); }

bool DynamicPoolHeap::Node::IsGarbage() const { return AtomicLoad(&mGarbage) != 0; }
void DynamicPoolHeap::Node::SetIsGarbage(bool value) { return AtomicStore(&mGarbage, value ? 1 : 0); }
bool DynamicPoolHeap::Node::MarkGarbage()
{
    Atomic32 prev = AtomicCompareExchange(&mGarbage, 1, 0);
    return prev == 0;
}
bool DynamicPoolHeap::Node::MarkRecycle()
{
    // note: We should spin at most x2, it's still unclear whether or not we need to spin at all
    //       all I want to guarantee is that when we recycle, if we recycle while marking for garbage
    //       the recycle will overrule the garbage.
    SizeT spin = 1000;
    while (spin != 0)
    {
        if (AtomicCompareExchange(&mGarbage, 0, 1) == 0)
        {
            return true;
        }
        --spin;
    }
    return false;
}

DynamicPoolHeap::DynamicPoolHeap() 
    : mTop()
    , mGCLock()
    , mPushHeapLock()
    , mHeapCount(0)
    , mGarbageHeapCount(0)
{

}
DynamicPoolHeap::DynamicPoolHeap(DynamicPoolHeap&& other)
    : mTop()
    , mGCLock()
    , mPushHeapLock()
    , mHeapCount(AtomicLoad(&other.mHeapCount))
    , mGarbageHeapCount(AtomicLoad(&other.mGarbageHeapCount))
{
    ScopeRWLockWrite lock(other.mGCLock);

    mTop.GetHeap() = std::move(other.mTop.GetHeap());
    mTop.SetNext(other.mTop.GetNext());
    mTop.SetIsGarbage(other.mTop.IsGarbage());

    other.mTop.SetNext(nullptr);
    other.mTop.SetIsGarbage(true);
    AtomicStore(&other.mHeapCount, 0);
    AtomicStore(&other.mGarbageHeapCount, 0);
    _ReadWriteBarrier();
}
DynamicPoolHeap::~DynamicPoolHeap()
{
    Release();
}

DynamicPoolHeap& DynamicPoolHeap::operator=(DynamicPoolHeap&& other)
{
    if (this == &other)
    {
        return *this;
    }
    Release();

    ScopeRWLockWrite lock(mGCLock);
    ScopeRWLockWrite otherLock(other.mGCLock);

    mTop.GetHeap() = std::move(other.mTop.GetHeap());
    mTop.SetNext(other.mTop.GetNext());
    mTop.SetIsGarbage(other.mTop.IsGarbage());

    other.mTop.SetNext(nullptr);
    other.mTop.SetIsGarbage(true);
    AtomicStore(&other.mHeapCount, 0);
    AtomicStore(&other.mGarbageHeapCount, 0);
    _ReadWriteBarrier();
    return *this;
}

bool DynamicPoolHeap::Initialize(SizeT objectSize, SizeT objectAlignment, SizeT numObjects, SizeT maxHeaps, UInt32 flags)
{
    if (!mTop.GetHeap().Initialize(objectSize, objectAlignment, numObjects, flags))
    {
        return false;
    }

    AtomicStore(&mHeapCount, 1);
    Assert(AtomicLoad(&mGarbageHeapCount) == 0);
    mMaxHeaps = maxHeaps;
    mTop.SetIsGarbage(false);
    mTop.SetNext(nullptr);
    return true;
}

void DynamicPoolHeap::Release()
{
    ScopeRWLockWrite lock(mGCLock);
    RecursiveFree(mTop.GetNext());
    mTop.GetHeap().Release();
    mTop.SetIsGarbage(true);
    mTop.SetNext(nullptr);

    AtomicStore(&mHeapCount, 0);
    AtomicStore(&mGarbageHeapCount, 0);
}

void* DynamicPoolHeap::Allocate()
{
    ScopeRWLockRead lock(mGCLock);
    // Heap not initialized.
    if (mTop.IsGarbage())
    {
        return nullptr;
    }

    void* pointer = mTop.GetHeap().Allocate();
    if (pointer)
    {
        return pointer;
    }
    else
    {
        Node* it = mTop.GetNext();
        while (it)
        {
            if (!it->IsGarbage())
            {
                pointer = it->GetHeap().Allocate();
                if (pointer)
                {
                    return pointer;
                }
            }
            it = it->GetNext();
        }
        it = PushHeap(pointer);
        return pointer;
    }
}

void DynamicPoolHeap::Free(void* pointer)
{
    ScopeRWLockRead lock(mGCLock);
    // Heap not initialized.
    if (mTop.IsGarbage())
    {
        return;
    }

    Node* it = &mTop;
    while (it)
    {
        // Iterate through all heaps until we find the owner of this memory,
        // if there are no owners we'll crash below.
        if (it->GetHeap().IsOwnerOf(pointer))
        {
            it->GetHeap().Free(pointer);
            // If the heap is not the 'root' and it no longer has objects allocated
            // then we'll mark it as garbage. It'll be swept later when it still has
            // 0 allocations. Allocate cannot allocate from 'garbage' heaps however
            // we could still get one allocation in the same moment as we mark it as
            // garbage. Hence why we use a GC
            if (it != &mTop && it->GetHeap().GetAllocations() == 0)
            {
                if (it->MarkGarbage())
                {
                    Assert(AtomicIncrement32(&mGarbageHeapCount) <= (mMaxHeaps - 1));
                }
            }
            return;
        }
        it = it->GetNext();
    }
    AssertMsg("Attempting to free pointer not owned by any pool heaps. Possible memory leak has occured");
}

void DynamicPoolHeap::GCCollect()
{
    ScopeRWLockWrite lock(mGCLock);
    if (mTop.IsGarbage())
    {
        return;
    }

    Node* it = &mTop;
    while (it->GetNext())
    {
        if (it->GetNext()->IsGarbage() && it->GetNext()->GetHeap().GetAllocations() == 0)
        {
            PopNext(it);
        }
        else
        {
            it = it->GetNext();
        }
    }
}

SizeT DynamicPoolHeap::GetGarbageHeapCount() const
{
    return AtomicLoad(&mGarbageHeapCount);
}
SizeT DynamicPoolHeap::GetHeapCount() const
{
    return AtomicLoad(&mHeapCount);
}
SizeT DynamicPoolHeap::GetAllocations() const
{
    ScopeRWLockRead lock(mGCLock);
    
    SizeT allocations = 0;
    const Node* it = &mTop;
    while (it)
    {
        allocations += it->GetHeap().GetAllocations();
        it = it->GetNext();
    }
    return allocations;
}

SizeT DynamicPoolHeap::GetMaxAllocations() const
{
    return mTop.GetHeap().GetObjectCount() * mMaxHeaps;
}

SizeT DynamicPoolHeap::GetObjectSize() const
{
    return mTop.GetHeap().GetObjectSize();
}

SizeT DynamicPoolHeap::GetObjectAlignment() const
{
    return mTop.GetHeap().GetObjectAlignment();
}

void DynamicPoolHeap::RecursiveFree(Node* node)
{
    if (node)
    {
        RecursiveFree(node->GetNext());
        node->GetHeap().Release();
        LFDelete(node);
    }
}

DynamicPoolHeap::Node* DynamicPoolHeap::PushHeap(void*& outPointer)
{
    outPointer = nullptr;
    ScopeLock lock(mPushHeapLock);

    // 1. a) Someone may have 'just' added a heap
    //    b) A heap may have 'just' been marked as garbage or was garbage and can be reclaimed.
    //

    Node* last = &mTop;
    Node* recycleHeap = nullptr;
    Node* lastNotGarbage = nullptr;
    while (last->GetNext())
    {
        last = last->GetNext();
        if (!last->IsGarbage())
        {
            lastNotGarbage = last;
        }
        else if(!recycleHeap && !last->GetHeap().IsOutOfMemory())
        {
            recycleHeap = last;
        }
    }

    // Successfully allocated using the last not garbage, caller should not allocate
    if (lastNotGarbage)
    {
        outPointer = lastNotGarbage->GetHeap().Allocate();
        if (outPointer != nullptr) 
        {
            return lastNotGarbage;
        }
    }

    // Successfully allocated a pointer from a recycled heap.
    if (recycleHeap)
    {
        outPointer = recycleHeap->GetHeap().Allocate();
        Assert(outPointer);
        recycleHeap->MarkRecycle();
        // recycleHeap->SetIsGarbage(false);
        AtomicDecrement32(&mGarbageHeapCount);
        return recycleHeap;
    }

    if (AtomicLoad(&mHeapCount) >= mMaxHeaps)
    {
        return nullptr;
    }

    // Oops: No more memory we have to allocate a new heap
    Node* next = LFNew<Node>();
    if (next == nullptr)
    {
        return nullptr;
    }
    next->GetHeap().Initialize(
        mTop.GetHeap().GetObjectSize(), 
        mTop.GetHeap().GetObjectAlignment(), 
        mTop.GetHeap().GetObjectCount(), 
        mTop.GetHeap().GetFlags());
    next->SetIsGarbage(false);
    next->SetNext(nullptr);
    if (next->GetHeap().IsOutOfMemory())
    {
        next->GetHeap().Release(); // In theory this should do nothing as we failed to initialize the heap!
        LFDelete(next);
        return nullptr;
    }

    outPointer = next->GetHeap().Allocate();
    Assert(outPointer);
    AtomicIncrement32(&mHeapCount);
    // In the case that we're inserting before garbage heaps.
    if (last->GetNext())
    {
        next->SetNext(last->GetNext());
    }
    last->SetNext(next);
    return next;

    // // Find the last 'non-garbage' node, but also if there is one
    // // that is garbage and its not out of memory then recycle it.
    // Node* last = &mTop;
    // Node* lastNotGarbage = &mTop;
    // while (last->GetNext())
    // {
    //     last = last->GetNext();
    //     if (!last->IsGarbage())
    //     {
    //         lastNotGarbage = last;
    //         outPointer = last->GetHeap().Allocate();
    //         if (outPointer)
    //         {
    //             return nullptr;
    //         }
    //         continue;
    //     }
    //     // recycle the heap:
    //     else if(!last->GetHeap().IsOutOfMemory())
    //     {
    //         last->SetIsGarbage(false);
    //         AtomicDecrement32(&mGarbageHeapCount);
    //         return last;
    //     }
    // }
    // last = lastNotGarbage;
    // // Assert(last->GetHeap().IsOutOfMemory());, can't really assert as someone could free in between
    // // however we can assume its out of memory 
    // 
    // if (last->GetHeap().IsOutOfMemory())
    // {
    //     if (AtomicLoad(&mHeapCount) >= mMaxHeaps)
    //     {
    //         return nullptr;
    //     }
    // 
    //     Node* next = LFNew<Node>();
    //     next->GetHeap().Initialize(
    //         mTop.GetHeap().GetObjectSize(), 
    //         mTop.GetHeap().GetObjectAlignment(), 
    //         mTop.GetHeap().GetObjectCount(), 
    //         mTop.GetHeap().GetFlags());
    //     next->SetIsGarbage(false);
    //     next->SetNext(nullptr);
    //     if (next->GetHeap().IsOutOfMemory())
    //     {
    //         LFDelete(next);
    //         return nullptr;
    //     }
    //     AtomicIncrement32(&mHeapCount);
    //     // In the case that we're inserting before garbage heaps.
    //     if (last->GetNext())
    //     {
    //         next->SetNext(last->GetNext());
    //     }
    //     last->SetNext(next);
    //     return next;
    // }
    // else
    // {
    //     return last;
    // }
}
void DynamicPoolHeap::PopNext(Node* node)
{
    CriticalAssert(node->GetNext()->IsGarbage() && node->GetNext()->GetHeap().GetAllocations() == 0);
    CriticalAssert(node->GetNext() != &mTop);
    Node* next = node->GetNext()->GetNext();
    // release:
    node->GetNext()->GetHeap().Release();
    LFDelete(node->GetNext());
    AtomicDecrement32(&mHeapCount);
    AtomicDecrement32(&mGarbageHeapCount);
    // pop:
    node->SetNext(next);
}


} // namespace lf 
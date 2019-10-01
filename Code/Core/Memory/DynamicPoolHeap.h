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
#ifndef LF_CORE_DYNAMIC_POOL_HEAP_H
#define LF_CORE_DYNAMIC_POOL_HEAP_H

#include "Core/Memory/PoolHeap.h"
#include "Core/Platform/RWSpinLock.h"

namespace lf {

// Extends the features of PoolHeap by allowing the 'pool' to grow and shrink.
class LF_CORE_API DynamicPoolHeap
{
private:
    struct Node
    {
    public:
        PoolHeap& GetHeap() { return mHeap; }
        const PoolHeap& GetHeap() const { return mHeap; }

        void SetNext(Node* value);
        Node* GetNext();
        const Node* GetNext() const;

        bool IsGarbage() const;
        void SetIsGarbage(bool value);

        bool MarkGarbage();
        bool MarkRecycle();
    private:
        PoolHeap mHeap;
        Node* volatile mNext;
        volatile Atomic32 mGarbage;
    };
public:
    DynamicPoolHeap(const DynamicPoolHeap&) = delete;
    DynamicPoolHeap& operator=(const DynamicPoolHeap&) = delete;

    DynamicPoolHeap();
    DynamicPoolHeap(DynamicPoolHeap&& other);
    ~DynamicPoolHeap();

    DynamicPoolHeap& operator=(DynamicPoolHeap&& other);



    // **********************************
    // Initializes the pool with a set of objects, the memory is considered owned and will be 
    // free'd when the heap goes out of scope or an explicit call to Release is made.
    // 
    // @param objectSize -- The size of each object
    // @param objectAlignment -- The alignment of which each object should be aligned.
    // @param numObjects -- The number of objects the pool will reserve.
    // @return Returns true if the pool was created successfully.
    // **********************************
    bool Initialize(SizeT objectSize, SizeT objectAlignment, SizeT numObjects, SizeT maxHeaps = 3, UInt32 flags = 0);

    // **********************************
    // Releases all memory from the heap.
    //
    // note: It is assumed that no more allocate/free operations may occur while attempting to release.
    // **********************************
    void Release();
    // **********************************
    // todo:
    // **********************************
    void* Allocate();
    // **********************************
    // todo:
    // **********************************
    void Free(void* pointer);
    // **********************************
    // todo:
    // **********************************
    void GCCollect();

    // **********************************
    // todo:
    // **********************************
    SizeT GetGarbageHeapCount() const;
    // **********************************
    // todo:
    // **********************************
    SizeT GetHeapCount() const;
    // **********************************
    // todo:
    // **********************************
    SizeT GetAllocations() const;
    // **********************************
    // todo:
    // **********************************
    SizeT GetMaxAllocations() const;
private:
    void RecursiveFree(Node* node);

    Node* PushHeap(void*& outPointer);
    void PopNext(Node* node);

    Node      mTop;

    // A lock used to ensure that Allocate/Free can both access the linked-list
    // safetly. GC operations might 'pop' a node out from the linked list.
    //
    // 
    mutable RWSpinLock mGCLock;
    // A lock used to ensure that callers of the 'Allocate' method all synchronize
    // and don't push multiple heaps. They must wait then try to allocate again before
    // attempting to push another heap.
    SpinLock   mPushHeapLock;

    SizeT mMaxHeaps;
    volatile Atomic32 mHeapCount;
    volatile Atomic32 mGarbageHeapCount;
};

} // namespace lf

#endif // LF_CORE_DYNAMIC_POOL_HEAP_H
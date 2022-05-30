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
#pragma once
#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Platform/SpinLock.h"

namespace lf {

class LF_CORE_API PoolHeap
{
public:
    enum Flags
    {
        //
        PHF_DOUBLE_FREE = 1 << 0,
        // Verifies all memory was freed when the heap is released.
        PHF_LEAK = 1 << 1,
        // Adds a single header/footer page to the contiguous block of memory.
        PHF_DETECT_GLOBAL_HEAP_CORRUPTION = 1 << 2,
        // Adds a single footer to all object allocations.
        PHF_DETECT_LOCAL_HEAP_CORRUPTION = 1 << 3,
        // Signals a callback that the heap ran out of memory.
        // !Signal
        PHF_OUT_OF_MEMORY = 1 << 4,
        // !Signal
        PHF_INVALID_OWNERSHIP = 1 << 5
    };

    // **********************************
    // Copy construction is not allowed.
    // **********************************
    PoolHeap(const PoolHeap&) = delete;
    // **********************************
    // Copy assignment is not allowed.
    // **********************************
    PoolHeap& operator=(const PoolHeap&) = delete;

    // **********************************
    // Default constructor initializes pool default values, no objects are allocated.
    // **********************************
    PoolHeap();
    // **********************************
    // Move constructor just moves objects from one pool to another.
    // **********************************
    PoolHeap(PoolHeap&& other);
    ~PoolHeap();

    // **********************************
    // Move assignment, do not use this across multiple threads as it could incur a deadlock
    //
    // It is safe however to do single moves while maintaining thread safe allocate/free
    // **********************************
    PoolHeap& operator=(PoolHeap&& other);
    
    // **********************************
    // Initializes the pool with a set of objects, the memory is considered owned and will be 
    // free'd when the heap goes out of scope or an explicit call to Release is made.
    // 
    // @param objectSize -- The size of each object
    // @param objectAlignment -- The alignment of which each object should be aligned.
    // @param numObjects -- The number of objects the pool will reserve.
    // @return Returns true if the pool was created successfully.
    // **********************************
    bool Initialize(SizeT objectSize, SizeT objectAlignment, SizeT numObjects, UInt32 flags = 0);
    // **********************************
    // Initializes the pool with a set of objects, the memory is considered owned and will be 
    // free'd when the heap goes out of scope or an explicit call to Release is made.
    // 
    // @param memory -- The memory provided to the heap to initialize the objects from. (Must be the correct size/alignment)
    // @param capacity -- The number of bytes available in the 'memory' argument.
    // @param objectSize -- The size of each object
    // @param objectAlignment -- The alignment of which each object should be aligned.
    // @param numObjects -- The number of objects the pool will reserve.
    // @return Returns true if the pool was created successfully.
    // **********************************
    // bool Initialize(void* memory, SizeT capacity, SizeT objectSize, SizeT objectAlignment, SizeT numObjects, UInt32 flags = 0);

    // **********************************
    // Releases all memory from the heap.
    //
    // note: It is assumed that no more allocate/free operations may occur while attempting to release.
    // **********************************
    void Release();

    // **********************************
    // Allocates an object from the heap.
    // 
    // note: This method is thread safe w/ Free
    // @return Returns a pointer to the next free object. If the heap is full then a nullptr is returned.
    // **********************************
    void* Allocate();
    // **********************************
    // Frees an object from the heap. If the 'pointer' does not belong to this heap it will simply return.
    // If the pointer has already been free'd and the application is running with LF_MEMORY_CHECK_DOUBLE_FREE
    // an assert is made.
    // 
    // note: This method is thread safe w/ Allocate
    // **********************************
    void  Free(void* pointer);

    // **********************************
    // Checks if the 'pointer' is owned by this heap.
    // @param pointer -- The pointer to check for ownership.
    // @returns Returns true if the 'pointer' is owned by the heap.
    // **********************************
    bool IsOwnerOf(const void* pointer);
    // **********************************
    // Checks if the heap is at capacity. (All objects are allocated)
    // 
    // **********************************
    bool IsOutOfMemory() const;
    // **********************************
    // @returns Returns the number of allocated objects.
    // **********************************
    SizeT GetAllocations() const;
    // **********************************
    // todo:
    // **********************************
    SizeT GetObjectSize() const { return mObjectSize; }
    // **********************************
    // todo:
    // **********************************
    SizeT GetObjectAlignment() const { return mObjectAlignment; }
    // **********************************
    // todo:
    // **********************************
    SizeT GetObjectCount() const { return mSize; }
    // **********************************
    // todo:
    // **********************************
    UInt32 GetFlags() const { return mFlags; }
private:
    // The next 'free' object in the pool.
    void* volatile mTop;
    // Base address of the memory allocated.
    void* mBase;
    // The address of the last object.
    void* mLast;
    SpinLock mLock;

    // The size of each object
    SizeT mObjectSize;
    // The alignment of each object.
    SizeT mObjectAlignment;
    // The number of objects allocated.
    SizeT mSize;
    UInt32 mFlags;
    volatile Atomic32 mAllocations;

#if defined(LF_OS_WINDOWS)
    // void* mProtected;
#endif
#if defined(LF_MEMORY_DEBUG)
    volatile Atomic16* mStatus;
    void* mObjectBase;
#endif
};


// template<typename T>
// class TPoolHeap
// {
// public:
// 
//     void Create(SizeT numObjects);
//     void Release();
// 
// 
//     T* Allocate();
//     void Free(T* pointer);
// 
// private:
//     PoolHeap mHeap;
// };

// todo: PoolHeap
// todo: TPoolHeap< Object, ObjectIsPOD > // for non-POD constructs the object
// todo: DynamicPoolHeap // If one heap fails, allocate another
// todo: TDynamicPoolHeap< Object ObjectIsPOD > // If one fails
//


} // namespace lf
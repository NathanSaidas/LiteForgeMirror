// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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

namespace lf {

// ********************************************************************
// A TempHeap is a type of memory heap that 
// preallocates a fixed size of memory and 
// simply advances a 'current' pointer to the next
// address on allocation.
// 
// There is no memory deallocation and the only way
// to 'reclaim' memory is call Reset, at which point
// all memory allocated before that is considered
// 'invalid'
// 
// In general TempHeap is designed for fast dynamic
// memory allocation of temporary variables.
// ********************************************************************
class LF_CORE_API TempHeap
{
public:
    // ********************************************************************
    // Provide default values.
    // ********************************************************************
    TempHeap();
    // ********************************************************************
    // Destructor - Release Memory
    // 
    // @policy Memory.Heap - Release must be called before destructor
    // ********************************************************************
    ~TempHeap();
    // ********************************************************************
    // Copy Constructor - Deleted, not allowed.
    // ********************************************************************
    TempHeap(const TempHeap&) = delete;
    // ********************************************************************
    // Copy Assignment - Deleted, not allowed.
    // ********************************************************************
    TempHeap& operator=(const TempHeap&) = delete;

    // ********************************************************************
    // Initialize a TempHeap of numBytes on a specified initial alignment.
    // ********************************************************************
    bool Initialize(SizeT numBytes, SizeT initialAlignment = 1);
    // ********************************************************************
    // Initialize a TempHeap with a pointer to memory.
    // 
    // NOTE: This memory is not free'd on Release.
    // ********************************************************************
    bool Initialize(void* memory, SizeT numBytes);
    // ********************************************************************
    // Calls Reset and releases any owned memory.
    // ********************************************************************
    void Release();

    // ********************************************************************
    // Attempts to allocate memory of the specified size/alignment.
    // ********************************************************************
    void* Allocate(SizeT size, SizeT alignment);

    // ********************************************************************
    // Resets the allocator 'current' pointer back to base, all previous
    // allocations are considered 'invalid'.
    // ********************************************************************
    void Reset();

    bool Empty() const { return mBasePointer == mEndPointer; }
private:
    // Pointer to the allocated memory
    void* mBasePointer;
    // Size of the allocation
    void* mEndPointer;
    // Pointer to the current place in memory.
    void* mCurrentPointer;
    // Whether or not the memory should be free'd
    bool  mHeapAllocated;
};

} // namespace lf
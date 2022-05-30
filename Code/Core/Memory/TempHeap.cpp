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
#include "Core/PCH.h"
#include "TempHeap.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"

namespace lf {

///This method will calculate the alignment needed for an address and return the adjustment required to align the address
///Eg...
static UIntPtrT AlignForwardAdjustment(void* aAddress, SizeT aAlignment)
{
    UIntPtrT adjustment = aAlignment - (reinterpret_cast<UIntPtrT>(aAddress) & static_cast<UIntPtrT>(aAlignment - 1));
    if (adjustment == aAlignment)
    {
        return 0;
    }
    return adjustment;
}

TempHeap::TempHeap()
: mBasePointer(nullptr)
, mEndPointer(nullptr)
, mCurrentPointer(nullptr)
, mHeapAllocated(false)
{
}
TempHeap::~TempHeap()
{
    CriticalAssert(mBasePointer == mEndPointer);
}

bool TempHeap::Initialize(SizeT numBytes, SizeT initialAlignment)
{
    CriticalAssert(mBasePointer == mEndPointer); // Must be empty.
    ReportBug(numBytes > 0);
    if (numBytes == 0)
    {
        return false;
    }
    ReportBug(initialAlignment > 0 && initialAlignment <= 4096); // Reasonable Alignment.
    if (initialAlignment == 0 || initialAlignment > 4096)
    {
        return false;
    }

    mBasePointer = LFAlloc(numBytes, initialAlignment);
    if (mBasePointer == nullptr)
    {
        return false;
    }
    mEndPointer = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(mBasePointer) + numBytes);
    mCurrentPointer = mBasePointer;
    mHeapAllocated = true;
    return true;
}
bool TempHeap::Initialize(void* memory, SizeT numBytes)
{
    CriticalAssert(mBasePointer == mEndPointer); // Must be empty.
    ReportBug(memory != nullptr);
    if (memory == nullptr)
    {
        return false;
    }
    ReportBug(numBytes > 0);
    if (memory == nullptr)
    {
        return false;
    }
    mBasePointer = memory;
    mEndPointer = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(mBasePointer) + numBytes);
    mCurrentPointer = mBasePointer;
    mHeapAllocated = false;
    return true;
}
void TempHeap::Release()
{
    if (mBasePointer == mEndPointer)
    {
        ReportBugMsg("Invalid operation trying to release TempHeap. It's not allocated!");
        return;
    }

    if (mHeapAllocated)
    {
        LFFree(mBasePointer);
    }
    mBasePointer = nullptr;
    mEndPointer = nullptr;
    mCurrentPointer = nullptr;
    mHeapAllocated = false;
}

void* TempHeap::Allocate(SizeT size, SizeT alignment)
{
    UIntPtrT adjustment = AlignForwardAdjustment(mCurrentPointer, alignment);
    void* targetEnd = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(mCurrentPointer) + (adjustment + size));
    if (targetEnd > mEndPointer)
    {
        return nullptr;
    }
    void* pointer = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(mCurrentPointer) + adjustment);
    mCurrentPointer = targetEnd;
    return pointer;
}
void TempHeap::Reset()
{
    mCurrentPointer = mBasePointer;
}

} // namespace lf
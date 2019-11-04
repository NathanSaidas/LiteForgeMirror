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
#include "PoolHeap.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/Atomic.h"
#include "Core/Utility/ErrorCore.h"

#include <utility>
#include <cmath>
#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {

static SizeT CalculateNumPages(SizeT size, const SizeT PAGE_SIZE)
{
#if defined(LF_OS_WINDOWS)
    

    // 40   -> 1
    // 400  -> 1
    // 4095 -> 1
    // 4096 -> 1
    // 4097 -> 2

    SizeT numPages = static_cast<SizeT>(ceil(size / static_cast<Float64>(PAGE_SIZE)));
    return numPages;
#else
    LF_STATIC_CRASH("Missing implementation.");
    return 0;
#endif
}

LF_INLINE void* AlignForward(void * address, SizeT alignment)
{
    return (void*)((reinterpret_cast<UIntPtrT>(address) + static_cast<UIntPtrT>(alignment - 1)) &  static_cast<UIntPtrT>(~(alignment - 1)));
}

// Allocate memory for all objects and initialize the stack. (linked list)
void* InitializeNormal(SizeT objectSize, SizeT objectAlignment, SizeT numObjects)
{
    void* base = LFAlloc(objectSize * numObjects, objectAlignment);
    if (base == nullptr)
    {
        return nullptr;
    }
    CriticalAssertEx(reinterpret_cast<UIntPtrT>(base) % objectAlignment == 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    void** obj = reinterpret_cast<void**>(base);
    for (SizeT i = 0; i < (numObjects - 1); ++i)
    {
        *obj = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(obj) + objectSize);
        obj = reinterpret_cast<void**>(*obj);
    }
    *obj = nullptr;
    return base;
}

// Allocate page protected memory and initialize the linked list.
//
// +------------------------+---------------+------------------------+---------------+------------------------+
// | Global Protected Header | Object Memory | Local Protected Footer | Object Memory | Local Protected Footer |
// +------------------------+---------------+------------------------+---------------+------------------------+
//                                  v                                      ^
//                                  |                                      |
//                                  >------------------------------------->|
//
void* InitializeLocalHeapCorruption(SizeT objectSize, SizeT objectAlignment, SizeT numObjects, void*& top, void*& last)
{
    top = nullptr;
    last = nullptr;

#if defined(LF_OS_WINDOWS)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    

    const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
    // If the alignment is greater than page size for some reason, we cannot guarantee we'll allocate enough
    // memory to align properly.
    CriticalAssert(objectAlignment < PAGE_SIZE);

    const SizeT numObjectPages = CalculateNumPages(objectSize + objectAlignment, PAGE_SIZE);
    const SizeT totalObjectPages = (numObjectPages + 1) * numObjects; // Each object has a protected header
    const SizeT totalPages = totalObjectPages + 1; // +1 for Global Protected Header.
    const SizeT memSize = totalPages * PAGE_SIZE;
    void* memory = VirtualAlloc(NULL, memSize, MEM_COMMIT, PAGE_READWRITE);

    void* globalProtectedHeader = memory;
    memset(globalProtectedHeader, 0xCC, PAGE_SIZE);
    DWORD oldProtect = 0;
    Assert(VirtualProtect(globalProtectedHeader, 1, PAGE_READONLY, &oldProtect) == TRUE);

    void* base = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(memory) + PAGE_SIZE);

    const SizeT pagedObjectSize = numObjectPages * PAGE_SIZE;
    void* obj = base;
    for (SizeT i = 0; i < (numObjects - 1); ++i)
    {
        UIntPtrT address = reinterpret_cast<UIntPtrT>(obj);
        memset(obj, 0xCC, (numObjectPages + 1) * PAGE_SIZE);

        
        // Protect Footer:
        void* footer = reinterpret_cast<void*>(address + pagedObjectSize);
        oldProtect = 0;
        Assert(VirtualProtect(footer, 1, PAGE_READONLY, &oldProtect) == TRUE);

        // Align memory
        void* aligned = AlignForward(obj, objectAlignment);
        if (top == nullptr)
        {
            top = aligned;
        }
        // Initialize stack
        void* next = AlignForward(reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(footer) + PAGE_SIZE), objectAlignment);
        *reinterpret_cast<void**>(aligned) = next;
        obj = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(footer) + PAGE_SIZE);
    }

    {
        memset(obj, 0xCC, (numObjectPages + 1) * PAGE_SIZE);

        // Protect Footer:
        void* footer = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(obj) + pagedObjectSize);
        oldProtect = 0;
        Assert(VirtualProtect(footer, 1, PAGE_READONLY, &oldProtect) == TRUE);

        // Terminate Linked List/Stack
        void* aligned = AlignForward(obj, objectAlignment);
        *reinterpret_cast<void**>(aligned) = nullptr;
        last = aligned;
    }

    return memory;
#else
    LF_STATIC_CRASH("Missing implementation.");
    return nullptr;
#endif
}

static void* InitializeGlobalHeapCorruption(SizeT objectSize, SizeT objectAlignment, SizeT numObjects, void*& top, void*& last)
{
    top = nullptr;
    last = nullptr;

#if defined(LF_OS_WINDOWS)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
    // If the alignment is greater than page size for some reason, we cannot guarantee we'll allocate enough
    // memory to align properly.
    CriticalAssert(objectAlignment < PAGE_SIZE);

    const SizeT numObjectPages = CalculateNumPages((objectSize + objectAlignment) * numObjects, PAGE_SIZE);
    const SizeT totalPages = numObjectPages + 2;
    const SizeT memSize = totalPages * PAGE_SIZE;
    void* memory = VirtualAlloc(NULL, memSize, MEM_COMMIT, PAGE_READWRITE);

    void* globalProtectedHeader = memory;
    void* globalProtectedFooter = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(memory) + (totalPages - 1) * PAGE_SIZE);
    memset(globalProtectedHeader, 0xCC, PAGE_SIZE);
    memset(globalProtectedFooter, 0xCC, PAGE_SIZE);;
    DWORD oldProtect = 0;
    Assert(VirtualProtect(globalProtectedHeader, 1, PAGE_READONLY, &oldProtect) == TRUE);
    oldProtect = 0;
    Assert(VirtualProtect(globalProtectedFooter, 1, PAGE_READONLY, &oldProtect) == TRUE);

    void* base = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(memory) + PAGE_SIZE);
    void* alignedBase = AlignForward(base, objectAlignment);

    top = alignedBase;
    last = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(alignedBase) + objectSize * (numObjects - 1));

    void** obj = reinterpret_cast<void**>(alignedBase);
    for (SizeT i = 0; i < (numObjects - 1); ++i)
    {
        *obj = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(obj) + objectSize);
        obj = reinterpret_cast<void**>(*obj);
    }
    *obj = nullptr;
    return memory;
#else
    LF_STATIC_CRASH("Missing implementation.");
    return nullptr;
#endif
}


static void ReleaseLocalHeapCorruptionMemory(void* base, SizeT objectSize, SizeT objectAlignment, SizeT numObjects)
{
#if defined(LF_OS_WINDOWS)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);


    const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
    // If the alignment is greater than page size for some reason, we cannot guarantee we'll allocate enough
    // memory to align properly.
    CriticalAssert(objectAlignment < PAGE_SIZE);

    const SizeT numObjectPages = CalculateNumPages(objectSize + objectAlignment, PAGE_SIZE);
    const SizeT totalObjectPages = (numObjectPages + 1) * numObjects; // Each object has a protected header
    const SizeT totalPages = totalObjectPages + 1; // +1 for Global Protected Header.
    const SizeT memSize = totalPages * PAGE_SIZE;

    DWORD oldProtect;
    Assert(VirtualProtect(base, memSize, PAGE_READWRITE, &oldProtect) == TRUE);
    Assert(VirtualFree(base, 0, MEM_RELEASE) == TRUE);
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

static void ReleaseGlobalHeapCorruptionMemory(void* base, SizeT objectSize, SizeT objectAlignment, SizeT numObjects)
{
#if defined(LF_OS_WINDOWS)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);


    const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
    // If the alignment is greater than page size for some reason, we cannot guarantee we'll allocate enough
    // memory to align properly.
    CriticalAssert(objectAlignment < PAGE_SIZE);

    const SizeT numObjectPages = CalculateNumPages((objectSize + objectAlignment) * numObjects, PAGE_SIZE);
    const SizeT totalPages = numObjectPages + 2;
    const SizeT memSize = totalPages * PAGE_SIZE;

    DWORD oldProtect = 0;
    Assert(VirtualProtect(base, memSize, PAGE_READWRITE, &oldProtect) == TRUE);
    Assert(VirtualFree(base, 0, MEM_RELEASE) == TRUE);
#else
    LF_STATIC_CRASH("Missing implementation");
#endif
}

PoolHeap::PoolHeap()
: mTop(nullptr)
, mBase(nullptr)
, mLast(nullptr)
, mLock()
, mObjectSize(0)
, mObjectAlignment(0)
, mSize(0)
, mFlags(0)
#if defined(LF_MEMORY_DEBUG)
, mStatus(nullptr)
#endif
{

}
PoolHeap::PoolHeap(PoolHeap&& other)
: mTop(nullptr)
, mBase(other.mBase)
, mLast(other.mLast)
, mLock()
, mObjectSize(other.mObjectSize)
, mObjectAlignment(other.mObjectAlignment)
, mSize(other.mSize)
, mFlags(other.mFlags)
#if defined(LF_MEMORY_DEBUG)
, mStatus(other.mStatus)
#endif
{
    other.mLock.Acquire();
    mTop = AtomicLoad(&other.mTop);
    AtomicStore(&other.mTop, nullptr);
    other.mTop = nullptr;
    other.mBase = nullptr;
    other.mObjectAlignment = 0;
    other.mObjectSize = 0;
    other.mSize = 0;
    other.mFlags = 0;
#if defined(LF_MEMORY_DEBUG)
    other.mStatus = nullptr;
#endif
    other.mLock.Release();
}
PoolHeap::~PoolHeap()
{
    Release();
}

PoolHeap& PoolHeap::operator=(PoolHeap&& other)
{
    if (&other == this)
    {
        return *this;
    }
    mLock.Acquire();
    other.mLock.Acquire();

    mTop = AtomicLoad(&other.mTop);
    AtomicStore(&other.mTop, nullptr);

    mBase = other.mBase;
    mLast = other.mLast;
    mObjectSize = other.mObjectSize;
    mObjectAlignment = other.mObjectAlignment;
    mSize = other.mSize;
    mFlags = other.mFlags;
#if defined(LF_MEMORY_DEBUG)
    mStatus = other.mStatus;
#endif
    other.mBase = nullptr;
    other.mLast = nullptr;
    other.mObjectSize = 0;
    other.mObjectAlignment = 0;
    other.mSize = 0;
    other.mFlags = 0;
#if defined(LF_MEMORY_DEBUG)
    other.mStatus = nullptr;
#endif

    other.mLock.Release();
    mLock.Release();
    return *this;
}

bool PoolHeap::Initialize(SizeT objectSize, SizeT objectAlignment, SizeT numObjects, UInt32 flags)
{
    // const UInt8 flags = DOUBLE_FREE | LOCAL_HEAP_CORRUPTION | GLOBAL_HEAP_CORRUPTION;

    // Cannot allocate 0 memory.
    if (objectSize == 0 || objectAlignment == 0 || numObjects == 0)
    {
        return false;
    }

    // Must be greater than pointer size as that's what we store in the objects when they are 'free'
    if (objectSize < sizeof(void*))
    {
        return false;
    }

    // Object size must be a multiple of the alignment
    if ((objectSize % objectAlignment) != 0)
    {
        return false;
    }

    // Pool already initialized.
    if (mBase)
    {
        return false; 
    }

    // // VirtualAlloc(address, size, allocationType, protection) : void*
    // // VirtualFree(address, size, freeType)
    // void* vbase = VirtualAlloc(NULL, objectSize * (numObjects + 1), MEM_COMMIT, PAGE_READWRITE);
    // // void* vbaseAligned = 
    // UIntPtrT remainder = reinterpret_cast<UIntPtrT>(vbase) % objectAlignment;
    // void* vbaseAligned = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(vbase) + remainder);
    // ReportBugEx(reinterpret_cast<UIntPtrT>(vbaseAligned) % objectAlignment == 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    // VirtualFree(vbase, objectSize * (numObjects + 1), MEM_RELEASE);

    // if LOCAL_HEAP_CORRUPTION
    //   objectPages = CalculateNumPages( objectSize ) + 1; [ object_pages + footer_pages ]
    //   objectSpace = objectPages * PAGE_SIZE;
    //   memory = Allocate((objectSpace * numObjects) + 2 * PAGE_SIZE)
    // 
    // if GLOBAL_HEAP_COPRRUPTION
    //   objectSpace = CalculateNumPages(objectSize * numObjects);
    //   memory = Allocate(objectSpace + 2 * PAGE_SIZE);
    //

    bool enableLocalHeapCorruptionDetection = (flags & PHF_DETECT_LOCAL_HEAP_CORRUPTION) > 0;
    bool enableGlobalHeapCorruptionDetection = (flags & PHF_DETECT_GLOBAL_HEAP_CORRUPTION) > 0;

    void* top = nullptr;
    void* last = nullptr;
    void* memory = nullptr;

    if (enableLocalHeapCorruptionDetection)
    {
        memory = InitializeLocalHeapCorruption(objectSize, objectAlignment, numObjects, top, last);
    }
    else if (enableGlobalHeapCorruptionDetection)
    {
        memory = InitializeGlobalHeapCorruption(objectSize, objectAlignment, numObjects, top, last);
    }
    else
    {
        memory = InitializeNormal(objectSize, objectAlignment, numObjects);
        top = memory;
        last = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(memory) + objectSize * (numObjects - 1));
    }

#if defined(LF_MEMORY_DEBUG)
    mObjectBase = top;
    if ((flags & PHF_DOUBLE_FREE) > 0)
    {
        mStatus = reinterpret_cast<volatile Atomic16*>(LFAlloc(sizeof(UInt16) * numObjects, alignof(UInt16)));
        memset(const_cast<Atomic16*>(mStatus), 0, sizeof(UInt16) * numObjects);
    }
#endif
    mBase = memory;
    mTop = top;
    mLast = last;
    mObjectSize = objectSize;
    mObjectAlignment = objectAlignment;
    mSize = numObjects;
    mFlags = flags;

    // void* base = LFAlloc(objectSize * numObjects, objectAlignment);
    // if (base == nullptr)
    // {
    //     return false;
    // }
    // CriticalAssertEx(reinterpret_cast<UIntPtrT>(base) % objectAlignment == 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    // if (Initialize(base, objectSize * numObjects, objectSize, objectAlignment, numObjects))
    // {
    //     mOwnedMemory = true;
    //     return true;
    // }
    return true;
}

// bool PoolHeap::Initialize(void* memory, SizeT capacity, SizeT objectSize, SizeT objectAlignment, SizeT numObjects)
// {
// 
//     if (objectAlignment == 0) 
//     {
//         return false;
//     }
// 
//     if (numObjects == 0)
//     {
//         return false;
//     }
// 
//     if (objectSize < sizeof(void*) || (objectSize % objectAlignment) != 0)
//     {
//         return false;
//     }
// 
//     if (memory == nullptr || reinterpret_cast<UIntPtrT>(memory) % objectAlignment != 0)
//     {
//         return false;
//     }
// 
//     if (capacity < (objectSize * numObjects))
//     {
//         return false;
//     }
// 
//     if (mBase)
//     {
//         return false; // Pool already initialized.
//     }
// 
//     void** obj = reinterpret_cast<void**>(memory);
//     for (SizeT i = 0; i < (numObjects - 1); ++i)
//     {
//         *obj = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(obj) + objectSize);
//         obj = reinterpret_cast<void**>(*obj);
//     }
//     *obj = nullptr;
// 
// #if defined(LF_MEMORY_DEBUG)
//     mStatus = reinterpret_cast<volatile Atomic16*>(LFAlloc(sizeof(UInt16) * objectSize, alignof(UInt16)));
//     memset(const_cast<Atomic16*>(mStatus), 0, sizeof(UInt16) * objectSize);
// #endif
// 
//     mBase = memory;
//     mTop = memory;
//     mLast = reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(memory) + objectSize * (numObjects - 1));
//     mObjectSize = objectSize;
//     mObjectAlignment = objectAlignment;
//     mSize = numObjects;
//     return true;
// }

void PoolHeap::Release()
{
    if (mBase)
    {
        // If this happens then you are trying to release the pool while an allocation is occuring!
        CriticalAssertEx(mLock.TryAcquire(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        // LFFree(mBase);
        if ((mFlags & PHF_DETECT_LOCAL_HEAP_CORRUPTION) > 0)
        {
            ReleaseLocalHeapCorruptionMemory(mBase, mObjectSize, mObjectAlignment, mSize);
        }
        else if ((mFlags & PHF_DETECT_GLOBAL_HEAP_CORRUPTION) > 0)
        {
            ReleaseGlobalHeapCorruptionMemory(mBase, mObjectSize, mObjectAlignment, mSize);
        }
        else
        {
            LFFree(mBase);
        }
        AtomicStore(&mTop, nullptr);
        mBase = nullptr;
        mObjectSize = 0;
        mObjectAlignment = 0;
        mSize = 0;
#if defined(LF_MEMORY_DEBUG)
        if ((mFlags & PHF_DOUBLE_FREE) > 0)
        {
            LFFree(const_cast<Atomic16*>(mStatus));
            mStatus = nullptr;
        }
#endif
        mLock.Release();
    }
}

void* PoolHeap::Allocate()
{
    void* pointer = nullptr;
    {
        ScopeLock lock(mLock);
        mTop = AtomicLoad(&mTop);
        if (mTop == nullptr)
        {
            return nullptr;
        }

        pointer = mTop;
        AtomicStore(&mTop, *reinterpret_cast<void**>(mTop));
        AtomicIncrement32(&mAllocations);
    }

#if defined(LF_MEMORY_DEBUG)
    if ((mFlags & PHF_DOUBLE_FREE) > 0)
    {
        if ((mFlags & PHF_DETECT_LOCAL_HEAP_CORRUPTION) > 0)
        {
            MEMORY_BASIC_INFORMATION memInfo;
            CriticalAssert(VirtualQuery(pointer, &memInfo, sizeof(memInfo)) == sizeof(memInfo));

            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);

            const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
            const SizeT numObjectPages = CalculateNumPages(mObjectSize + mObjectAlignment, PAGE_SIZE);
            const SizeT pagedObjectSize = (numObjectPages + 1) * PAGE_SIZE;

            UIntPtrT objectBase = reinterpret_cast<UIntPtrT>(mBase) + PAGE_SIZE;
            SizeT index = (reinterpret_cast<UIntPtrT>(memInfo.BaseAddress) - objectBase) / pagedObjectSize;
            CriticalAssert(index < mSize);
            AtomicIncrement16(&mStatus[index]);
        }
        else if ((mFlags & PHF_DETECT_GLOBAL_HEAP_CORRUPTION) > 0)
        {
            SizeT index = (reinterpret_cast<UIntPtrT>(pointer) - reinterpret_cast<UIntPtrT>(mObjectBase)) / mObjectSize;
            CriticalAssert(index < mSize);
            AtomicIncrement16(&mStatus[index]);
        }
        else
        {
            SizeT index = (reinterpret_cast<UIntPtrT>(pointer) - reinterpret_cast<UIntPtrT>(mBase)) / mObjectSize;
            CriticalAssert(index < mSize);
            AtomicIncrement16(&mStatus[index]);
        }
    }

    // Write 0xBA 'bad memory' to the extra memory in the page thats not used by the object.
    // That block of memory should still read 0xBA when the object is freed, if not there was some
    // corruption somewhere.
    if ((mFlags & PHF_DETECT_LOCAL_HEAP_CORRUPTION) > 0)
    {
        MEMORY_BASIC_INFORMATION memInfo;
        CriticalAssert(VirtualQuery(pointer, &memInfo, sizeof(memInfo)) == sizeof(memInfo));

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
        const SizeT numObjectPages = CalculateNumPages(mObjectSize + mObjectAlignment, PAGE_SIZE);
        const SizeT pagedObjectSize = numObjectPages * PAGE_SIZE;

        memset(pointer, 0xBA, pagedObjectSize);
        memset(pointer, 0x00, mObjectSize);
    }
#endif

    return pointer;
}
void PoolHeap::Free(void* pointer)
{
    if (pointer == nullptr)
    {
        return;
    }

    UIntPtrT address = reinterpret_cast<UIntPtrT>(pointer);
    if(address < reinterpret_cast<UIntPtrT>(mBase) || address > reinterpret_cast<UIntPtrT>(mLast))
    {
        ReportBugMsgEx("Attempting to free a pointer that does not belong to this pool heap.", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
        return;
    }

#if defined(LF_MEMORY_DEBUG)
    if ((mFlags & PHF_DOUBLE_FREE) > 0)
    {
        if ((mFlags & PHF_DETECT_LOCAL_HEAP_CORRUPTION) > 0)
        {
            MEMORY_BASIC_INFORMATION memInfo;
            CriticalAssert(VirtualQuery(pointer, &memInfo, sizeof(memInfo)) == sizeof(memInfo));

            SYSTEM_INFO sysInfo;
            GetSystemInfo(&sysInfo);

            const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
            const SizeT numObjectPages = CalculateNumPages(mObjectSize + mObjectAlignment, PAGE_SIZE);
            const SizeT pagedObjectSize = (numObjectPages + 1) * PAGE_SIZE;

            UIntPtrT objectBase = reinterpret_cast<UIntPtrT>(mBase) + PAGE_SIZE;
            SizeT index = (reinterpret_cast<UIntPtrT>(memInfo.BaseAddress) - objectBase) / pagedObjectSize;
            CriticalAssert(index < mSize);
            AssertEx(AtomicDecrement16(&mStatus[index]) >= 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        }
        else if ((mFlags & PHF_DETECT_GLOBAL_HEAP_CORRUPTION) > 0)
        {
            SizeT index = (reinterpret_cast<UIntPtrT>(pointer) - reinterpret_cast<UIntPtrT>(mObjectBase)) / mObjectSize;
            CriticalAssert(index < mSize);
            AssertEx(AtomicDecrement16(&mStatus[index]) >= 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        }
        else
        {
            SizeT index = (reinterpret_cast<UIntPtrT>(pointer) - reinterpret_cast<UIntPtrT>(mBase)) / mObjectSize;
            CriticalAssert(index < mSize);
            AssertEx(AtomicDecrement16(&mStatus[index]) >= 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        }
    }

    if ((mFlags & PHF_DETECT_LOCAL_HEAP_CORRUPTION) > 0)
    {
        MEMORY_BASIC_INFORMATION memInfo;
        CriticalAssert(VirtualQuery(pointer, &memInfo, sizeof(memInfo)) == sizeof(memInfo));

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        const SizeT PAGE_SIZE = static_cast<SizeT>(sysInfo.dwPageSize);
        const SizeT numObjectPages = CalculateNumPages(mObjectSize + mObjectAlignment, PAGE_SIZE);
        const SizeT pagedObjectSize = numObjectPages * PAGE_SIZE;
        
        const ByteT* pagedMemory = reinterpret_cast<const ByteT*>(reinterpret_cast<UIntPtrT>(pointer) + mObjectSize);
        const SizeT pagedMemorySize = pagedObjectSize - mObjectSize;
        for(SizeT i = 0; i < pagedMemorySize; ++i)
        {
            AssertEx(pagedMemory[i] == 0xBA, LF_ERROR_MEMORY_CORRUPTION, ERROR_API_CORE);
        }
    }
#endif

    {
        ScopeLock lock(mLock);
        *reinterpret_cast<void**>(pointer) = mTop;
        mTop = pointer;
        AtomicDecrement32(&mAllocations);
    }
}

bool PoolHeap::IsOwnerOf(const void* pointer)
{
    UIntPtrT address = reinterpret_cast<UIntPtrT>(pointer);
    if (address < reinterpret_cast<UIntPtrT>(mBase) || address > reinterpret_cast<UIntPtrT>(mLast))
    {
        return false;
    }
    return true;
}

bool PoolHeap::IsOutOfMemory() const
{
    return AtomicLoad(&mTop) == nullptr;
}

SizeT PoolHeap::GetAllocations() const
{
    return static_cast<SizeT>(AtomicLoad(&mAllocations));
}

}
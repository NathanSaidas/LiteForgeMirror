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
#include "Memory.h"
#include <memory>

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <intrin.h>
#endif

// Enable to output allocation/free for use with /Tools/LogAnaylzer <filename>
// #define LF_TRACK_ALLOCS

namespace lf {

PointerNode gNullPointerNode;
AtomicPointerNode gNullAtomicPointerNode;
const NullPtr NULL_PTR(&gNullPointerNode);
const AtomicNullPtr ATOMIC_NULL_PTR(&gNullAtomicPointerNode);
#if defined(LF_TRACK_ALLOCS)
Atomic32 gTrackAllocs = 0;
#endif

struct MemoryHeader
{
    SizeT size;
    SizeT alignment;
    UInt8 tag;
    UInt8 headerSize;
};

#if defined(LF_TRACK_ALLOCS)
LF_FORCE_INLINE static bool TrackAlloc()
{
    return _InterlockedCompareExchange(&gTrackAllocs, 0, 0) > 0;
}
#endif

LF_FORCE_INLINE static void* PtrAdd(void* pointer, SizeT amount)
{
    return reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(pointer) + amount);
}
LF_FORCE_INLINE static void* PtrSub(void* pointer, SizeT amount)
{
    return reinterpret_cast<void*>(reinterpret_cast<UIntPtrT>(pointer) - amount);
}

MemoryMarkup::MemoryMarkup() : 
mBytesAllocated(0),
mAllocs(0)
{

}

MemoryMarkup MEMORY_MARK_UP[MMT_MAX_VALUE];
const char*  MEMORY_MARK_UP_STRING[MMT_MAX_VALUE];

LF_THREAD_LOCAL MemoryMarkupTag gCurrentMarkupTag = MMT_GENERAL;

MemoryMarkupTag LFGetCurrentMemoryTag()
{
    return gCurrentMarkupTag;
}
void LFSetCurrentMemoryTag(MemoryMarkupTag tag)
{
    gCurrentMarkupTag = tag;
}


void* LFAlloc(SizeT size, SizeT alignment)
{
    if (size == 0)
    {
        return nullptr;
    }
    size = size < alignment ? alignment : size;
#if defined(LF_TRACK_ALLOCS)
    if (TrackAlloc())
    {
        UIntPtrT callingAddress = reinterpret_cast<UIntPtrT>(_ReturnAddress());
        char buffer[64];
        sprintf_s(buffer, sizeof(buffer), "[DEBUG:Allocate] %llu %llu\n", size, callingAddress);
        OutputDebugString(buffer);
    }
#endif
    const SizeT actualHeaderSize = sizeof(MemoryHeader) + ((size + sizeof(MemoryHeader)) % alignment);
    const SizeT sizeWithHeader = size + actualHeaderSize;
    void* pointer = _aligned_malloc(sizeWithHeader, alignment);
    if (!pointer)
    {
        return nullptr;
    }
    MemoryHeader* header = static_cast<MemoryHeader*>(pointer);
    header->size = size;
    header->alignment = alignment;
    header->tag = static_cast<UInt8>(LFGetCurrentMemoryTag());
    header->headerSize = static_cast<ByteT>(actualHeaderSize);

    ByteT* headerSize = static_cast<ByteT*>(PtrAdd(pointer, actualHeaderSize - 1));
    *headerSize = static_cast<ByteT>(actualHeaderSize);
    _InlineInterlockedAdd64(&MEMORY_MARK_UP[LFGetCurrentMemoryTag()].mBytesAllocated, static_cast<long long>(size));
    _InterlockedIncrement64(&MEMORY_MARK_UP[LFGetCurrentMemoryTag()].mAllocs);
    return PtrAdd(pointer, actualHeaderSize);
}
void LFFree(void* pointer)
{
    ByteT* headerSize = static_cast<ByteT*>(PtrSub(pointer, 1));
    MemoryHeader* header = static_cast<MemoryHeader*>(PtrSub(pointer, static_cast<SizeT>(*headerSize)));
#if defined(LF_TRACK_ALLOCS)
    if (TrackAlloc())
    {
        UIntPtrT callingAddress = reinterpret_cast<UIntPtrT>(_ReturnAddress());
        char buffer[64];
        sprintf_s(buffer, sizeof(buffer), "[DEBUG:Free] %llu %llu\n", header->size, callingAddress);
        OutputDebugString(buffer);
    }
#endif
    _InlineInterlockedAdd64(&MEMORY_MARK_UP[header->tag].mBytesAllocated, -static_cast<long long>(header->size));
    _InterlockedDecrement64(&MEMORY_MARK_UP[header->tag].mAllocs);
    _aligned_free(header);
}

SizeT LFGetBytesAllocated()
{
    SizeT result = 0;
    for (SizeT i = 0; i < LF_ARRAY_SIZE(MEMORY_MARK_UP); ++i)
    {
        result += static_cast<SizeT>(MEMORY_MARK_UP[i].mBytesAllocated);
    }
    return result;
}
SizeT LFGetAllocations()
{
    SizeT result = 0;
    for (SizeT i = 0; i < LF_ARRAY_SIZE(MEMORY_MARK_UP); ++i)
    {
        result += static_cast<SizeT>(MEMORY_MARK_UP[i].mAllocs);
    }
    return result;
}

void LFEnterTrackAllocs()
{
#if defined(LF_TRACK_ALLOCS)
    _InterlockedIncrement(&gTrackAllocs);
#endif
}
void LFExitTrackAllocs()
{
#if defined(LF_TRACK_ALLOCS)
    _InterlockedDecrement(&gTrackAllocs);
#endif
}

} // namespace lf
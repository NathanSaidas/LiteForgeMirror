// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "Memory.h"
#include <memory>

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

// Enable to output allocation/free for use with /Tools/LogAnaylzer <filename>
// #define LF_TRACK_ALLOCS

namespace lf {

PointerNode gNullPointerNode;
AtomicPointerNode gNullAtomicPointerNode;
const NullPtr NULL_PTR(&gNullPointerNode);
const AtomicNullPtr ATOMIC_NULL_PTR(&gNullAtomicPointerNode);

struct MemoryHeader
{
    SizeT size;
    SizeT alignment;
    UInt8 tag;
    UInt8 headerSize;
};

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

void* LFAlloc(SizeT size, SizeT alignment, MemoryMarkupTag tag)
{
    if (size == 0)
    {
        return nullptr;
    }
    size = size < alignment ? alignment : size;
#if defined(LF_TRACK_ALLOCS)
    UIntPtrT callingAddress = reinterpret_cast<UIntPtrT>(_ReturnAddress());
    char buffer[64];
    sprintf_s(buffer, sizeof(buffer), "[DEBUG:Allocate] %llu %llu\n", size, callingAddress);
    OutputDebugString(buffer);
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
    header->tag = static_cast<UInt8>(tag);
    header->headerSize = static_cast<ByteT>(actualHeaderSize);

    ByteT* headerSize = static_cast<ByteT*>(PtrAdd(pointer, actualHeaderSize - 1));
    *headerSize = static_cast<ByteT>(actualHeaderSize);
    _InterlockedAdd64(&MEMORY_MARK_UP[tag].mBytesAllocated, static_cast<long long>(size));
    _InterlockedIncrement64(&MEMORY_MARK_UP[tag].mAllocs);
    return PtrAdd(pointer, actualHeaderSize);
}
void LFFree(void* pointer)
{
    ByteT* headerSize = static_cast<ByteT*>(PtrSub(pointer, 1));
    MemoryHeader* header = static_cast<MemoryHeader*>(PtrSub(pointer, static_cast<SizeT>(*headerSize)));
#if defined(LF_TRACK_ALLOCS)
    UIntPtrT callingAddress = reinterpret_cast<UIntPtrT>(_ReturnAddress());
    char buffer[64];
    sprintf_s(buffer, sizeof(buffer), "[DEBUG:Free] %llu %llu\n", header->size, callingAddress);
    OutputDebugString(buffer);
#endif
    _InterlockedAdd64(&MEMORY_MARK_UP[header->tag].mBytesAllocated, -static_cast<long long>(header->size));
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

} // namespace lf
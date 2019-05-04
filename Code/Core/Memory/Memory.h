// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_MEMORY_H
#define LF_CORE_MEMORY_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include <new>

namespace lf {

enum MemoryMarkupTag
{
    MMT_GENERAL,
    MMT_MAX_VALUE,
    MMT_POINTER_NODE
};
struct MemoryMarkup
{
    MemoryMarkup();
    volatile long long   mBytesAllocated;
    volatile long long   mAllocs;
};

LF_CORE_API extern MemoryMarkup MEMORY_MARK_UP[MMT_MAX_VALUE];
LF_CORE_API extern const char*  MEMORY_MARK_UP_STRING[MMT_MAX_VALUE];

LF_CORE_API void* LFAlloc(SizeT size, SizeT alignment, MemoryMarkupTag tag = MMT_GENERAL);
LF_CORE_API void  LFFree(void* pointer);
LF_CORE_API SizeT LFGetBytesAllocated();
LF_CORE_API SizeT LFGetAllocations();

template<typename T>
T* LFNew(MemoryMarkupTag tag = MMT_GENERAL)
{
    void* pointer = LFAlloc(sizeof(T), alignof(T), tag);
    if (!pointer)
    {
        return nullptr;
    }
    return new(pointer)T();
}
template<typename T>
void LFDelete(T* pointer)
{
    pointer->~T();
    LFFree(pointer);
}

struct DefaultAllocator
{
    static void* Allocate(SizeT bytes, SizeT alignment)
    {
        return LFAlloc(bytes, alignment);
    }
    static void Free(void* pointer)
    {
        LFFree(pointer);
    }
};

struct PointerNode
{
    PointerNode() :
        mPointer(nullptr),
        mStrong(1),
        mWeak(0)
    {}
    void* mPointer;
    Int32 mStrong;
    Int32 mWeak;
};
LF_CORE_API extern PointerNode gNullPointerNode;

struct NullPtr
{
    NullPtr() : mNode(nullptr) {}
    NullPtr(PointerNode* node) : mNode(node) {}
private:
    PointerNode* mNode;
};
LF_CORE_API extern const NullPtr NULL_PTR;

struct AtomicPointerNode
{
    AtomicPointerNode() :
        mPointer(nullptr),
        mStrong(1),
        mWeak(0)
    {}
    volatile void* mPointer;
    volatile Atomic32 mStrong;
    volatile Atomic32 mWeak;
};
LF_CORE_API extern AtomicPointerNode gNullAtomicPointerNode;

struct AtomicNullPtr
{
    AtomicNullPtr() : mNode(nullptr) {}
    AtomicNullPtr(AtomicPointerNode* node) : mNode(node) {}
private:
    AtomicPointerNode* mNode;
};
LF_CORE_API extern const AtomicNullPtr ATOMIC_NULL_PTR;

template<typename Dest, typename Src>
const Dest& StaticCast(const Src& src)
{
    // If you got this error your types are not related.
    LF_STATIC_IS_A(typename Dest::ValueType, typename Src::ValueType);
    return *reinterpret_cast<const Dest*>(&src);
}


} // namespace lf

#endif // LF_CORE_MEMORY_H
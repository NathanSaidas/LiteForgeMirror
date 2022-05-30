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
#include <new>

namespace lf {

enum MemoryMarkupTag
{
    MMT_GENERAL,
    MMT_POINTER_NODE,
    MMT_GRAPHICS,
    MMT_MAX_VALUE
};
struct MemoryMarkup
{
    MemoryMarkup();
    volatile long long   mBytesAllocated;
    volatile long long   mAllocs;
};

LF_CORE_API MemoryMarkupTag LFGetCurrentMemoryTag();
LF_CORE_API void LFSetCurrentMemoryTag(MemoryMarkupTag tag);

struct ScopedMemoryTag
{
    LF_INLINE ScopedMemoryTag() : mPreviousTag(LFGetCurrentMemoryTag())
    {
        LFSetCurrentMemoryTag(MemoryMarkupTag::MMT_GENERAL);
    }
    LF_INLINE ScopedMemoryTag(MemoryMarkupTag value) : mPreviousTag(LFGetCurrentMemoryTag())
    {
        LFSetCurrentMemoryTag(value);
    }
    ~ScopedMemoryTag()
    {
        LFSetCurrentMemoryTag(mPreviousTag);
    }

    MemoryMarkupTag mPreviousTag;
};

#define LF_SCOPED_MEMORY(tag_) ::lf::ScopedMemoryTag LF_ANONYMOUS_NAME(scopedMemoryTag)(tag_);

LF_CORE_API extern MemoryMarkup MEMORY_MARK_UP[MMT_MAX_VALUE];
LF_CORE_API extern const char*  MEMORY_MARK_UP_STRING[MMT_MAX_VALUE];

LF_CORE_API void* LFAlloc(SizeT size, SizeT alignment);
LF_CORE_API void  LFFree(void* pointer);
LF_CORE_API SizeT LFGetBytesAllocated();
LF_CORE_API SizeT LFGetAllocations();
LF_CORE_API void  LFEnterTrackAllocs();
LF_CORE_API void  LFExitTrackAllocs();

template<typename T, typename... ARGS>
T* LFNew(ARGS... constructorArgs)
{
    void* pointer = LFAlloc(sizeof(T), alignof(T));
    if (!pointer)
    {
        return nullptr;
    }
    return new(pointer)T(constructorArgs...);
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

struct PointerConvertibleType
{

};

} // namespace lf
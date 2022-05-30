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
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/Platform/Atomic.h"

#define DECLARE_ATOMIC_PTR(T) class T; using T##AtomicPtr = ::lf::TAtomicStrongPointer<T>
#define DECLARE_ATOMIC_WPTR(T) class T; using T##AtomicWPtr = ::lf::TAtomicWeakPointer<T>

#define DECLARE_STRUCT_ATOMIC_PTR(T) struct T; using T##AtomicPtr = ::lf::TAtomicStrongPointer<T>;
#define DECLARE_STRUCT_ATOMIC_WPTR(T) struct T; using T##AtomicWPtr = ::lf::TAtomicWeakPoitner<T>;

namespace lf {

template<typename T>
struct TAtomicPointerNode
{
    T* volatile mPointer;
    Atomic32 mStrong;
    Atomic32 mWeak;
};

template<typename T> class TAtomicStrongPointer;
template<typename T> class TAtomicWeakPointer;

// **********************************
// An implementation of TStrongPointer but using atomic operations to be read-only thread-safe
// -- Modifying the internal pointer state (create/destroy) is still not thread-safe
// -- Use this when a pointer transfers ownership across multiple threads.
// **********************************
template<typename T>
class TAtomicStrongPointer
{
    friend TAtomicWeakPointer<T>;
    using NodeType = TAtomicPointerNode<T>;
public:
    using ValueType = T;
    using Pointer = T*;
    using StrongType = TAtomicStrongPointer<T>;
    using WeakType = TAtomicWeakPointer<T>;

    TAtomicStrongPointer();
    TAtomicStrongPointer(const StrongType& other);
    TAtomicStrongPointer(StrongType&& other);
    TAtomicStrongPointer(const NullPtr&);
    explicit TAtomicStrongPointer(Pointer memory);
    TAtomicStrongPointer(const WeakType& other);
    ~TAtomicStrongPointer();

    template<typename U>
    TAtomicStrongPointer(const TAtomicStrongPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TAtomicStrongPointer<T>* otherPtr = reinterpret_cast<const TAtomicStrongPointer<T>*>(&other);
        AtomicStorePointer(&mNode, AtomicLoadPointer(&otherPtr->mNode));
        IncrementRef();
    }

    template<typename U>
    TAtomicStrongPointer(const TAtomicWeakPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TAtomicWeakPointer<T>* otherPtr = reinterpret_cast<const TAtomicWeakPointer<T>*>(&other);
        AtomicStorePointer(&mNode, AtomicLoadPointer(&otherPtr->mNode));
        IncrementRef();
    }

    StrongType& operator=(const StrongType& other);
    StrongType& operator=(const WeakType& other);
    StrongType& operator=(const NullPtr&);
    StrongType& operator=(StrongType&& other);

    bool operator==(const StrongType& other) const;
    bool operator==(const WeakType& other) const;
    bool operator==(const NullPtr&) const;
    bool operator!=(const StrongType& other) const;
    bool operator!=(const WeakType& other) const;
    bool operator!=(const NullPtr&) const;

    operator bool() const;
    operator Pointer() { return AtomicLoadPointer(&mNode->mPointer); }
    operator const Pointer() const { return AtomicLoadPointer(&mNode->mPointer); }

    Pointer AsPtr() { return AtomicLoadPointer(&mNode->mPointer); }
    const Pointer AsPtr() const { return AtomicLoadPointer(&mNode->mPointer); }

    Pointer operator->();
    const Pointer operator->() const;

    ValueType& operator*();
    const ValueType& operator*() const;

    void Release();
    SizeT GetWeakRefs() const;
    SizeT GetStrongRefs() const;

private:
    void AllocNode()
    {
        mNode = AtomicLoadPointer(&mNode);
        Assert(!mNode);
        LF_SCOPED_MEMORY(MMT_POINTER_NODE);
        AtomicStorePointer(&mNode, static_cast<NodeType*>(LFAlloc(sizeof(NodeType), alignof(NodeType))));
        mNode->mStrong = 1;
        mNode->mWeak = 0;
        mNode->mPointer = nullptr;
    }
    void ReleaseNode()
    {
        mNode = AtomicLoadPointer(&mNode);
        Assert(mNode);
        Assert(!AtomicLoadPointer(&mNode->mPointer));
        LFFree(const_cast<NodeType*>(mNode));
        AtomicStorePointer(&mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    }
    void DestroyPointer()
    {
        mNode = AtomicLoadPointer(&mNode);
        Assert(mNode);
        Pointer temp = AtomicLoadPointer(&mNode->mPointer);
        if (temp)
        {
            AtomicStorePointer(&mNode->mPointer, Pointer(nullptr));
            temp->~T();
            LFFree(temp);
        }
    }
    void IncrementRef()
    {
        if (AtomicLoadPointer(&mNode))
        {
            AtomicIncrement32(&mNode->mStrong);
        }
    }
    void DecrementRef()
    {
        if (AtomicLoadPointer(&mNode))
        {
            Atomic32 result = AtomicDecrement32(&mNode->mStrong);
            if (result == 0)
            {
                AtomicIncrement32(&mNode->mWeak);
                DestroyPointer();
                AtomicDecrement32(&mNode->mWeak);
            }
            AtomicRWBarrier();
            if (result == 0)
            {
                if (AtomicLoad(&mNode->mWeak) == 0 && AtomicLoad(&mNode->mStrong) == 0)
                {
                    ReleaseNode();
                }
            }
            NodeType* null = nullptr;
            AtomicStorePointer(&mNode, null);
        }
    }

    NodeType* volatile mNode;
};

// **********************************
// An implementation of TWeakPointer but using atomic operations to be read-only thread-safe
// -- Modifying the internal pointer state (create/destroy) is still not thread-safe
// -- Use this when a pointer transfers ownership across multiple threads.
// **********************************
template<typename T>
class TAtomicWeakPointer
{
    friend TAtomicStrongPointer<T>;
    using NodeType = TAtomicPointerNode<T>;
public:
    using ValueType = T;
    using Pointer = T*;
    using StrongType = TAtomicStrongPointer<T>;
    using WeakType = TAtomicWeakPointer<T>;

    TAtomicWeakPointer();
    TAtomicWeakPointer(const WeakType& other);
    TAtomicWeakPointer(WeakType&& other);
    TAtomicWeakPointer(const NullPtr&);
    TAtomicWeakPointer(const StrongType& other);
    ~TAtomicWeakPointer();

    template<typename U>
    TAtomicWeakPointer(const TAtomicStrongPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TAtomicStrongPointer<T>* otherPtr = reinterpret_cast<const TAtomicStrongPointer<T>*>(&other);
        AtomicStorePointer(&mNode, otherPtr->mNode);
        IncrementRef();
    }

    template<typename U>
    TAtomicWeakPointer(const TAtomicWeakPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TAtomicWeakPointer<T>* otherPtr = reinterpret_cast<const TAtomicWeakPointer<T>*>(&other);
        AtomicStorePointer(&mNode, otherPtr->mNode);
        IncrementRef();
    }


    WeakType& operator=(const StrongType& other);
    WeakType& operator=(const WeakType& other);
    WeakType& operator=(const NullPtr&);
    WeakType& operator=(WeakType&& other);


    bool operator==(const StrongType& other) const;
    bool operator==(const WeakType& other) const;
    bool operator==(const NullPtr&) const;
    bool operator!=(const StrongType& other) const;
    bool operator!=(const WeakType& other) const;
    bool operator!=(const NullPtr&) const;

    operator bool() const;
    operator Pointer() { return AtomicLoadPointer(&mNode->mPointer); }
    operator const Pointer() const { return AtomicLoadPointer(&mNode->mPointer); }

    Pointer operator->();
    const Pointer operator->() const;

    ValueType& operator*();
    const ValueType& operator*() const;

    void Release();
    SizeT GetWeakRefs() const;
    SizeT GetStrongRefs() const;

private:
    void ReleaseNode()
    {
        mNode = AtomicLoadPointer(&mNode);
        Assert(mNode);
        Assert(!mNode->mPointer);
        LFFree(mNode);
        AtomicStorePointer(&mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    }
    void IncrementRef()
    {
        if (AtomicLoadPointer(&mNode))
        {
            AtomicIncrement32(&mNode->mWeak);
        }
    }
    void DecrementRef()
    {
        if (AtomicLoadPointer(&mNode))
        {
            if (AtomicDecrement32(&mNode->mWeak) == 0)
            {
                if (AtomicLoad(&mNode->mStrong) == 0)
                {
                    ReleaseNode();
                }
            }
            NodeType* null = nullptr;
            AtomicStorePointer(&mNode, null);
        }
    }

    NodeType* volatile mNode;
};

// Convertible Smart Atomic Pointers
// usage:
// class MyType : public TAtomicWeakPointerConvertible
//
// public: 
//      // ### Use this typedef to automatically create a convertible weak pointer when using ReflectionMgr
//      using PointerConvertible = PointerConvertibleType;
// 
// ptr = MakeConvertibleAtomicPtr<MyType>();
// wptr = GetAtomicPointer(rawPtr);
//

template<typename T>
struct TAtomicWeakPointerConvertible
{
public:
    const TAtomicWeakPointer<T>& GetWeakPointer() const { return mPointer; }
    TAtomicWeakPointer<T>& GetWeakPointer() { return mPointer; }
private:
    TAtomicWeakPointer<T> mPointer;
};

template<typename T>
TAtomicStrongPointer<T> MakeConvertibleAtomicPtr()
{
    TAtomicStrongPointer<T> ptr(LFNew<T>());
    ptr->GetWeakPointer() = ptr;
    return ptr;
}

template<typename T, typename ... ARGS>
TAtomicStrongPointer<T> MakeConvertibleAtomicPtr(ARGS&&... args)
{
    TAtomicStrongPointer<T> ptr(LFNew<T>(std::forward<ARGS>(args)...));
    ptr->GetWeakPointer() = ptr;
    return ptr;
}

template<typename T>
TAtomicWeakPointer<T> GetAtomicPointer(T* self)
{
    if (!self)
    {
        return NULL_PTR;
    }
    return StaticCast<TAtomicWeakPointer<T>>(self->GetWeakPointer());
}

template<typename T>
const TAtomicWeakPointer<T>& GetAtomicPointer(const T* self)
{
    if (!self)
    {
        return *reinterpret_cast<const TAtomicWeakPointer<T>*>(&NULL_PTR);
    }
    return StaticCast<TAtomicWeakPointer<T>>(self->GetWeakPointer());
}

template<typename T>
TAtomicStrongPointer<T>::TAtomicStrongPointer() : mNode(reinterpret_cast<NodeType*>(&gNullAtomicPointerNode))
{
    IncrementRef();
}
template<typename T>
TAtomicStrongPointer<T>::TAtomicStrongPointer(const StrongType& other) : mNode(other.mNode)
{
    IncrementRef();
}
template<typename T>
TAtomicStrongPointer<T>::TAtomicStrongPointer(StrongType&& other) : mNode(other.mNode)
{
    AtomicStorePointer(&other.mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    other.IncrementRef();
}
template<typename T>
TAtomicStrongPointer<T>::TAtomicStrongPointer(const NullPtr&) : mNode(reinterpret_cast<NodeType*>(&gNullAtomicPointerNode))
{
    IncrementRef();
}
template<typename T>
TAtomicStrongPointer<T>::TAtomicStrongPointer(Pointer memory) : mNode(reinterpret_cast<NodeType*>(&gNullAtomicPointerNode))
{
    if (memory)
    {
        NodeType* null = nullptr;
        AtomicStorePointer(&mNode, null);
        AllocNode();
        AtomicStorePointer(&mNode->mPointer, memory);
    }
    else
    {
        IncrementRef();
    }
}
template<typename T>
TAtomicStrongPointer<T>::TAtomicStrongPointer(const WeakType& other) : mNode(other.mNode)
{
    IncrementRef();
}

template<typename T>
TAtomicStrongPointer<T>::~TAtomicStrongPointer()
{
    if (AtomicLoadPointer(&mNode))
    {
        Atomic32 result = AtomicDecrement32(&mNode->mStrong);
        if (result == 0)
        {
            AtomicIncrement32(&mNode->mWeak);
            DestroyPointer();
            AtomicDecrement32(&mNode->mWeak);
        }
        AtomicRWBarrier();
        if (result == 0)
        {
            if (AtomicLoad(&mNode->mWeak) == 0 && AtomicLoad(&mNode->mStrong) == 0)
            {
                ReleaseNode();
            }
        }
        NodeType* null = nullptr;
        AtomicStorePointer(&mNode, null);
    }
}

template<typename T>
typename TAtomicStrongPointer<T>::StrongType& TAtomicStrongPointer<T>::operator=(const StrongType& other)
{
    if (this == &other)
    {
        return *this;
    }
    DecrementRef();
    AtomicStorePointer(&mNode, other.mNode);
    IncrementRef();
    return *this;
}
template<typename T>
typename TAtomicStrongPointer<T>::StrongType& TAtomicStrongPointer<T>::operator=(const WeakType& other)
{
    if (mNode == other.mNode)
    {
        return *this;
    }
    DecrementRef();
    AtomicStorePointer(&mNode, other.mNode);
    IncrementRef();
    return *this;
}
template<typename T>
typename TAtomicStrongPointer<T>::StrongType& TAtomicStrongPointer<T>::operator=(const NullPtr&)
{
    DecrementRef();
    AtomicStorePointer(&mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    AtomicIncrement32(&mNode->mStrong);
    return *this;
}
template<typename T>
typename TAtomicStrongPointer<T>::StrongType& TAtomicStrongPointer<T>::operator=(StrongType&& other)
{
    if (this == &other)
    {
        return *this;
    }
    DecrementRef();
    AtomicStorePointer(&mNode, other.mNode);
    AtomicStorePointer(&other.mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    other.IncrementRef();
    return *this;
}

template<typename T>
bool TAtomicStrongPointer<T>::operator==(const StrongType& other) const
{
    return AtomicLoadPointer(&other.mNode->mPointer) == AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator==(const WeakType& other) const
{
    return AtomicLoadPointer(&other.mNode->mPointer) == AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator==(const NullPtr&) const
{
    return AtomicLoadPointer(&mNode->mPointer) == nullptr;
}
template<typename T>
bool TAtomicStrongPointer<T>::operator!=(const StrongType& other) const
{
    return AtomicLoadPointer(&other.mNode->mPointer) != AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator!=(const WeakType& other) const
{
    return AtomicLoadPointer(&other.mNode->mPointer) != AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator!=(const NullPtr&) const
{
    return AtomicLoadPointer(&mNode->mPointer) != nullptr;
}

template<typename T>
TAtomicStrongPointer<T>::operator bool() const
{
    return AtomicLoadPointer(&mNode->mPointer) != nullptr;
}

template<typename T>
typename TAtomicStrongPointer<T>::Pointer TAtomicStrongPointer<T>::operator->()
{
    return AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
const typename TAtomicStrongPointer<T>::Pointer TAtomicStrongPointer<T>::operator->() const
{
    return AtomicLoadPointer(&mNode->mPointer);
}

template<typename T>
typename TAtomicStrongPointer<T>::ValueType& TAtomicStrongPointer<T>::operator*()
{
    return *AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
const typename TAtomicStrongPointer<T>::ValueType& TAtomicStrongPointer<T>::operator*() const
{
    return *AtomicLoadPointer(&mNode->mPointer);
}

template<typename T>
void TAtomicStrongPointer<T>::Release()
{
    DecrementRef();
    AtomicStorePointer(&mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    AtomicIncrement32(&mNode->mStrong);
}
template<typename T>
SizeT TAtomicStrongPointer<T>::GetWeakRefs() const
{
    return AtomicLoad(&mNode->mWeak);
}
template<typename T>
SizeT TAtomicStrongPointer<T>::GetStrongRefs() const
{
    return AtomicLoad(&mNode->mStrong);
}

template<typename T>
TAtomicWeakPointer<T>::TAtomicWeakPointer() : mNode(reinterpret_cast<NodeType*>(&gNullAtomicPointerNode))
{
    IncrementRef();
}
template<typename T>
TAtomicWeakPointer<T>::TAtomicWeakPointer(const WeakType& other) : mNode(other.mNode)
{
    IncrementRef();
}
template<typename T>
TAtomicWeakPointer<T>::TAtomicWeakPointer(WeakType&& other) : mNode(other.mNode)
{
    AtomicStorePointer(&other.mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    other.IncrementRef();
}
template<typename T>
TAtomicWeakPointer<T>::TAtomicWeakPointer(const NullPtr&) : mNode(reinterpret_cast<NodeType*>(&gNullAtomicPointerNode))
{
    IncrementRef();
}
template<typename T>
TAtomicWeakPointer<T>::TAtomicWeakPointer(const StrongType& other) : mNode(other.mNode)
{
    IncrementRef();
}
template<typename T>
TAtomicWeakPointer<T>::~TAtomicWeakPointer()
{
    if (AtomicLoadPointer(&mNode))
    {
        if (AtomicDecrement32(&mNode->mWeak) == 0)
        {
            if (AtomicLoad(&mNode->mStrong) == 0)
            {
                ReleaseNode();
            }
        }
        NodeType* null = nullptr;
        AtomicStorePointer(&mNode, null);
    }
}


template<typename T>
typename TAtomicWeakPointer<T>::WeakType& TAtomicWeakPointer<T>::operator=(const StrongType& other)
{
    DecrementRef();
    AtomicStorePointer(&mNode, other.mNode);
    IncrementRef();
    return *this;
}
template<typename T>
typename TAtomicWeakPointer<T>::WeakType& TAtomicWeakPointer<T>::operator=(const WeakType& other)
{
    DecrementRef();
    AtomicStorePointer(&mNode, other.mNode);
    IncrementRef();
    return *this;
}
template<typename T>
typename TAtomicWeakPointer<T>::WeakType& TAtomicWeakPointer<T>::operator=(const NullPtr&)
{
    DecrementRef();
    AtomicStorePointer(&mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    AtomicIncrement32(&mNode->mWeak);
    return *this;
}
template<typename T>
typename TAtomicWeakPointer<T>::WeakType& TAtomicWeakPointer<T>::operator=(WeakType&& other)
{
    if (this == &other)
    {
        return *this;
    }
    DecrementRef();
    AtomicStorePointer(&mNode, other.mNode);
    AtomicStorePointer(&other.mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    other.IncrementRef();
    return *this;
}


template<typename T>
bool TAtomicWeakPointer<T>::operator==(const StrongType& other) const
{
    return AtomicLoadPointer(&mNode->mPointer) == AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator==(const WeakType& other) const
{
    return AtomicLoadPointer(&mNode->mPointer) == AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator==(const NullPtr&) const
{
    return AtomicLoadPointer(&mNode->mPointer) == nullptr;
}
template<typename T>
bool TAtomicWeakPointer<T>::operator!=(const StrongType& other) const
{
    return AtomicLoadPointer(&mNode->mPointer) != AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator!=(const WeakType& other) const
{
    return AtomicLoadPointer(&mNode->mPointer) != AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator!=(const NullPtr&) const
{
    return AtomicLoadPointer(&mNode->mPointer) != nullptr;
}

template<typename T>
TAtomicWeakPointer<T>::operator bool() const
{
    return AtomicLoadPointer(&mNode->mPointer) != nullptr;
}

template<typename T>
typename TAtomicWeakPointer<T>::Pointer TAtomicWeakPointer<T>::operator->()
{
    return AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
const typename TAtomicWeakPointer<T>::Pointer TAtomicWeakPointer<T>::operator->() const
{
    return AtomicLoadPointer(&mNode->mPointer);
}

template<typename T>
typename TAtomicWeakPointer<T>::ValueType& TAtomicWeakPointer<T>::operator*()
{
    return *AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
const typename TAtomicWeakPointer<T>::ValueType& TAtomicWeakPointer<T>::operator*() const
{
    return *AtomicLoadPointer(&mNode->mPointer);
}

template<typename T>
void TAtomicWeakPointer<T>::Release()
{
    DecrementRef();
    AtomicStorePointer(&mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    AtomicIncrement32(&mNode->mWeak);
}
template<typename T>
SizeT TAtomicWeakPointer<T>::GetWeakRefs() const
{
    return AtomicLoad(&mNode->mWeak);
}
template<typename T>
SizeT TAtomicWeakPointer<T>::GetStrongRefs() const
{
    return AtomicLoad(&mNode->mStrong);
}

} // namespace lf
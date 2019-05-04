// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_ATOMIC_SMART_POINTER_H
#define LF_CORE_ATOMIC_SMART_POINTER_H

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
        AtomicStorePointer(&mNode, AtomicLoadPointer(otherPtr->mNode));
        IncrementRef();
    }

    template<typename U>
    TAtomicStrongPointer(const TAtomicWeakPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TAtomicWeakPointer<T>* otherPtr = reinterpret_cast<const TAtomicWeakPointer<T>*>(&other);
        AtomicStorePointer(&mNode, AtomicLoadPointer(otherPtr->mNode));
        IncrementRef();
    }

    StrongType& operator=(const StrongType& other);
    StrongType& operator=(const WeakType& other);
    StrongType& operator=(const NullPtr&);
    StrongType& operator=(StrongType&& other);

    bool operator==(const StrongType& other);
    bool operator==(const WeakType& other);
    bool operator==(const NullPtr&);
    bool operator!=(const StrongType& other);
    bool operator!=(const WeakType& other);
    bool operator!=(const NullPtr&);

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
        AtomicStorePointer(&mNode, static_cast<NodeType*>(LFAlloc(sizeof(NodeType), alignof(NodeType), MMT_POINTER_NODE)));
        mNode->mStrong = 1;
        mNode->mWeak = 0;
        mNode->mPointer = nullptr;
    }
    void ReleaseNode()
    {
        mNode = AtomicLoadPointer(&mNode);
        Assert(mNode);
        Assert(!mNode->mPointer);
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
            if ((AtomicLoad(&mNode->mStrong) - 1) == 0)
            {
                DestroyPointer();
            }

            if (AtomicDecrement32(&mNode->mStrong) == 0)
            {
                if (AtomicLoad(&mNode->mWeak) == 0)
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


    bool operator==(const StrongType& other);
    bool operator==(const WeakType& other);
    bool operator==(const NullPtr&);
    bool operator!=(const StrongType& other);
    bool operator!=(const WeakType& other);
    bool operator!=(const NullPtr&);

    operator bool() const;
    operator Pointer() { return AtomicLoadPointer(&mNode->mPointer); }
    operator const Pointer() const { return AtomicLoadPointer(mNode->mPointer); }

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
            AtomicStorePointer(&mNode, nullptr);
        }
    }

    NodeType* volatile mNode;
};

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
        if ((AtomicLoad(&mNode->mStrong) - 1) == 0)
        {
            DestroyPointer();
        }

        if (AtomicDecrement32(&mNode->mStrong) == 0)
        {
            if (AtomicLoad(&mNode->mWeak) == 0)
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
bool TAtomicStrongPointer<T>::operator==(const StrongType& other)
{
    return AtomicLoadPointer(&other.mNode->mPointer) == AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator==(const WeakType& other)
{
    return AtomicLoadPointer(&other.mNode->mPointer) == AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator==(const NullPtr&)
{
    return AtomicLoadPointer(&mNode->mPointer) == nullptr;
}
template<typename T>
bool TAtomicStrongPointer<T>::operator!=(const StrongType& other)
{
    return AtomicLoadPointer(&other.mNode->mPointer) != AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator!=(const WeakType& other)
{
    return AtomicLoadPointer(&other.mNode->mPointer) != AtomicLoadPointer(&mNode->mPointer);
}
template<typename T>
bool TAtomicStrongPointer<T>::operator!=(const NullPtr&)
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
    AtomicStorePointer(other.mNode, reinterpret_cast<NodeType*>(&gNullAtomicPointerNode));
    other.IncrementRef();
    return *this;
}


template<typename T>
bool TAtomicWeakPointer<T>::operator==(const StrongType& other)
{
    return AtomicLoadPointer(&mNode->mPointer) == AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator==(const WeakType& other)
{
    return AtomicLoadPointer(&mNode->mPointer) == AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator==(const NullPtr&)
{
    return AtomicLoadPointer(&mNode->mPointer) == nullptr;
}
template<typename T>
bool TAtomicWeakPointer<T>::operator!=(const StrongType& other)
{
    return AtomicLoadPointer(&mNode->mPointer) != AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator!=(const WeakType& other)
{
    return AtomicLoadPointer(&mNode->mPointer) != AtomicLoadPointer(&other.mNode->mPointer);
}
template<typename T>
bool TAtomicWeakPointer<T>::operator!=(const NullPtr&)
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

#endif // LF_CORE_ATOMIC_SMART_POINTER_H
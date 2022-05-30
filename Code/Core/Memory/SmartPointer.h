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

#define DECLARE_PTR(T) class T; using T##Ptr = TStrongPointer<T>
#define DECLARE_WPTR(T) class T; using T##WPtr = TWeakPointer<T>

#define DECLARE_STRUCT_PTR(T) struct T; using T##Ptr = TStrongPointer<T>;
#define DECLARE_STRUCT_WPTR(T) struct T; using T##WPtr = TWeakPoitner<T>;

#define DECLARE_MANAGED_PTR(T) class T; using T##Ptr = TManagedPointer<T>
#define DECLARE_STRUCT_MANAGED_PTR(T) struct T; using T##Ptr = TManagedPointer<T>;

#define DECLARE_MANAGED_CPTR(T) class T; using T##CPtr = TConstManagedPointer<const T>;
#define DECLARE_STRUCT_MANAGED_CPTR(T) struct T; using T##CPtr = TConstManagedPointer<const T>;

namespace lf {

template<typename T>
struct TPointerNode
{
    T* mPointer;
    Int32 mStrong;
    Int32 mWeak;
};

template<typename T> class TStrongPointer;
template<typename T> class TWeakPointer;

template<typename T>
class TStrongPointer
{
    friend TWeakPointer<T>;
    using NodeType = TPointerNode<T>;
public:
    using ValueType = T;
    using Pointer = T*;
    using StrongType = TStrongPointer<T>;
    using WeakType = TWeakPointer<T>;

    TStrongPointer();
    TStrongPointer(const StrongType& other);
    TStrongPointer(StrongType&& other);
    TStrongPointer(const NullPtr&);
    explicit TStrongPointer(Pointer memory);
    TStrongPointer(const WeakType& other);
    ~TStrongPointer();

    template<typename U>
    TStrongPointer(const TStrongPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TStrongPointer<T>* otherPtr = reinterpret_cast<const TStrongPointer<T>*>(&other);
        mNode = otherPtr->mNode;
        IncrementRef();
    }

    template<typename U>
    TStrongPointer(const TWeakPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TWeakPointer<T>* otherPtr = reinterpret_cast<const TWeakPointer<T>*>(&other);
        mNode = otherPtr->mNode;
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
    operator Pointer() { return mNode->mPointer; }
    operator const Pointer() const { return mNode->mPointer; }

    Pointer AsPtr() { return mNode->mPointer; }
    const Pointer AsPtr() const { return mNode->mPointer; }

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
        Assert(!mNode);
        LF_SCOPED_MEMORY(MMT_POINTER_NODE);
        mNode = static_cast<NodeType*>(LFAlloc(sizeof(NodeType), alignof(NodeType)));
        mNode->mStrong = 1;
        mNode->mWeak = 0;
        mNode->mPointer = nullptr;
    }
    void ReleaseNode()
    {
        Assert(mNode);
        Assert(!mNode->mPointer);
        LFFree(mNode);
        mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    }
    void DestroyPointer()
    {
        Assert(mNode);
        if (mNode->mPointer)
        {
            Pointer temp = mNode->mPointer;
            mNode->mPointer = nullptr;
            temp->~T();
            LFFree(temp);
        }
    }
    void IncrementRef()
    {
        if (mNode)
        {
            ++mNode->mStrong;
        }
    }
    void DecrementRef()
    {
        if (mNode)
        {
            if ((mNode->mStrong - 1) == 0)
            {
                DestroyPointer();
            }

            --mNode->mStrong;
            if (mNode->mStrong == 0)
            {
                if (mNode->mWeak == 0)
                {
                    ReleaseNode();
                }
            }
        }
        mNode = nullptr;
    }

    NodeType* mNode;
};

template<typename T>
class TWeakPointer
{
    friend TStrongPointer<T>;
    using NodeType = TPointerNode<T>;
public:
    using ValueType = T;
    using Pointer = T*;
    using StrongType = TStrongPointer<T>;
    using WeakType = TWeakPointer<T>;

    TWeakPointer();
    TWeakPointer(const WeakType& other);
    TWeakPointer(WeakType&& other);
    TWeakPointer(const NullPtr&);
    TWeakPointer(const StrongType& other);
    ~TWeakPointer();

    template<typename U>
    TWeakPointer(const TStrongPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TStrongPointer<T>* otherPtr = reinterpret_cast<const TStrongPointer<T>*>(&other);
        mNode = otherPtr->mNode;
        IncrementRef();
    }

    template<typename U>
    TWeakPointer(const TWeakPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TWeakPointer<T>* otherPtr = reinterpret_cast<const TWeakPointer<T>*>(&other);
        mNode = otherPtr->mNode;
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
    operator Pointer() { return mNode->mPointer; }
    operator const Pointer() const { return mNode->mPointer; }

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
        Assert(mNode);
        Assert(!mNode->mPointer);
        LFFree(mNode);
        mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    }
    void IncrementRef()
    {
        if (mNode)
        {
            ++mNode->mWeak;
        }
    }
    void DecrementRef()
    {
        if (mNode)
        {
            --mNode->mWeak;
            if (mNode->mWeak == 0)
            {
                if (mNode->mStrong == 0)
                {
                    ReleaseNode();
                }
            }
        }
        mNode = nullptr;
    }

    NodeType* mNode;
};

// Convertible Smart Pointers
// usage:
// class MyType : public TWeakPointerConvertible
// 
// public: 
//      // ### Use this typedef to automatically create a convertible weak pointer when using ReflectionMgr
//      using PointerConvertible = PointerConvertibleType;
// 
// ptr = MakeConvertiblePtr<MyType>();
// wptr = GetPointer(rawPtr);
//
template<typename T>
struct TWeakPointerConvertible
{
public:
    const TWeakPointer<T>& GetWeakPointer() const { return mPointer; }
    TWeakPointer<T>& GetWeakPointer() { return mPointer; }
private:
    TWeakPointer<T> mPointer;
};

// ********************************************************************
// This represents a managed pointer (managed meaning the allocation/free
// is managed else where)
//
// The TManagedPointer just provides convience reference count management.
// 
// The 'owner' (one that manages memory lifetime of pointer) MUST NOT 
// release the memory, unless the ref count is 0.
// 
// The 'owner' SHOULD verify the ref count never goes below 0.
//
// typename T must implement 2 methods 
//          'void IncrementRef()' -- Increment the reference count by one 
//          'void DecrementRef()' -- Decrement the reference count by one
//
// TODO: TLockingManagedPointer (Use locks to get TManagedPointer)
// ********************************************************************
template<typename T>
class TManagedPointer
{
public:
    using ValueType = T;

    TManagedPointer() : mPointer(nullptr) {}
    TManagedPointer(const TManagedPointer& other) : mPointer(other.mPointer) { IncrementRef(); }
    TManagedPointer(TManagedPointer&& other) : mPointer(other.mPointer) { other.mPointer = nullptr; }
    TManagedPointer(const NullPtr&) : mPointer(nullptr) {}
    explicit TManagedPointer(T* other) : mPointer(other) { IncrementRef(); }
    ~TManagedPointer() { DecrementRef(); }

    template<typename U>
    TManagedPointer(const TManagedPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TManagedPointer<T>* otherPtr = reinterpret_cast<const TManagedPointer<T>*>(&other);
        mNode = otherPtr->mNode;
        IncrementRef();
    }

    TManagedPointer& operator=(const TManagedPointer& other)
    {
        if (&other != this)
        {
            Release();
            mPointer = other.mPointer;
            IncrementRef();
        }
        return *this;
    }

    TManagedPointer& operator=(TManagedPointer&& other)
    {
        if (&other != this)
        {
            Release();
            mPointer = other.mPointer;
            other.mPointer = nullptr;
        }
        return *this;
    }

    TManagedPointer& operator=(T* other)
    {
        if (other != mPointer)
        {
            Release();
            mPointer = other;
            IncrementRef();
        }
        return *this;
    }
    TManagedPointer& operator=(const NullPtr&)
    {
        Release();
        return *this;
    }

    void Release()
    {
        DecrementRef();
        mPointer = nullptr;
    }

    operator T* () { return mPointer; }
    operator const T* () const { return mPointer; }

    T& operator*() { return *mPointer; }
    const T& operator*() const { return *mPointer; }
    T* operator->() { return mPointer; }
    const T* operator->() const { return mPointer; }

private:
    void DecrementRef() { if (mPointer) mPointer->DecrementRef(); }
    void IncrementRef() { if (mPointer) mPointer->IncrementRef(); }

    T* mPointer;
};

template<typename T>
class TConstManagedPointer
{
public:
    using ValueType = T;

    TConstManagedPointer() : mPointer(nullptr) {}
    TConstManagedPointer(const TConstManagedPointer& other) : mPointer(other.mPointer) { IncrementRef(); }
    TConstManagedPointer(TConstManagedPointer&& other) : mPointer(other.mPointer) { other.mPointer = nullptr; }
    TConstManagedPointer(const TManagedPointer<T>& other) : mPointer(other.mPointer) { IncrementRef(); }
    TConstManagedPointer(const NullPtr&) : mPointer(nullptr) {}
    explicit TConstManagedPointer(const T* other) : mPointer(other) { IncrementRef(); }
    ~TConstManagedPointer() { DecrementRef(); }

    template<typename U>
    TConstManagedPointer(const TConstManagedPointer<U>& other) : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        const TManagedPointer<T>* otherPtr = reinterpret_cast<const TManagedPointer<T>*>(&other);
        mNode = otherPtr->mNode;
        IncrementRef();
    }

    TConstManagedPointer& operator=(const TConstManagedPointer& other)
    {
        if (&other != this)
        {
            Release();
            mPointer = other.mPointer;
            IncrementRef();
        }
        return *this;
    }

    TConstManagedPointer& operator=(TConstManagedPointer&& other)
    {
        if (&other != this)
        {
            Release();
            mPointer = other.mPointer;
            other.mPointer = nullptr;
        }
        return *this;
    }

    TConstManagedPointer& operator=(T* other)
    {
        if (other != mPointer)
        {
            Release();
            mPointer = other;
            IncrementRef();
        }
        return *this;
    }
    TConstManagedPointer& operator=(const NullPtr&)
    {
        Release();
        return *this;
    }

    void Release()
    {
        mPointer = nullptr;
        DecrementRef();
    }

    operator const T* () const { return mPointer; }
    const T& operator*() const { return *mPointer; }
    const T* operator->() const { return mPointer; }

private:
    void DecrementRef() { if (mPointer) mPointer->DecrementRef(); }
    void IncrementRef() { if (mPointer) mPointer->IncrementRef(); }

    const T* mPointer;
};

template<typename T>
TStrongPointer<T> MakeConvertiblePtr()
{
    TStrongPointer<T> ptr(LFNew<T>());
    ptr->GetWeakPointer() = ptr;
    return ptr;
}

template<typename T, typename ... ARGS>
TStrongPointer<T> MakeConvertiblePtr(ARGS&&... args)
{
    TStrongPointer<T> ptr(LFNew<T>(std::forward<ARGS>(args)...));
    ptr->GetWeakPointer() = ptr;
    return ptr;
}

template<typename T>
TWeakPointer<T> GetPointer(T* self)
{
    if (!self)
    {
        return NULL_PTR;
    }
    return StaticCast<TWeakPointer<T>>(self->GetWeakPointer());
}

template<typename T>
const TWeakPointer<T>& GetPointer(const T* self)
{
    if (!self)
    {
        return *reinterpret_cast<const TWeakPointer<T>*>(&NULL_PTR);
    }
    return StaticCast<TWeakPointer<T>>(self->GetWeakPointer());
}

template<typename T>
TStrongPointer<T>::TStrongPointer() : mNode(reinterpret_cast<NodeType*>(&gNullPointerNode))
{
    IncrementRef();
}
template<typename T>
TStrongPointer<T>::TStrongPointer(const StrongType& other) : mNode(other.mNode)
{
    IncrementRef();
}
template<typename T>
TStrongPointer<T>::TStrongPointer(StrongType&& other) : mNode(other.mNode)
{
    // todo: This doesn't seem atomic... Need to InterlockedExchangePointer
    other.mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    other.IncrementRef();
}
template<typename T>
TStrongPointer<T>::TStrongPointer(const NullPtr&) : mNode(reinterpret_cast<NodeType*>(&gNullPointerNode))
{
    IncrementRef();
}
template<typename T>
TStrongPointer<T>::TStrongPointer(Pointer memory) : mNode(reinterpret_cast<NodeType*>(&gNullPointerNode))
{
    if (memory)
    {
        mNode = nullptr;
        AllocNode();
        mNode->mPointer = memory;
    }
    else
    {
        IncrementRef();
    }
}
template<typename T>
TStrongPointer<T>::TStrongPointer(const WeakType& other) : mNode(other.mNode)
{
    IncrementRef();
}

template<typename T>
TStrongPointer<T>::~TStrongPointer()
{
    if (mNode)
    {
        if ((mNode->mStrong - 1) == 0)
        {
            DestroyPointer();
        }

        --mNode->mStrong;
        if (mNode->mStrong == 0)
        {
            if (mNode->mWeak == 0)
            {
                ReleaseNode();
            }
        }
        mNode = nullptr;
    }
}

template<typename T>
typename TStrongPointer<T>::StrongType& TStrongPointer<T>::operator=(const StrongType& other)
{
    if (this == &other)
    {
        return *this;
    }
    DecrementRef();
    mNode = other.mNode;
    IncrementRef();
    return *this;
}
template<typename T>
typename TStrongPointer<T>::StrongType& TStrongPointer<T>::operator=(const WeakType& other)
{
    if (mNode == other.mNode)
    {
        return *this;
    }
    DecrementRef();
    mNode = other.mNode;
    IncrementRef();
    return *this;
}
template<typename T>
typename TStrongPointer<T>::StrongType& TStrongPointer<T>::operator=(const NullPtr&)
{
    DecrementRef();
    mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    ++mNode->mStrong;
    return *this;
}
template<typename T>
typename TStrongPointer<T>::StrongType& TStrongPointer<T>::operator=(StrongType&& other)
{
    if (this == &other)
    {
        return *this;
    }
    DecrementRef();
    mNode = other.mNode;
    other.mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);;
    other.IncrementRef();
    return *this;
}


template<typename T>
bool TStrongPointer<T>::operator==(const StrongType& other) const
{
    return other.mNode->mPointer == mNode->mPointer;
}
template<typename T>
bool TStrongPointer<T>::operator==(const WeakType& other) const
{
    return other.mNode->mPointer == mNode->mPointer;
}
template<typename T>
bool TStrongPointer<T>::operator==(const NullPtr&) const
{
    return mNode->mPointer == nullptr;
}
template<typename T>
bool TStrongPointer<T>::operator!=(const StrongType& other) const
{
    return other.mNode->mPointer != mNode->mPointer;
}
template<typename T>
bool TStrongPointer<T>::operator!=(const WeakType& other) const
{
    return other.mNode->mPointer != mNode->mPointer;
}
template<typename T>
bool TStrongPointer<T>::operator!=(const NullPtr&) const
{
    return mNode->mPointer != nullptr;
}

template<typename T>
TStrongPointer<T>::operator bool() const
{
    return mNode->mPointer != nullptr;
}

template<typename T>
typename TStrongPointer<T>::Pointer TStrongPointer<T>::operator->()
{
    return mNode->mPointer;
}
template<typename T>
const typename TStrongPointer<T>::Pointer TStrongPointer<T>::operator->() const
{
    return mNode->mPointer;
}

template<typename T>
typename TStrongPointer<T>::ValueType& TStrongPointer<T>::operator*()
{
    return *mNode->mPointer;
}
template<typename T>
const typename TStrongPointer<T>::ValueType& TStrongPointer<T>::operator*() const
{
    return *mNode->mPointer;
}

template<typename T>
void TStrongPointer<T>::Release()
{
    DecrementRef();
    mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    ++mNode->mStrong;
}
template<typename T>
SizeT TStrongPointer<T>::GetWeakRefs() const
{
    return mNode->mWeak;
}
template<typename T>
SizeT TStrongPointer<T>::GetStrongRefs() const
{
    return mNode->mStrong;
}

template<typename T>
TWeakPointer<T>::TWeakPointer() : mNode(reinterpret_cast<NodeType*>(&gNullPointerNode))
{
    IncrementRef();
}
template<typename T>
TWeakPointer<T>::TWeakPointer(const WeakType& other) : mNode(other.mNode)
{
    IncrementRef();
}
template<typename T>
TWeakPointer<T>::TWeakPointer(WeakType&& other) : mNode(other.mNode)
{
    // todo: InterlockedExchangePointer
    other.mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    other.IncrementRef();
}
template<typename T>
TWeakPointer<T>::TWeakPointer(const NullPtr&) : mNode(reinterpret_cast<NodeType*>(&gNullPointerNode))
{
    IncrementRef();
}
template<typename T>
TWeakPointer<T>::TWeakPointer(const StrongType& other) : mNode(other.mNode)
{
    IncrementRef();
}
template<typename T>
TWeakPointer<T>::~TWeakPointer()
{
    if (mNode)
    {
        --mNode->mWeak;
        if (mNode->mWeak == 0)
        {
            if (mNode->mStrong == 0)
            {
                ReleaseNode();
            }
        }
        mNode = nullptr;
    }
}


template<typename T>
typename TWeakPointer<T>::WeakType& TWeakPointer<T>::operator=(const StrongType& other)
{
    DecrementRef();
    mNode = other.mNode;
    IncrementRef();
    return *this;
}
template<typename T>
typename TWeakPointer<T>::WeakType& TWeakPointer<T>::operator=(const WeakType& other)
{
    DecrementRef();
    mNode = other.mNode;
    IncrementRef();
    return *this;
}
template<typename T>
typename TWeakPointer<T>::WeakType& TWeakPointer<T>::operator=(const NullPtr&)
{
    DecrementRef();
    mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    ++mNode->mWeak;
    return *this;
}
template<typename T>
typename TWeakPointer<T>::WeakType& TWeakPointer<T>::operator=(WeakType&& other)
{
    if (this == &other)
    {
        return *this;
    }
    // todo: InterlockedExchangePointer
    DecrementRef();
    mNode = other.mNode;
    other.mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    other.IncrementRef();
    return *this;
}


template<typename T>
bool TWeakPointer<T>::operator==(const StrongType& other) const
{
    return mNode->mPointer == other.mNode->mPointer;
}
template<typename T>
bool TWeakPointer<T>::operator==(const WeakType& other) const
{
    return mNode->mPointer == other.mNode->mPointer;
}
template<typename T>
bool TWeakPointer<T>::operator==(const NullPtr&) const
{
    return mNode->mPointer == nullptr;
}
template<typename T>
bool TWeakPointer<T>::operator!=(const StrongType& other) const
{
    return mNode->mPointer != other.mNode->mPointer;
}
template<typename T>
bool TWeakPointer<T>::operator!=(const WeakType& other) const
{
    return mNode->mPointer != other.mNode->mPointer;
}
template<typename T>
bool TWeakPointer<T>::operator!=(const NullPtr&) const
{
    return mNode->mPointer != nullptr;
}

template<typename T>
TWeakPointer<T>::operator bool() const
{
    return mNode->mPointer != nullptr;
}

template<typename T>
typename TWeakPointer<T>::Pointer TWeakPointer<T>::operator->()
{
    return mNode->mPointer;
}
template<typename T>
const typename TWeakPointer<T>::Pointer TWeakPointer<T>::operator->() const
{
    return mNode->mPointer;
}

template<typename T>
typename TWeakPointer<T>::ValueType& TWeakPointer<T>::operator*()
{
    return *mNode->mPointer;
}
template<typename T>
const typename TWeakPointer<T>::ValueType& TWeakPointer<T>::operator*() const
{
    return *mNode->mPointer;
}

template<typename T>
void TWeakPointer<T>::Release()
{
    DecrementRef();
    mNode = reinterpret_cast<NodeType*>(&gNullPointerNode);
    ++mNode->mWeak;
}
template<typename T>
SizeT TWeakPointer<T>::GetWeakRefs() const
{
    return mNode->mWeak;
}
template<typename T>
SizeT TWeakPointer<T>::GetStrongRefs() const
{
    return mNode->mStrong;
}

} // namespace lf
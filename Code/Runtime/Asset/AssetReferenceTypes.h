#pragma once
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
#include "Core/Reflection/Type.h"
#include "Core/IO/Stream.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/AssetTypeInfo.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/AssetMgr.h"
#include "Runtime/Reflection/ReflectionTypes.h"

// DECLARE_ASSET
// DECLARE_ASSET_TYPE

namespace lf {


#define DECLARE_ASSET(Type_) \
class Type_;                 \
using Type_##Asset = ::lf::TAsset<Type_>  \


#define DECLARE_ASSET_TYPE(Type_)                   \
class Type_;                                        \
using Type_##AssetType = ::lf::TAssetType<Type_>    \


template<typename T>
struct TAssetHandle : public UnknownAssetHandle
{
    T*                   mPrototype;
    volatile Atomic32    mStrongRefs;
    volatile Atomic32    mWeakRefs;
    const AssetTypeInfo* mType;
};

class AssetMgr;

struct LF_RUNTIME_API DefaultAssetMgrProvider
{
    static AssetMgr& GetManager();
};

template<typename T, typename TProvider> class TAsset;
template<typename T, typename TProvider> class TAssetType;

template<typename T, typename TProvider = DefaultAssetMgrProvider>
class TAsset
{
    friend TAssetType<T, TProvider>;
public:
    using ValueType = T;
    using Pointer = T*;
    using StrongType = TAsset<T, TProvider>;
    using WeakType = TAssetType<T, TProvider>;
    using HandleType = TAssetHandle<T>;

    TAsset();
    TAsset(const NullPtr&);
    explicit TAsset(const AssetPath& path, AssetLoadFlags::Value flags);
    explicit TAsset(const AssetTypeInfo* type, AssetLoadFlags::Value flags);
    explicit TAsset(const WeakType& type, AssetLoadFlags::Value flags);
    TAsset(const StrongType& other);
    TAsset(const WeakType& other);
    TAsset(WeakType&& other);
    ~TAsset();

    template<typename U>
    TAsset(const TAssetType<U, TProvider>& other)
    : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        mNode = reinterpret_cast<const TAssetType<T, TProvider>&>(other);
        AtomicIncrement32(&mNode->mStrongRefs);
    }

    template<typename U>
    TAsset(const TAsset<U, TProvider>& other)
    : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        mNode = reinterpret_cast<const TAsset<T, TProvider>&>(other);
        AtomicIncrement32(&mNode->mStrongRefs);
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

    const Pointer operator->() const;
    const ValueType& operator*() const;

    void Acquire(const AssetPath& path, AssetLoadFlags::Value flags);
    void Acquire(const AssetTypeInfo* type, AssetLoadFlags::Value flags);
    void Release();

    SizeT GetWeakRefs() const;
    SizeT GetStrongRefs() const;

    bool IsA(const Type* other) const;
    bool IsA(const AssetTypeInfo* other) const;
    bool IsA(const WeakType& other) const;
    bool IsA(const StrongType& other) const;

    bool IsLoaded() const;

    AssetTypeInfoCPtr GetType() const;
    const AssetPath& GetPath() const;
    const Type* GetConcreteType() const;
    const Pointer GetPrototype() const;
private:
    AssetMgr& Mgr() { return TProvider::GetManager(); }
    AssetMgr& Mgr() const { return TProvider::GetManager(); }


    HandleType* mNode;
};

template<typename T, typename TProvider = DefaultAssetMgrProvider>
class TAssetType
{
    friend TAsset<T, TProvider>;
public:
    using ValueType = T;
    using Pointer = T*;
    using StrongType = TAsset<T, TProvider>;
    using WeakType = TAssetType<T, TProvider>;
    using HandleType = TAssetHandle<T>;

    TAssetType();
    TAssetType(const NullPtr&);
    explicit TAssetType(const AssetPath& path);
    explicit TAssetType(const AssetTypeInfo* type);
    TAssetType(const StrongType& other);
    TAssetType(const WeakType& other);
    TAssetType(WeakType&& other);
    ~TAssetType();

    template<typename U>
    TAssetType(const TAssetType<U, TProvider>& other)
    : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        mNode = reinterpret_cast<const TAssetType<T, TProvider>&>(other).mNode;
        AtomicIncrement32(&mNode->mWeakRefs);
    }

    template<typename U>
    TAssetType(const TAsset<U, TProvider>& other)
    : mNode(nullptr)
    {
        LF_STATIC_IS_A(U, ValueType);
        mNode = reinterpret_cast<const TAsset<T, TProvider>&>(other).mNode;
        AtomicIncrement32(&mNode->mWeakRefs);
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

    template<typename U>
    bool operator==(const TAssetType<U, TProvider>& other)
    {
        LF_STATIC_IS_A(U, ValueType);
        return mNode == reinterpret_cast<const TAssetType<T, TProvider>&>(other).mNode;
    }
    template<typename U>
    bool operator==(const TAsset<U, TProvider>& other)
    {
        LF_STATIC_IS_A(U, ValueType);
        return mNode == reinterpret_cast<const TAsset<T, TProvider>&>(other).mNode;
    }

    template<typename U>
    bool operator!=(const TAssetType<U, TProvider>& other)
    {
        LF_STATIC_IS_A(U, ValueType);
        return mNode != reinterpret_cast<const TAssetType<T, TProvider>&>(other).mNode;
    }
    template<typename U>
    bool operator!=(const TAsset<U, TProvider>& other)
    {
        LF_STATIC_IS_A(U, ValueType);
        return mNode != reinterpret_cast<const TAsset<T, TProvider>&>(other).mNode;
    }

    operator bool() const;
    operator AssetTypeInfoCPtr () const { return GetType(); }
    AssetTypeInfoCPtr operator->() const { return GetType(); }

    void Acquire(const AssetPath& path);
    void Acquire(const AssetTypeInfo* type);
    void Release();

    SizeT GetWeakRefs() const;
    SizeT GetStrongRefs() const;

    bool IsA(const Type* other) const;
    bool IsA(const AssetTypeInfo* other) const;
    bool IsA(const WeakType& other) const;
    bool IsA(const StrongType& other) const;

    AssetTypeInfoCPtr GetType() const;
    const AssetPath& GetPath() const;
    const Type* GetConcreteType() const;
private:
    AssetMgr& Mgr() const { return TProvider::GetManager(); }
    HandleType* mNode;
};

// =========================================================================
// TAsset
// =========================================================================
template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset()
: mNode(nullptr)
{
    Mgr().AcquireStrongNull(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}

template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset(const NullPtr&)
: mNode(nullptr)
{
    Mgr().AcquireStrongNull(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}
template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset(const AssetPath& path, AssetLoadFlags::Value flags)
: mNode(nullptr)
{
    Acquire(path, flags);
}
template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset(const AssetTypeInfo* type, AssetLoadFlags::Value flags)
: mNode(nullptr)
{
    Acquire(type, flags);
}
template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset(const WeakType& type, AssetLoadFlags::Value flags)
: mNode(nullptr)
{
    Acquire(type.GetType(), flags);
}

template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset(const StrongType& other)
: mNode(other.mNode)
{
    AtomicIncrement32(&mNode->mStrongRefs);
}
template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset(const WeakType& other)
: mNode(nullptr)
{
    Acquire(other.GetType(), AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_ASYNC);
}
template<typename T, typename TProvider>
TAsset<T, TProvider>::TAsset(WeakType&& other)
: mNode(other.mNode)
{
    other.mNode = nullptr;
}
template<typename T, typename TProvider>
TAsset<T, TProvider>::~TAsset()
{
    Mgr().ReleaseStrong(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}
template<typename T, typename TProvider>
typename TAsset<T, TProvider>::StrongType& TAsset<T, TProvider>::operator=(const StrongType& other)
{
    if (this == &other)
    {
        return *this;
    }
    Release();
    mNode = other.mNode;
    AtomicIncrement32(&mNode->mStrongRefs);
    return *this;
}
template<typename T, typename TProvider>
typename TAsset<T, TProvider>::StrongType& TAsset<T, TProvider>::operator=(const WeakType& other)
{
    Release();
    mNode = other.mNode;
    AtomicIncrement32(&mNode->mStrongRefs);
    return *this;
}
template<typename T, typename TProvider>
typename TAsset<T, TProvider>::StrongType& TAsset<T, TProvider>::operator=(const NullPtr&)
{
    Release();
    return *this;
}
template<typename T, typename TProvider>
typename TAsset<T, TProvider>::StrongType& TAsset<T, TProvider>::operator=(StrongType&& other)
{
    if (this == &other)
    {
        return *this;
    }
    Release();
    mNode = other.mNode;
    other.mNode = nullptr;
    return *this;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::operator==(const StrongType& other) const
{
    return mNode == other.mNode;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::operator==(const WeakType& other) const
{
    return mNode == other.mNode;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::operator==(const NullPtr&) const
{
    return Mgr().IsNull(mNode);
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::operator!=(const StrongType& other) const
{
    return mNode != other.mNode;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::operator!=(const WeakType& other) const
{
    return mNode != other.mNode;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::operator!=(const NullPtr&) const
{
    return !Mgr().IsNull(mNode);
}
template<typename T, typename TProvider>
TAsset<T, TProvider>::operator bool() const
{
    return !Mgr().IsNull(mNode);
}
template<typename T, typename TProvider>
const typename TAsset<T, TProvider>::Pointer TAsset<T, TProvider>::operator->() const
{
    return mNode->mPrototype;
}
template<typename T, typename TProvider>
const typename TAsset<T, TProvider>::ValueType& TAsset<T, TProvider>::operator*() const
{
    CriticalAssert(mNode->mPrototype != nullptr);
    return *mNode->mPrototype;
}
template<typename T, typename TProvider>
void TAsset<T, TProvider>::Acquire(const AssetPath& path, AssetLoadFlags::Value flags)
{
    Mgr().AcquireStrong(reinterpret_cast<UnknownAssetHandle*&>(mNode), path, typeof(T), flags);
}
template<typename T, typename TProvider>
void TAsset<T, TProvider>::Acquire(const AssetTypeInfo* type, AssetLoadFlags::Value flags)
{
    Mgr().AcquireStrong(reinterpret_cast<UnknownAssetHandle*&>(mNode), type, typeof(T), flags);
}
template<typename T, typename TProvider>
void TAsset<T, TProvider>::Release()
{
    Mgr().AcquireStrongNull(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}
template<typename T, typename TProvider>
SizeT TAsset<T, TProvider>::GetWeakRefs() const
{
    return mNode->mWeakRefs;
}
template<typename T, typename TProvider>
SizeT TAsset<T, TProvider>::GetStrongRefs() const
{
    return mNode->mStrongRefs;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::IsA(const Type* other) const
{
    CriticalAssert(other);
    const Type* self = GetConcreteType();
    return self ? self->IsA(other) : false;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::IsA(const AssetTypeInfo* other) const
{
    CriticalAssert(other);
    const AssetTypeInfo* self = GetType();
    return self ? self->IsA(other) : false;
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::IsA(const WeakType& other) const
{
    return IsA(other.GetType());
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::IsA(const StrongType& other) const
{
    return IsA(other.GetType());
}
template<typename T, typename TProvider>
bool TAsset<T, TProvider>::IsLoaded() const
{
    return mNode->mType ? AssetLoadState::ALS_LOADED == mNode->mType->GetLoadState() : false;
}
template<typename T, typename TProvider>
AssetTypeInfoCPtr TAsset<T, TProvider>::GetType() const
{
    return AssetTypeInfoCPtr(mNode->mType);
}
template<typename T, typename TProvider>
const AssetPath& TAsset<T, TProvider>::GetPath() const
{
    return mNode->mType ? mNode->mType->GetPath() : EMPTY_PATH;
}
template<typename T, typename TProvider>
const Type* TAsset<T, TProvider>::GetConcreteType() const
{
    return mNode->mType ? mNode->mType->GetConcreteType() : nullptr;
}
template<typename T, typename TProvider>
const typename TAsset<T, TProvider>::Pointer TAsset<T, TProvider>::GetPrototype() const
{
    return mNode->mPrototype;
}
// =========================================================================
// TAssetType
// =========================================================================
template<typename T, typename TProvider>
TAssetType<T, TProvider>::TAssetType()
: mNode(nullptr)
{
    Mgr().AcquireWeakNull(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::TAssetType(const NullPtr&)
: mNode(nullptr)
{
    Mgr().AcquireWeakNull(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::TAssetType(const AssetPath& path)
: mNode(nullptr)
{
    Acquire(path);
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::TAssetType(const AssetTypeInfo* type)
: mNode(nullptr)
{
    Acquire(type);
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::TAssetType(const StrongType& other)
: mNode(other.mNode)
{
    AtomicIncrement32(&other.mNode->mWeakRefs);
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::TAssetType(const WeakType& other)
: mNode(other.mNode)
{
    AtomicIncrement32(&other.mNode->mWeakRefs);
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::TAssetType(WeakType&& other)
: mNode(other.mNode)
{
    other.mNode = nullptr;
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::~TAssetType()
{
    Mgr().ReleaseWeak(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}
template<typename T, typename TProvider>
typename TAssetType<T, TProvider>::WeakType & TAssetType<T, TProvider>::operator=(const StrongType& other)
{
    Release();
    mNode = other.mNode;
    AtomicIncrement32(&mNode->mWeakRefs);
    return *this;
}
template<typename T, typename TProvider>
typename TAssetType<T, TProvider>::WeakType& TAssetType<T, TProvider>::operator=(const WeakType& other)
{
    if (this == &other)
    {
        return *this;
    }
    Release();
    mNode = other.mNode;
    AtomicIncrement32(&mNode->mWeakRefs);
    return *this;
}
template<typename T, typename TProvider>
typename TAssetType<T, TProvider>::WeakType& TAssetType<T, TProvider>::operator=(const NullPtr&)
{
    Release();
    return *this;
}
template<typename T, typename TProvider>
typename TAssetType<T, TProvider>::WeakType& TAssetType<T, TProvider>::operator=(WeakType&& other)
{
    if (this == &other)
    {
        return *this;
    }
    Release();
    mNode = other.mNode;
    other.mNode = nullptr;
    return *this;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::operator==(const StrongType& other) const
{
    return mNode == other.mNode;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::operator==(const WeakType& other) const
{
    return mNode == other.mNode;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::operator==(const NullPtr&) const
{
    return Mgr().IsNull(mNode);
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::operator!=(const StrongType& other) const
{
    return mNode != other.mNode;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::operator!=(const WeakType& other) const
{
    return mNode != other.mNode;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::operator!=(const NullPtr&) const
{
    return !Mgr().IsNull(mNode);
}
template<typename T, typename TProvider>
TAssetType<T, TProvider>::operator bool() const
{
    return !Mgr().IsNull(mNode);
}
template<typename T, typename TProvider>
void TAssetType<T, TProvider>::Acquire(const AssetPath& path)
{
    Mgr().AcquireWeak(reinterpret_cast<UnknownAssetHandle*&>(mNode), path, typeof(T));
}
template<typename T, typename TProvider>
void TAssetType<T, TProvider>::Acquire(const AssetTypeInfo* type)
{
    Mgr().AcquireWeak(reinterpret_cast<UnknownAssetHandle*&>(mNode), type, typeof(T));
}
template<typename T, typename TProvider>
void TAssetType<T, TProvider>::Release()
{
    Mgr().AcquireWeakNull(reinterpret_cast<UnknownAssetHandle*&>(mNode));
}
template<typename T, typename TProvider>
SizeT TAssetType<T, TProvider>::GetWeakRefs() const
{
    return mNode->mWeakRefs;
}
template<typename T, typename TProvider>
SizeT TAssetType<T, TProvider>::GetStrongRefs() const
{
    return mNode->mStrongRefs;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::IsA(const Type* other) const
{
    CriticalAssert(other);
    const Type* self = GetConcreteType();
    return self ? self->IsA(other) : false;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::IsA(const AssetTypeInfo* other) const
{
    CriticalAssert(other);
    const AssetTypeInfo* self = GetType();
    return self ? self->IsA(other) : false;
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::IsA(const WeakType& other) const
{
    return IsA(other.GetType());
}
template<typename T, typename TProvider>
bool TAssetType<T, TProvider>::IsA(const StrongType& other) const
{
    return IsA(other.GetType());
}
template<typename T, typename TProvider>
AssetTypeInfoCPtr TAssetType<T, TProvider>::GetType() const
{
    return AssetTypeInfoCPtr(mNode->mType);
}
template<typename T, typename TProvider>
const AssetPath& TAssetType<T, TProvider>::GetPath() const
{
    return mNode->mType ? mNode->mType->GetPath() : EMPTY_PATH;
}
template<typename T, typename TProvider>
const Type* TAssetType<T, TProvider>::GetConcreteType() const
{
    return mNode->mType ? mNode->mType->GetConcreteType() : nullptr;
}

template<typename T, typename TProvider>
Stream& operator<<(Stream& s, TAsset<T, TProvider>& asset)
{
    if (s.IsReading())
    {
        Token assetName;
        s.SerializeAsset(assetName, false);
        asset.Acquire(AssetPath(assetName), s.GetAssetLoadFlags());
    }
    else
    {
        Token assetName = asset.GetPath().AsToken();
        s.SerializeAsset(assetName, false);
    }
    return s;
}

template<typename T, typename TProvider>
Stream& operator<<(Stream& s, TAssetType<T, TProvider>& asset)
{
    if (s.IsReading())
    {
        Token assetName;
        s.SerializeAsset(assetName, true);
        asset.Acquire(AssetPath(assetName));
    }
    else
    {
        Token assetName = asset.GetPath().AsToken();
        s.SerializeAsset(assetName, true);
    }
    return s;
}

template <typename T>
struct TAssetTypeLess
{
    bool operator()(const TAssetType<T>& left, const TAssetType<T>& right) const
    {
        return left.GetType() < right.GetType();
    }
};

} // namespace lf

#pragma once
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
#include "Core/Platform/RWLock.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/DateTime.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/CacheTypes.h"


namespace lf {

class AssetDataController;
class Type;
struct AssetHandle;
DECLARE_ATOMIC_WPTR(AssetObject);
DECLARE_MANAGED_CPTR(AssetTypeInfo);

// ********************************************************************
// Represents runtime type information about an asset.
// 
// Threading:
//      Modifying Source Data: (External Program)
//      Modifying Cache Data: Acquire write lock
//      Modifying Runtime Type Info: Acquire write lock
//      Reading Runtime Type Info: 
//              a) Non-Volatile, No lock
//              b) Volatile, Read Lock
//      Reading Cache Data: Acquire read lock
//      Reading Source Data: Acquire read lock
// ********************************************************************
class LF_RUNTIME_API AssetTypeInfo
{
    using WeakAssetInstanceArray = TVector<AssetObjectAtomicWPtr>;
    friend class AssetDataController;
public:
    AssetTypeInfo();
    AssetTypeInfo(AssetTypeInfo&& other);

    AssetTypeInfo& operator=(AssetTypeInfo&& other);

    bool IsA(const AssetTypeInfo* type) const;

    const AssetPath& GetPath() const { return mPath; }
    AssetTypeInfoCPtr GetParent() const { return AssetTypeInfoCPtr(mParent); }
    const Type* GetConcreteType() const { return mConcreteType; }
    CacheIndex GetCacheIndex() const { return mCacheIndex; }
    // ********************************************************************
    // The number of assets that reference this type (weak relationship)
    // ********************************************************************
    UInt32 GetWeakReferences() const { return mWeakReferences; }
    // ********************************************************************
    // The number of assets that reference this type (strong relationship)
    // ********************************************************************
    UInt32 GetStrongReferences() const { return mStrongReferences; }

    DateTime GetModifyDate() const { return mModifyDate; }

    AssetLoadState::Value GetLoadState() const { return mLoadState; }
    AssetOpState::Value GetOpState() const { /* ScopeRWLockRead lock(mOpStateLock); */ return mOpState; }

    RWLock& GetLock() const { return mLock; }

    bool IsPrototype(const AssetObject* object) const { return object && mHandle->mPrototype == object; }
    bool IsConcrete() const { return mParent == nullptr; }

    void IncrementRef() const { AtomicIncrement32(&mRefs); }
    void DecrementRef() const { Assert(AtomicDecrement32(&mRefs) >= 0); }
    SizeT GetRefs() const { return static_cast<SizeT>(AtomicLoad(&mRefs)); }
private:
    AssetTypeInfo(const AssetTypeInfo&) = delete;
    AssetTypeInfo& operator=(const AssetTypeInfo&) = delete;

    // ** This is how the asset is identified (in code, in data) and its a path to asset in source as well.
    AssetPath                   mPath;
    // ** Pointer to the parent asset type
    const AssetTypeInfo*        mParent;
    // ** 
    const Type*                 mConcreteType;

    // Cache-Info
    CacheIndex                  mCacheIndex;

    // Runtime
    // ** Pointer to the handle this type corresponds to
    AssetHandle*                mHandle;
    // ** Pointer to the instances this type corresponds to
    WeakAssetInstanceArray*     mInstances;
    // ** Pointer to the controller this type corresponds to
    AssetDataController*        mController;
    // ** A hash of the data to compare changes with
    AssetHash                   mModifyHash;
    // ** The last modify date
    DateTime                    mModifyDate;

    // ** The current load state, only those who actually acquire the 'op state' lock are qualified to
    //    modify this.
    AssetLoadState::Value       mLoadState;

    // [DEPRECATED]
    AssetOpState::Value         mOpState;

    // RWLock doesn't support copying.. for pretty good reason  (maybe we can just have functions exposed)
    // or moving...
    // mutable RWLock              mOpStateLock;

    mutable RWLock              mLock;

    mutable SpinLock            mInstanceLock;

    // ********************************************************************
    // The number of assets that reference this type (weak relationship)
    // ********************************************************************
    volatile Atomic32           mWeakReferences;
    // ********************************************************************
    // The number of assets that reference this type (strong relationship)
    // ********************************************************************
    volatile Atomic32           mStrongReferences;

    mutable volatile Atomic32   mRefs;
};

} // namespace lf

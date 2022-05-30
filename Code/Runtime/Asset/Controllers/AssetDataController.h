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
#ifndef LF_RUNTIME_ASSET_DATABASE_H
#define LF_RUNTIME_ASSET_DATABASE_H

#include "Core/Common/API.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/Error.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/AssetIndex.h"
#include "Runtime/Asset/AssetTypeInfo.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "Runtime/Asset/CacheBlockType.h"

#include <algorithm>

namespace lf {

DECLARE_ATOMIC_PTR(AssetObject);
DECLARE_ATOMIC_WPTR(AssetObject);
DECLARE_PTR(AssetProcessor);
class AssetTypeMap;
class AssetPath;

class LF_RUNTIME_API AssetDataController
{
public:
    using WeakAssetInstanceArray = TVector<AssetObjectAtomicWPtr>;

    struct DynamicSort
    {
        bool operator()(const char* a, const char* b) const { return a < b; }
    };

    struct DynamicTuple
    {
        AssetTypeInfo          mType;
        AssetHandle            mHandle;
        WeakAssetInstanceArray mInstances;
    };

    using DynamicTable = TMap<const char*, DynamicTuple, DynamicSort>;
    using DynamicIterator = DynamicTable::iterator;
    using DynamicIDTable = TMap<UInt32, DynamicIterator>;
    using DynamicAliasTable = TMap<const char*, DynamicIterator>;

    struct QueryResult
    {
        QueryResult()
        : mType()
        , mHandle(nullptr)
        , mDynamicId()
        , mStaticId(INVALID)
        , mValid(false)
        {}
        operator bool() { return mValid; }
        AssetTypeInfoCPtr           mType;
        AssetHandle*                mHandle;
        DynamicIterator             mDynamicId;
        SizeT                       mStaticId;
        bool                        mValid;
    };

private:

    struct DomainContext
    {
        String                          mDomain;
    };
    using DomainContextPtr = TStrongPointer<DomainContext>;

public:
    AssetDataController();
    ~AssetDataController();

    void SetProcessors(const TVector<AssetProcessorPtr>& processors);

    bool LoadDomain(const String& domain, const AssetTypeMap& cachedTypes);
    bool UnloadDomain(const String& domain);
    bool WriteDomain(const String& domain, AssetTypeMap& typeMap);

    void Update();

    // Shutdown()
    //
    //
    // 
    // CreatePrototype : AssetHandle
    // CreateInstance  : AssetObject
    // FindByName      : QueryResult
    // FindByID        : QueryResult
    // 
    // GetInstances

    // ********************************************************************
    // Creates AssetTypeInfo from the 'assetName'
    // @threadsafe
    // ********************************************************************
    QueryResult CreateType(const AssetPath& assetName, const Type* concreteType, const AssetTypeInfo* parent);
    // ********************************************************************
    // Find an asset from the name
    // @threadsafe
    // ********************************************************************
    QueryResult Find(const AssetPath& assetName, bool includeDeleted = false) const;
    // ********************************************************************
    // Find an asset type for the given concrete type
    // @threadsafe
    // ********************************************************************
    AssetTypeInfoCPtr Find(const Type* concreteType) const;

    bool UpdateCacheIndex(const AssetTypeInfo* assetType, const CacheIndex& index);
    // [DEPRECATED] Call this to attempt to set the current op on an asset type
    //    Should this call fail and return false the caller should not modify the load state.
    // 
    bool SetOp(const AssetTypeInfo* assetType, AssetOpState::Value value);
    // [DEPRECATED]
    bool ClearOp(const AssetTypeInfoCPtr& assetType);
    // ** Call this to set the load state of an asset type.
    //  note: This method is thread safe assuming you have acquired the write lock on the asset type.
    bool SetLoadState(const AssetTypeInfo* assetType, AssetLoadState::Value value);
    // **
    //
    bool AddDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency);
    // **
    //
    bool RemoveDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency);
    
    template<typename T>
    bool AddDependency(const TAsset<T>& assetType, const AssetTypeInfo* dependant)
    {
        return AddDependency(assetType.GetType(), dependant, false);
    }
    template<typename T>
    bool AddDependency(const TAssetType<T>& assetType, const AssetTypeInfo* dependant)
    {
        return AddDependency(assetType.GetType(), dependant, true);
    }
    template<typename T>
    bool RemoveDependency(const TAsset<T>& assetType, const AssetTypeInfo* dependant)
    {
        return RemoveDependency(assetType.GetType(), dependant, false);
    }
    template<typename T>
    bool RemoveDependency(const TAssetType<T>& assetType, const AssetTypeInfo* dependant)
    {
        return RemoveDependency(assetType.GetType(), dependant, true);
    }

    // ********************************************************************
    // ** Call this method to create the prototype of an asset if it has not already been created.
    // 
    // @threadsafe - This method is thread safe assuming you have acquired the write lock on the asset type.
    // ********************************************************************
    APIResult<bool> CreatePrototype(const AssetTypeInfo* assetType, AssetHandle*& handle);
    // ********************************************************************
    // ** Call this method to create the prototype of an asset if it has not already been created.
    // 
    // @threadsafe - This method is thread safe assuming you have acquired the write lock on the asset type.
    // ********************************************************************
    APIResult<bool> UnloadPrototype(const AssetTypeInfo* assetType);

    bool DeleteType(const AssetTypeInfo* assetType);

    AssetObjectAtomicPtr CreateInstance(const AssetTypeInfo* assetType);

    AssetHandle* GetHandle(const AssetTypeInfo* assetType);

    // ********************************************************************
    // Retrieves a AssetProcessor based off the concrete 'TargetType'
    // 
    // NOTE: This is usually used for runtime type creation/manipulation.
    // ********************************************************************
    AssetProcessor* GetProcessor(const AssetTypeInfo* assetType);
    AssetProcessor* GetProcessor(const Type* concreteType);
    AssetProcessor* GetProcessor(const AssetPath& path);
    // ********************************************************************
    // Retrieves a AssetProcessor based off the cache block type.
    // 
    // NOTE: This is usually used for import/exports where we know the filepath
    // and extension.
    // ********************************************************************
    AssetProcessor* GetProcessor(CacheBlockType::Value cacheBlock);
     
    void UpdateType(const AssetTypeInfo* assetType, const AssetHash* hash, const DateTime* modifyDate, bool& outUpdate);
    void UpdateInstances(const AssetTypeInfo* assetType, AssetObject* sourceObject);

    bool HasDomain(const String& domain);
    TVector<String> GetDomains();
    TVector<AssetTypeInfoCPtr> GetTypes(const String& domain);
    TVector<AssetTypeInfoCPtr> GetTypes(const Type* concreteType);
private:
    AssetDataController(const AssetDataController&) = delete;
    AssetDataController& operator=(const AssetDataController&) = delete;

    AssetTypeInfoCPtr FindConcrete(const Type* concreteType) const;

    DomainContext* FindDomain(const String& domain);
    const DomainContext* FindDomain(const String& domain) const;
    UInt32 GenerateUID(DomainContext* domain);

    void ReleaseDomainContext(DomainContext* context);

    void CollectGarbage(AssetTypeInfo& assetType);

    TVector<DomainContextPtr> mDomainContexts;

    DynamicTable        mTable;
    DynamicIDTable      mIDTable;
    DynamicAliasTable   mAliasTable;

    TVector<AssetProcessorPtr> mProcessors;
    // ********************************************************************
    // The lock for executing different functions of the Data Controller.
    // ********************************************************************
    mutable RWSpinLock mLock;
};

} // namespace lf

#endif // LF_RUNTIME_ASSET_DATABASE_H
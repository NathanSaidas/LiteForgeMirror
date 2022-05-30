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
#include "Core/Common/API.h"
#include "Core/String/String.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/Thread.h"
#include "Core/Utility/Time.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

class AssetTypeMap;
class AssetPath;
struct AssetOpDependencyContext;
DECLARE_PTR(AssetCacheController);
DECLARE_PTR(AssetDataController);
DECLARE_PTR(AssetSourceController);
DECLARE_PTR(AssetOpController);
DECLARE_PTR(AssetProcessor);
DECLARE_ATOMIC_WPTR(AssetOp);
DECLARE_ATOMIC_PTR(AssetObject);
DECLARE_MANAGED_CPTR(AssetTypeInfo);

// 'Load Target'                -- The load target is configured during initialization. By default the load target is the 
//                                 cache however it's possible to configure the load target as the source.
//
// 'mass import'
// 
// ImportDirectory( path, recursive )
// 
// Which domain is this directory in?
// 
// Scan the directory and build a list of 'files' (full path)
// Convert full path to Asset Path
// 

struct LF_RUNTIME_API AssetMgrInitializeData
{
    AssetMgrInitializeData();
    ~AssetMgrInitializeData();

    AssetMgrInitializeData(const AssetMgrInitializeData&) = delete;
    AssetMgrInitializeData& operator=(const AssetMgrInitializeData&) = delete;

    // ********************************************************************
    // A collection of asset processors to process asset data/requests/events
    // 
    // @default({DefaultAssetProcessor}), static
    // ********************************************************************
    TVector<AssetProcessorPtr> mProcessors;
    // ********************************************************************
    // A flag determining if the AssetMgr should become global. (eg Accessable 
    // via GetAssetMgr())
    // 
    // @default({false}), static
    // ********************************************************************
    bool mIsGlobal;
};

class LF_RUNTIME_API AssetMgr
{
public:
    AssetMgr();
    ~AssetMgr();

    bool SetGlobal();
    bool Initialize(const String& contentSourcePath, const String& contentCachePath, bool enableCache, AssetMgrInitializeData* initData = nullptr);
    void Update();
    void Shutdown();

    // ** Load the specified asset
    // AssetOpAtomicWPtr Load(const AssetPath& assetPath, UnknownAssetHandle*& unknownHandle, const Type* requiredType, AssetLoadFlags::Value flags);

    AssetOpAtomicWPtr Load(const AssetTypeInfo* type, AssetLoadFlags::Value flags);
    // ** Unload the specified asset
    // void Unload();
    // ** Create a new asset from concrete type
    AssetOpAtomicWPtr Create(const AssetPath& assetPath, const AssetObject* object, const AssetTypeInfo* parent);
    // ** Import an existing asset assuming a concrete type
    AssetOpAtomicWPtr Import(const AssetPath& assetPath);
    // ** Delete an asset (moving it to tmp)
    AssetOpAtomicWPtr Delete(const AssetTypeInfo* type);

    AssetOpAtomicWPtr CreateDomain(const AssetPath& assetPath);

    bool Wait(AssetOp* op);

    // ** Create a temporary object that is editable. This can be passed into the 'Create' function as the object argument.
    template<typename T>
    TAtomicStrongPointer<T> CreateEditable() const
    {
        LF_STATIC_IS_A(T, AssetObject);
        TAtomicStrongPointer<T> editableObject = MakeConvertibleAtomicPtr<T>();
        editableObject->SetType(typeof(T));
        return editableObject;
    }

    // // ** Remove reference to an asset, deleting it from cache memory
    // void Remove();
    // // ** Update the properties of an existing asset
    // void Update();
    // // ** Reload the asset and push the update out to any instances
    // void Refresh();
    // // ** Search for an asset type with the name
    // void FindType();
    // // ** Get the properties of an asset type
    // void GetProperties();
    // // ** Get all the instances of an asset type
    // void GetInstances();
    // // ** Get the prototype of an asset type
    // void GetPrototype();

    // ** Called upon by the TAsset/TAssetType classes to manage their internal asset handle pointers, avoid calling directly.
    // 
    //  This method releases the 'handle' passed in (if valid) and then acquires a pointer to the handle of the type at 'path'
    //  after that it will begin a LoadOperation.
    // 
    //  Assets that acquire a handle should not be assumed to be loaded before use! Use the IsLoaded method to determine whether
    //  or not an asset is loaded.
    void AcquireStrong(UnknownAssetHandle*& handle, const AssetPath& path, const Type* requiredType, AssetLoadFlags::Value flags);
    void AcquireStrong(UnknownAssetHandle*& handle, const AssetTypeInfo* type, const Type* requiredType, AssetLoadFlags::Value flags);
    // ** Called upon by the TAsset/TAssetType classes to manage their internal asset handle pointers, avoid calling directly.
    //
    //  This method releases the 'handle' passed in (if valid) and then acquires a pointer to the 'null' handle held by the AssetMgr
    void AcquireStrongNull(UnknownAssetHandle*& handle);
    // ** Called upon by the TAsset/TAssetType classes to manage their internal asset handle pointers, avoid calling directly.
    // 
    //  This method releases the 'handle' passed in (if valid) and then acquires a pointer to the handle of the type at 'path'
    void AcquireWeak(UnknownAssetHandle*& handle, const AssetPath& path, const Type* requiredType);
    void AcquireWeak(UnknownAssetHandle*& handle, const AssetTypeInfo* type, const Type* requiredType);
    // ** Called upon by the TAsset/TAssetType classes to manage their internal asset handle pointers, avoid calling directly.
    //
    //  This method releases the 'handle' passed in (if valid) and then acquires a pointer to the 'null' handle held by the AssetMgr
    void AcquireWeakNull(UnknownAssetHandle*& handle);
    // ** Called upon by the TAsset/TAssetType classes to manage their internal asset handle pointers, avoid calling directly.
    //
    //  This method releases the 'handle' passed in.
    void ReleaseStrong(UnknownAssetHandle*& handle);
    // ** Called upon by the TAsset/TAssetType classes to manage their internal asset handle pointers, avoid calling directly.
    //
    //  This method releases the 'handle' passed in.
    void ReleaseWeak(UnknownAssetHandle*& handle);
    // ** Called upon by the TAsset/TAssetType classes to manage their internal asset handle pointers, avoid calling directly.
    //
    //  This method does a comparison to check if it represents the 'null' handle.
    bool IsNull(const UnknownAssetHandle* handle);


    AssetObjectAtomicPtr CreateAssetInstance(const AssetTypeInfo* type);
    

    template<typename T>
    TAtomicStrongPointer<T> CreateInstance(const AssetTypeInfo* type)
    {
        LF_STATIC_IS_A(T, AssetObject);
        if (!type || !type->GetConcreteType()->IsA(typeof(T)))
        {
            return NULL_PTR;
        }
        return StaticCast<TAtomicStrongPointer<T>>(CreateAssetInstance(type));
    }

    // ** Call this method to start an operation to flush changes to domain type maps.
    AssetOpAtomicWPtr SaveDomain(const String& domain);
    // ** Call this method to start an operation to flush changes to domain cache.
    AssetOpAtomicWPtr SaveDomainCache(const String& domain);
    // ********************************************************************
    // ********************************************************************
    AssetOpAtomicWPtr UpdateCacheData(const AssetTypeInfo* type);


    AssetTypeInfoCPtr FindType(const AssetPath& path);
    bool GetSourceData(const AssetPath& path, MemoryBuffer& buffer);
    bool GetSourceData(const AssetTypeInfo* type, MemoryBuffer& buffer);
    bool GetCacheData(const AssetTypeInfo* type, MemoryBuffer& buffer);
    // Returns the fully qualified path of an asset path
    String GetFullPath(const AssetPath& path);
    // Returns the asset paths for source assets not imported. (Recursively)
    // @param path=The root path to check for source assets eg 'engine//'
    TVector<AssetPath> GetSourcePaths(const AssetPath& path);


    TVector<AssetTypeInfoCPtr> GetTypes(const String& domain);
    // ********************************************************************
    // Returns all the asset types that 'IsA' concreteType.
    // Ignores deleted/corrupted asset types.
    //
    // @threadsafe
    // ********************************************************************
    TVector<AssetTypeInfoCPtr> GetTypes(const Type* concreteType);

    void CacheControllerUpdate();
    void CacheControllerValidate();

    bool QuerySourceInfo(const AssetPath& path, const AssetInfoQuery& query, AssetInfoQueryResult& result);

    void UpdateType(const AssetTypeInfo* type, const AssetHash* hash, const DateTime* modifyDate);
    void UpdateInstances(const AssetTypeInfo* assetType, AssetObject* sourceObject);

    // **
    //
    bool AddDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency);
    // **
    //
    bool RemoveDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency);

private:
    AssetMgr(const AssetMgr&) = delete;
    AssetMgr& operator=(const AssetMgr&) = delete;

    void LoadDomain(const String& domain);
    void SaveDomain(const String& domain, AssetTypeMap& typeMap);
    void UnloadDomain(const String& domain);

    AssetOpDependencyContext GetOpDependencyContext();

    String mContentSourcePath;
    String mContentCachePath;
    bool   mCacheEnabled;

    AssetHandle mNullHandle;

    AssetCacheControllerPtr     mCacheController;
    AssetDataControllerPtr      mDataController;
    AssetSourceControllerPtr    mSourceController;
    AssetOpControllerPtr        mOpController;


    Float32                     mSourceToCacheUpdateTime;
    Timer                       mSourceToCacheUpdateTimer;
    TVector<AssetOpAtomicWPtr>   mSourceToCacheUpdates;
};

LF_RUNTIME_API AssetMgr& GetAssetMgr();

} // namespace lf

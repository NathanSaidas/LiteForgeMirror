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
#include "Runtime/PCH.h"
#include "AssetMgr.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"
#include "Core/Reflection/Type.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetTypeMap.h"
#include "Runtime/Asset/DefaultAssetProcessor.h"
// Controllers:
#include "Runtime/Asset/Controllers/AssetCacheController.h"
#include "Runtime/Asset/Controllers/AssetDataController.h"
#include "Runtime/Asset/Controllers/AssetSourceController.h"
#include "Runtime/Asset/Controllers/AssetOpController.h"
// Ops:
#include "Runtime/Asset/Ops/AssetCreateOp.h"
#include "Runtime/Asset/Ops/AssetDeleteOp.h"
#include "Runtime/Asset/Ops/AssetImportOp.h"
#include "Runtime/Asset/Ops/AssetLoadOp.h"
#include "Runtime/Asset/Ops/AssetRemoveOp.h"
#include "Runtime/Asset/Ops/AssetUpdateOp.h"
#include "Runtime/Asset/Ops/CreateDomainOp.h"
#include "Runtime/Asset/Ops/SaveDomainOp.h"
#include "Runtime/Asset/Ops/UpdateCacheOp.h"

namespace lf {

static AssetMgr* sInstance = nullptr;


AssetMgr& GetAssetMgr()
{
    return *sInstance;
}

AssetMgrInitializeData::AssetMgrInitializeData()
: mProcessors()
, mIsGlobal(false)
{}
AssetMgrInitializeData::~AssetMgrInitializeData()
{}

AssetMgr::AssetMgr()
: mContentSourcePath()
, mContentCachePath()
, mCacheEnabled(false)
, mNullHandle()
, mCacheController()
, mDataController()
, mSourceController()
, mOpController()
, mSourceToCacheUpdateTime(5.0f)
, mSourceToCacheUpdateTimer()
, mSourceToCacheUpdates()
{
    mNullHandle.mStrongRefs = 1;
}

AssetMgr::~AssetMgr()
{
    mOpController = NULL_PTR;
}

bool AssetMgr::SetGlobal()
{
    if (sInstance)
    {
        return false;
    }
    sInstance = this;
    return true;
}

bool AssetMgr::Initialize(const String& contentSourcePath, const String& contentCachePath, bool enableCache, AssetMgrInitializeData* initData)
{
    mContentSourcePath = contentSourcePath;
    mContentCachePath = contentCachePath;
    mCacheEnabled = enableCache;
    if (!FileSystem::PathExists(mContentSourcePath))
    {
        if (!mContentSourcePath.Empty())
        {
            gSysLog.Error(LogMessage("AssetMgr::Initialize failed, could not find ContentSourcePath. ") << mContentSourcePath);
        }
        return false;
    }

    if (!FileSystem::PathExists(mContentCachePath))
    {
        if (mContentCachePath.Empty())
        {
            gSysLog.Error(LogMessage("AssetMgr::Initialize failed, could not find ContentCachePath and the path was empty."));
            return false;
        }
        
        if (!FileSystem::PathCreate(mContentCachePath))
        {
            gSysLog.Error(LogMessage("AssetMgr::Initialize failed, could not create ContentCachePath. ") << mContentCachePath);
            return false;
        }
    }

    // Defaults:
    AssetMgrInitializeData defaultInitData;
    if (!initData)
    {
        initData = &defaultInitData;
        initData->mProcessors.push_back(AssetProcessorPtr(LFNew<DefaultAssetProcessor>()));
    }

    Timer loadTimer;

    mDataController = AssetDataControllerPtr(LFNew<AssetDataController>());
    mCacheController = AssetCacheControllerPtr(LFNew<AssetCacheController>());
    mSourceController = AssetSourceControllerPtr(LFNew<AssetSourceController>());
    mOpController = AssetOpControllerPtr(LFNew<AssetOpController>());

    AssetDependencyContext dependencies;
    dependencies.mCacheController = mCacheController;
    dependencies.mDataController = mDataController;
    dependencies.mSourceController = mSourceController;
    dependencies.mOpController = mOpController;

    // Controller Initialization:
    mOpController->Initialize();
    for (AssetProcessor* processor : initData->mProcessors)
    {
        processor->Initialize(dependencies);
    }
    mDataController->SetProcessors(initData->mProcessors);

    // todo: Modding support, we'll need to aggregate asset type maps for every mod.
    loadTimer.Start();
    LoadDomain("engine");
    loadTimer.Stop();
    gSysLog.Info(LogMessage("AssetMgr initialized data controller in ") << loadTimer.GetDelta() << " seconds...");

    String modsDir = FileSystem::PathJoin(mContentCachePath, "Mods\\");
    TVector<String> mods;
    FileSystem::GetDirectories(modsDir, mods);

    for (const String& mod : mods)
    {
        loadTimer.Start();
        LoadDomain(mod);
        loadTimer.Stop();
        gSysLog.Info(LogMessage("AssetMgr initialized mod ") << mod << " in " << loadTimer.GetDelta() << " seconds...");
    }


    // load the 'typemaps'
    // Asset Name => { CacheIndex, ConcreteType, Flags }

    // TODO:
    // mUpdateThread.Fork()

    return true;
}

void AssetMgr::Update()
{
    // Cache Controller Update:
    CacheControllerUpdate();
    mOpController->Update();
    mDataController->Update();
}

void AssetMgr::Shutdown()
{
    // todo: Lets figure out how to save the 'Data Controller'

    mOpController->Shutdown();

    const String engineDomain("engine");
    TVector<String> domains = mDataController->GetDomains();
    for (const String& domain : domains)
    {
        if (domain != engineDomain)
        {
            UnloadDomain(domain);
        }
    }
    
    UnloadDomain(engineDomain);

    if (sInstance == this)
    {
        sInstance = nullptr;
    }
}

AssetOpAtomicWPtr AssetMgr::Load(const AssetTypeInfo* type, AssetLoadFlags::Value flags)
{
    AssetOpDependencyContext context = GetOpDependencyContext();
    if (!mCacheEnabled)
    {
        flags |= AssetLoadFlags::LF_SOURCE;
    }

    // @!!!RACE CONDITION!!!
    // If the data is unloaded 'AssetDataController::UnloadPrototype' then we'll need to load it now!
    // If the data is not unloaded, then it's safe to return

    AssetHandle* handle = mDataController->GetHandle(type);
    Assert(handle);
    AtomicIncrement32(&handle->mStrongRefs);

    const AssetLoadState::Value loadState = type->GetLoadState();
    if ((flags & AssetLoadFlags::LF_RECURSIVE_PROPERTIES) > 0)
    {
        if (loadState == AssetLoadState::ALS_LOADED)
        {
            AtomicDecrement32(&handle->mStrongRefs);
            return mOpController->GetCompleted();
        }
    }
    else if ((flags & AssetLoadFlags::LF_IMMEDIATE_PROPERTIES) > 0)
    {
        if (loadState == AssetLoadState::ALS_SERIALIZED_PROPERTIES)
        {
            AtomicDecrement32(&handle->mStrongRefs);
            return mOpController->GetCompleted();
        }
    }
    else if((flags & AssetLoadFlags::LF_ACQUIRE) == 0)
    {
        if (loadState == AssetLoadState::ALS_CREATED)
        {
            AtomicDecrement32(&handle->mStrongRefs);
            return mOpController->GetCompleted();
        }
    }

    AtomicDecrement32(&handle->mStrongRefs);

    const bool loadCache = (flags & AssetLoadFlags::LF_SOURCE) == 0;

    auto op = MakeConvertibleAtomicPtr<AssetLoadOp>(AssetTypeInfoCPtr(type), flags, loadCache, context);
    op->Start();

    const bool async = (flags & AssetLoadFlags::LF_ASYNC) > 0;
    if (!async)
    {
        Wait(op);
    }
    return op;

    // So at the asset mgr level we have a few options:
    //
    // Developer:
    //      Assets can be loaded from the source file. (There can even be a config option to prefer it over cache)
    //      Assets can be loaded from cache.
    //      Assets will never be updated from a content server.
    // Modder:
    //      Assets in the modder 'domain' can be loaded from source file.
    //      Assets can be loaded from cache.
    //      Assets will never be updated from a content server.
    //
    // Retail:
    //      Assets can never be loaded from source.
    //      Assets are always loaded from cache
    //      Game Assets may be updated by a content server.
    //      Modded Assets will never be updated by content server.
    //

    // In addition to the asset mgr having certain loading constraints an asset has different 
    // load states in conjunction with operations.
    //
    // Unavailable
    //      An asset is unavailable if it does not exist or if there was an error retrieving it.
    // Deleted
    //      An asset may be deleted by the editor or content server
    // Unloaded
    //      An asset typically starts off in the unloaded state
    // LoadedImmediateProperties
    //      An asset has it's immediate properties loaded.
    // LoadedRecursiveProperties
    //      An asset has it's dependencies loaded (at least for properties)
    // LoadedRuntime
    //      An asset has created any runtime required resources.

    // 
    // Operations Available:
    // Load(ImmediateProperties | [Acquire])
    //      The asset will load only the immediate properties (children will not be loaded) 
    //      optionally only just acquire the properties (ie don't do any loading if not loaded)
    // Load(RecursiveProperties | [Acquire])
    //      The asset will load all the properties and scheduler the dependencies to also load.
    // Load(Runtime | [Acquire])
    //      The asset will do the same loading as RecursiveProperties and also register/initialize
    //      any runtime resources.
    // UpdateData()
    //      The asset will have it's cache data updated, then the prototype will be updated,
    //      the all the instances will be updated. (Caching data only occurs if caching is enabled)
    // CacheContent()
    //      The asset will save it's data in the cache. (If caching is enabled)
    // DeleteCacheContent()
    //      The asset will delete its cache content.
    // DeleteSourceContent()
    // 

    // if (!type || type->IsDeleted())
    // {
    //     return;
    // }
    // 
    // if (!config.load_from_source)
    // {
    //     cacheLocator->Read(type)
    //         .Then([]() {
    //             InitializeFromMemory(type, buffer);
    //         })
    //         .Error([]() {
    //             sourceLocator->Read(type)
    //             .Then([]() {
    // 
    //             });
    //         });
    // }

    // const AssetTypeInfo* assetType = mDataController->FindByName(assetPath.CStr());
    

    // if (!mCacheEnabled)
    // {
    //     mSourceController->GetData(assetType);
    //     mSourceController->GetDataAsync(assetType)
    //         .Then([]{
    //             LoadAsset(buffer, assetType);
    //             // Instantiate the concrete type
    //             // Serialize the concrete type with load flags.
    //             // 
    //         })
    //         .Error([]{
    // 
    //         });
    // }
    // else
    // {
    // 
    // }
    // 
    // const AssetCacheInfo* cacheInfo = mCacheController->GetCacheInfo(assetType);
    // if (!cacheInfo)
    // {
    //     mCommandController->CacheContent(assetType);
    // }

}

AssetOpAtomicWPtr AssetMgr::Create(const AssetPath& assetPath, const AssetObject* object, const AssetTypeInfo* parent)
{
    AssetOpDependencyContext context = GetOpDependencyContext();
    auto op = MakeConvertibleAtomicPtr<AssetCreateOp>(assetPath, object, AssetTypeInfoCPtr(parent), context);
    op->Start();
    return op;
}

AssetOpAtomicWPtr AssetMgr::Import(const AssetPath& assetPath)
{
    const bool allowRawData = false;
    AssetOpDependencyContext context = GetOpDependencyContext();
    auto op = MakeConvertibleAtomicPtr<AssetImportOp>(assetPath, allowRawData, context);
    op->Start();

    return op;
}

AssetOpAtomicWPtr AssetMgr::Delete(const AssetTypeInfo* type)
{
    AssetOpDependencyContext context = GetOpDependencyContext();
    auto op = MakeConvertibleAtomicPtr<AssetDeleteOp>(AssetTypeInfoCPtr(type), context);
    op->Start();
    return op;
}

AssetOpAtomicWPtr AssetMgr::CreateDomain(const AssetPath& assetPath)
{
    AssetOpDependencyContext context = GetOpDependencyContext();
    auto op = MakeConvertibleAtomicPtr<CreateDomainOp>(assetPath, mContentCachePath, mContentSourcePath, context);
    op->Start();
    return op;
}

bool AssetMgr::Wait(AssetOp* op)
{
    AssetOpAtomicPtr pinned = GetAtomicPointer(op);
    // TODO: Replace with 'Update Thread' or proper thread tags...
    if (IsMainThread())
    {
        while (!op->IsComplete())
        {
            Update();
        }
    }
    else
    {
        // This is bad and can deadlock if called from non-main thread.
        while(!op->IsComplete()) { }
    }
    if (pinned->IsFailed())
    {
        gSysLog.Error(LogMessage("Failed asset operation: ") << pinned->GetFailReason());
    }

    return pinned->IsSuccess();
}

void AssetMgr::AcquireStrong(UnknownAssetHandle*& unknownHandle, const AssetPath& path, const Type* requiredType, AssetLoadFlags::Value flags)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mStrongRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }

    // Query
    AssetDataController::QueryResult result = mDataController->Find(path);
    if (!result)
    {
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mStrongRefs);
        return;
    }

    // Validate type
    if (!result.mType->GetConcreteType()->IsA(requiredType))
    {
        ReportBugMsg("Failed to load asset handle. The result type is not related to the requried type.");
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mStrongRefs);
        return;
    }

    // Inc-Ref and Load
    unknownHandle = result.mHandle;
    AtomicIncrement32(&result.mHandle->mStrongRefs);

    // Strong loads may specify they want to acquire the type (in which case they will not load!)
    const bool acquire = (flags & AssetLoadFlags::LF_ACQUIRE) > 0;
    if (!acquire)
    {
        Load(result.mType, flags);
    }
}
void AssetMgr::AcquireStrong(UnknownAssetHandle*& unknownHandle, const AssetTypeInfo* type, const Type* requiredType, AssetLoadFlags::Value flags)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mStrongRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }

    handle = mDataController->GetHandle(type);
    if (!handle)
    {
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mStrongRefs);
        return;
    }

    if (!type->GetConcreteType()->IsA(requiredType))
    {
        ReportBugMsg("Failed to load asset handle. The result type is not related to the requried type.");
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mStrongRefs);
        return;
    }

    unknownHandle = handle;
    AtomicIncrement32(&handle->mStrongRefs);


    // Strong loads may specify they want to acquire the type (in which case they will not load!)
    const bool acquire = (flags & AssetLoadFlags::LF_ACQUIRE) > 0;
    if (!acquire)
    {
        Load(type, flags);
    }
}
void AssetMgr::AcquireStrongNull(UnknownAssetHandle*& unknownHandle)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mStrongRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }
    unknownHandle = &mNullHandle;
    AtomicIncrement32(&mNullHandle.mStrongRefs);
}
void AssetMgr::AcquireWeak(UnknownAssetHandle*& unknownHandle, const AssetPath& path, const Type* requiredType)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mWeakRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }

    // Query
    AssetDataController::QueryResult result = mDataController->Find(path);
    if (!result)
    {
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mWeakRefs);
        return;
    }

    // Validate type
    if (!result.mType->GetConcreteType()->IsA(requiredType))
    {
        ReportBugMsg("Failed to load asset handle. The result type is not related to the requried type.");
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mWeakRefs);
        return;
    }

    // Inc-Ref and Load
    unknownHandle = result.mHandle;
    AtomicIncrement32(&result.mHandle->mWeakRefs);
}
void AssetMgr::AcquireWeak(UnknownAssetHandle*& unknownHandle, const AssetTypeInfo* type, const Type* requiredType)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mWeakRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }

    handle = mDataController->GetHandle(type);
    if (!handle)
    {
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mWeakRefs);
        return;
    }

    if (!type->GetConcreteType()->IsA(requiredType))
    {
        ReportBugMsg("Failed to load asset handle. The result type is not related to the requried type.");
        unknownHandle = &mNullHandle;
        AtomicIncrement32(&mNullHandle.mWeakRefs);
        return;
    }

    unknownHandle = handle;
    AtomicIncrement32(&handle->mWeakRefs);
}
void AssetMgr::AcquireWeakNull(UnknownAssetHandle*& unknownHandle)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mWeakRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }
    unknownHandle = &mNullHandle;
    AtomicIncrement32(&mNullHandle.mWeakRefs);
}
void AssetMgr::ReleaseStrong(UnknownAssetHandle*& unknownHandle)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mStrongRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }
}
void AssetMgr::ReleaseWeak(UnknownAssetHandle*& unknownHandle)
{
    AssetHandle* handle = static_cast<AssetHandle*>(unknownHandle);
    // Release:
    if (handle)
    {
        AtomicDecrement32(&handle->mWeakRefs);
        unknownHandle = nullptr;
        handle = nullptr;
    }
}
bool AssetMgr::IsNull(const UnknownAssetHandle* handle)
{
    return handle == &mNullHandle;
}

AssetObjectAtomicPtr AssetMgr::CreateAssetInstance(const AssetTypeInfo* type)
{
    return mDataController->CreateInstance(type);
}

AssetOpAtomicWPtr AssetMgr::SaveDomain(const String& domain)
{
    AssetOpDependencyContext context = GetOpDependencyContext();
    auto op = MakeConvertibleAtomicPtr<SaveDomainOp>(domain, mContentCachePath, context);
    op->Start();
    return op;
}

AssetOpAtomicWPtr AssetMgr::SaveDomainCache(const String& domain)
{
    AssetOpDependencyContext context = GetOpDependencyContext();
    auto op = MakeConvertibleAtomicPtr<SaveDomainCacheOp>(domain, context);
    op->Start();
    return op;
}
AssetOpAtomicWPtr AssetMgr::UpdateCacheData(const AssetTypeInfo* type)
{
    if (!mCacheEnabled)
    {
        return NULL_PTR;
    }

    AssetOpDependencyContext context = GetOpDependencyContext();
    auto op = MakeConvertibleAtomicPtr<UpdateCacheOp>(AssetTypeInfoCPtr(type), context);
    op->Start();
    return op;
}

void AssetMgr::LoadDomain(const String& domain)
{
    String path;
    String cacheDir;
    String sourceDir;

    // TODO [Nathan] When we start loading actual mods we'll need to parse modinfo.json for the typemap format. 'TypeMapFormat': [Json|Binary]
    if (StrToLower(domain) == "engine")
    {
        path = FileSystem::PathJoin(FileSystem::PathJoin(mContentCachePath, "Content"), "cache.typemap");
        cacheDir = FileSystem::PathJoin(mContentCachePath, "Content");
        sourceDir = mContentSourcePath;
    }
    else
    {
        path = FileSystem::PathJoin(FileSystem::PathJoin(mContentCachePath, "Mods\\" + domain), "modinfo.typemap");
        cacheDir = FileSystem::PathJoin(mContentCachePath, "Mods\\" + domain);
        sourceDir = FileSystem::PathJoin(mContentSourcePath, "Mods\\" + domain);
    }
    gSysLog.Info(LogMessage("Loading domain ") << domain << " : " << path << "...");
    AssetTypeMap typeMap;
    if (!typeMap.Read(AssetTypeMap::JSON, path))
    {
        gSysLog.Warning(LogMessage("Failed to load the domain... It must be rebuilt from source."));
    }
    mCacheController->AddDomain(domain, cacheDir);
    mSourceController->AddDomain(domain, sourceDir);
    mDataController->LoadDomain(domain, typeMap);
}
void AssetMgr::SaveDomain(const String& domain, AssetTypeMap& typeMap)
{
    String path;
    if (StrToLower(domain) == "engine")
    {
        path = FileSystem::PathJoin(FileSystem::PathJoin(mContentCachePath, "Content"), "cache.typemap");
    }
    else
    {
        path = FileSystem::PathJoin(FileSystem::PathJoin(mContentCachePath, "Mods\\" + domain), "modinfo.typemap");
    }
    gSysLog.Info(LogMessage("Saving domain ") << domain << " : " << path << "...");
    typeMap.Write(AssetTypeMap::JSON, path);
}

void AssetMgr::UnloadDomain(const String& domain)
{
    AssetTypeMap typeMap;
    if (mDataController->WriteDomain(domain, typeMap))
    {
        SaveDomain(domain, typeMap);
    }

    mSourceController->RemoveDomain(domain);
    mCacheController->RemoveDomain(domain);
    mDataController->UnloadDomain(domain);
}

AssetOpDependencyContext AssetMgr::GetOpDependencyContext()
{
    AssetOpDependencyContext context;
    context.mDataController = mDataController;
    context.mSourceController = mSourceController;
    context.mCacheController = mCacheController;
    context.mOpController = mOpController;
    return context;
}

AssetTypeInfoCPtr AssetMgr::FindType(const AssetPath& path)
{
    auto result = mDataController->Find(path);
    if (result)
    {
        return result.mType;
    }
    return nullptr;
}

bool AssetMgr::GetSourceData(const AssetPath& path, MemoryBuffer& buffer)
{
    SizeT size;
    if (!mSourceController->QuerySize(path, size))
    {
        return false;
    }

    buffer.Allocate(size, 1);
    return mSourceController->Read(buffer, path);
}

bool AssetMgr::GetSourceData(const AssetTypeInfo* type, MemoryBuffer& buffer)
{
    SizeT size;
    if (!mSourceController->QuerySize(type->GetPath(), size))
    {
        return false;
    }
    buffer.Allocate(size, 1);
    return mSourceController->Read(buffer, type->GetPath());
}

bool AssetMgr::GetCacheData(const AssetTypeInfo* type, MemoryBuffer& buffer)
{
    if (!type)
    {
        return false;
    }

    SizeT size;
    if (!mCacheController->QuerySize(type, size))
    {
        return false;
    }

    buffer.Allocate(size, 1);
    CacheIndex index;
    return mCacheController->Read(buffer, type, index);
}

String AssetMgr::GetFullPath(const AssetPath& path)
{
    return mSourceController->GetFullPath(path);
}

TVector<AssetPath> AssetMgr::GetSourcePaths(const AssetPath& path)
{
    return mSourceController->GetSourcePaths(path);
}

TVector<AssetTypeInfoCPtr> AssetMgr::GetTypes(const String& domain)
{
    return mDataController->GetTypes(domain);
}
TVector<AssetTypeInfoCPtr> AssetMgr::GetTypes(const Type* concreteType)
{
    return mDataController->GetTypes(concreteType);
}

void AssetMgr::CacheControllerUpdate()
{
    if (!mSourceToCacheUpdates.empty())
    {
        return;
    }

    if (!mSourceToCacheUpdateTimer.IsRunning())
    {
        mSourceToCacheUpdateTimer.Start();
    }

    if (mSourceToCacheUpdateTimer.PeekDelta() < mSourceToCacheUpdateTime)
    {
        return;
    }
    mSourceToCacheUpdateTimer.Stop();
    mSourceToCacheUpdateTimer.Start();

    // foreach Domain
    // if(Domain->SourceEnabled())
    //
    // foreach Type 
    // if(Type->SourceModified != Type->CacheModified)
    //   UpdateAssetObject()

    TStackVector<const AssetTypeInfo*, 16> updateTypes;

    Timer t;
    t.Start();
    TVector<String> domains = mDataController->GetDomains();
    for (const String& domain : domains)
    {
        TVector<AssetTypeInfoCPtr> types = mDataController->GetTypes(domain);
        for (const AssetTypeInfo* type : types)
        {
            if (type->GetParent() == nullptr)
            {
                continue; // Skip the concrete types
            }

            AssetInfoQuery query;
            AssetInfoQueryResult sourceResult;
            AssetInfoQueryResult cacheResult;

            query.mHash = false;
            query.mModifyDate = true;
            if (mSourceController->QueryInfo(type->GetPath(), query, sourceResult)
             && mCacheController->QueryInfo(type, query, cacheResult)
            )
            {
                if (sourceResult.mModifyDate != cacheResult.mModifyDate)
                {
                    updateTypes.push_back(type);
                }
            }
        }
    }
    t.Stop();

    Float32 dt = ToMilliseconds(TimeTypes::Seconds(t.GetDelta())).mValue;

    gSysLog.Info(LogMessage("Types required to be updated... (") << dt << "ms)");
    for (const AssetTypeInfo* type : updateTypes)
    {
        gSysLog.Info(LogMessage("  ") << type->GetPath().CStr());
    }
}

void AssetMgr::CacheControllerValidate()
{

    TVector<String> domains = mDataController->GetDomains();
    for (const String& domain : domains)
    {
        TVector<AssetTypeInfoCPtr> types = mDataController->GetTypes(domain);
        for (const AssetTypeInfo* type : types)
        {
            if (type->GetParent() == nullptr)
            {
                continue;
            }

            CacheIndex index;
            CacheObject object;
            if (!mCacheController->FindIndex(type, index))
            {
                if (mCacheController->FindObject(type, object, index))
                {
                    gSysLog.Info(LogMessage("  ") << type->GetPath().CStr() << ", missing index but have object."); // Cache has been deleted, best to delete the object and 'update cache data'

                }
                else
                {

                    // Object just straight up doesn't exist in cache, just 'update cache data'
                    gSysLog.Info(LogMessage("  ") << type->GetPath().CStr() << ", missing index."); 
                }
            }
            else if (!mCacheController->FindObject(type, object, index))
            {
                gSysLog.Info(LogMessage("  ") << type->GetPath().CStr() << ", missing object."); // Object has index but no data, just 'update cache data'
            }
        }
    }
}

bool AssetMgr::QuerySourceInfo(const AssetPath& path, const AssetInfoQuery& query, AssetInfoQueryResult& result)
{
    return mSourceController->QueryInfo(path, query, result);
}

void AssetMgr::UpdateType(const AssetTypeInfo* type, const AssetHash* hash, const DateTime* modifyDate)
{
    bool updateCache = false;
    mDataController->UpdateType(type, hash, modifyDate, updateCache);

    if (updateCache)
    {

    }
}

void AssetMgr::UpdateInstances(const AssetTypeInfo* assetType, AssetObject* sourceObject)
{
    mDataController->UpdateInstances(assetType, sourceObject);
}

// **
//
bool AssetMgr::AddDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency)
{
    return mDataController->AddDependency(assetType, dependant, weakDependency);
}
// **
//
bool AssetMgr::RemoveDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency)
{
    return mDataController->RemoveDependency(assetType, dependant, weakDependency);
}

} // namespace lf
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
#include "Runtime/PCH.h"
#include "AssetDataController.h"
#include "Core/IO/BinaryStream.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/StandardError.h"
#include "Runtime/Asset/AssetCommon.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetTypeMap.h"
#include "Runtime/Asset/AssetProcessor.h"
#include "Runtime/Reflection/ReflectionMgr.h"

#include <algorithm>

namespace lf {

static void PopulateConcreteTypes(TVector<const Type*>& outConcreteTypes)
{
    ReflectionMgr& reflection = GetReflectionMgr();
    for (TypeIterator it = reflection.GetTypesBegin(), end = reflection.GetTypesEnd(); it != end; ++it)
    {
        const Type* type = &(*it);
        if (type->IsNative() || type->IsEnum())
        {
            continue;
        }

        if (!type->IsA(typeof(AssetObject)))
        {
            continue;
        }
        outConcreteTypes.push_back(type);
    }
}

static bool IsDeleted(const AssetTypeInfo* type)
{
    return type->GetLoadState() == AssetLoadState::ALS_DELETED;
}
static bool IsCorrupted(const AssetTypeInfo* type)
{
    return type->GetLoadState() == AssetLoadState::ALS_CORRUPTED;
}

class InvalidPrototypeTypeError : public StandardError
{
public:
    InvalidPrototypeTypeError(const AssetTypeInfo* assetType)
    {
        static const auto errorMessage = StaticString("AssetType failed to acquire concrete for prototype. Type=");
        const char* typePath = assetType->GetPath().CStr();
        const SizeT stringSize =
            errorMessage.Size()             // Error Message
            + assetType->GetPath().Size()   // Asset Type Name
            + 1;                            // Null Terminator
        PrintError(stringSize, "%s%s", errorMessage.CStr(), typePath);
    }
    static ErrorBase* Create(const ErrorBase::Info& info, const AssetTypeInfo* assetType)
    {
        return ErrorUtil::MakeError<InvalidPrototypeTypeError>(info, assetType);
    }
};

class OperationFailureCreatePrototype : public StandardError
{
public:
    OperationFailureCreatePrototype(const AssetTypeInfo* assetType)
    {
        CriticalAssert(assetType != nullptr);
        static const auto errorMessage = StaticString("AssetType failed to create prototype. Type=");
        const char* typePath = assetType->GetPath().CStr();
        const SizeT stringSize =
            errorMessage.Size()
            + assetType->GetPath().Size()
            + 1;
        PrintError(stringSize, "%s%s", errorMessage.CStr(), typePath);
    }
    static ErrorBase* Create(const ErrorBase::Info& info, const AssetTypeInfo* assetTypeName)
    {
        return ErrorUtil::MakeError<OperationFailureCreatePrototype>(info, assetTypeName);
    }
};

AssetDataController::AssetDataController()
: mDomainContexts()
, mTable()
, mIDTable()
, mProcessors()
, mLock()
{}
AssetDataController::~AssetDataController()
{}


void AssetDataController::SetProcessors(const TVector<AssetProcessorPtr>& processors)
{
    mProcessors = processors;
    for (AssetProcessor* processor : mProcessors)
    {
        CriticalAssert(processor != nullptr);
    }
}

bool AssetDataController::LoadDomain(const String& domain, const AssetTypeMap& cachedTypes)
{
    if (FindDomain(domain))
    {
        return false;
    }

    // #TODO [Nathan] We won't need to use StrToLower when we make AssetPath lower case or 'case-insensitive'
    const bool loadConcrete = StrCompareAgnostic(domain, "engine");
    ;
    DomainContextPtr context(LFNew<DomainContext>());
    context->mDomain = domain;
    
    // ** List of concrete types to load (only loaded for engine domain)
    TVector<const Type*> concreteTypes;

    if (loadConcrete)
    {
        // Get all the concrete AssetObject types
        PopulateConcreteTypes(concreteTypes);
    }

    if (loadConcrete)
    {
        // Build Concrete AssetTypes
        for (const Type* type : concreteTypes)
        {
            AssetPath fullname(AssetUtil::GetConcreteAssetName(type));
            Assert(!fullname.Empty());
            Assert(StrCompareAgnostic(fullname.GetDomain(), "engine"));

            auto pair = mTable.emplace(fullname.CStr(), DynamicTuple());

            // Should always insert new
            Assert(pair.second == true);
            DynamicTuple& tuple = pair.first->second;

            AssetTypeInfo& assetType = tuple.mType;
            assetType.mPath = fullname;
            assetType.mConcreteType = type;
            assetType.mParent = nullptr;
            // NOTE Concrete type do not have a UID because they are not needed to be saved in the cache.
            assetType.mCacheIndex = CacheIndex();

            // Concrete types are addressable by asset name or type name.
            mAliasTable.emplace(assetType.mPath.CStr(), pair.first);
            mAliasTable.emplace(type->GetFullName().CStr(), pair.first);
        }
    }


    // Build DataDriven AssetTypes
    for (const AssetTypeMapping& data : cachedTypes.GetTypes())
    {
        auto pair = mTable.emplace(data.mPath.CStr(), DynamicTuple());
        Assert(pair.second == true);

        DynamicTuple& tuple = pair.first->second;;

        AssetTypeInfo& assetType = tuple.mType;
        assetType.mPath.SetPath(data.mPath.CStr());
        if (assetType.mPath.Empty())
        {
            gSysLog.Warning(LogMessage("Failed to initialize AssetTypeInfo 'bad name'. Name=") << data.mPath << ", ConcreteType=" << data.mConcreteType);
            continue;
        }
        assetType.mConcreteType = GetReflectionMgr().FindType(data.mConcreteType);
        assetType.mParent = nullptr;
        assetType.mCacheIndex.mBlobID = data.mCacheBlobID;
        assetType.mCacheIndex.mObjectID = data.mCacheObjectID;
        assetType.mCacheIndex.mUID = data.mCacheUID;
        assetType.mWeakReferences = data.mWeakReferences;
        assetType.mStrongReferences = data.mStrongReferences;

        mAliasTable.emplace(assetType.mPath.CStr(), pair.first);
        Assert(Valid(data.mCacheUID));
        mIDTable.emplace(data.mCacheUID, pair.first);
    }

    // After Indices are built we can begin linking all the parent types.
    // todo: We can use promises or a task to link concurrently
    for (const AssetTypeMapping& data : cachedTypes.GetTypes())
    {
        // TODO: Mods cannot load data because they are in a different tables
        // ; We cannot reference other tables because we might reference ourself while we're building
        // ; It makes more sense to just have one big table.
        //
        // We separated to keep these 'static' tables
        // 
        // TMap<const char*, DynamicTuple, DynamicSort>     GlobalTable;
        // THashMap<UInt32, DynamicIterator>                IDTable;
        //
        // OnLoadDomain()
        // OnUnloadDomain()
        // OnLoadAsset()
        // OnUnloadAsset()
        //

        auto typeIterator = mAliasTable.find(data.mPath.CStr());
        auto parentIterator = data.mParent.Empty() ? mAliasTable.find(data.mConcreteType.CStr()) : mAliasTable.find(data.mParent.CStr());
        Assert(typeIterator != mAliasTable.end() && parentIterator != mAliasTable.end());
        DynamicTuple& typeTuple = typeIterator->second->second;
        DynamicTuple& parentTuple = parentIterator->second->second;
        typeTuple.mType.mParent = &parentTuple.mType;
    }

    for (auto& pair : mTable)
    {
        auto& type = pair.second.mType;
        type.mHandle = &pair.second.mHandle;
        type.mInstances = &pair.second.mInstances;
        type.mController = this;
        type.mHandle->mType = &type;
    }

    mDomainContexts.push_back(context);
    return true;
}

bool AssetDataController::UnloadDomain(const String& domain)
{
    if (domain.Empty())
    {
        return false;
    }

    if (StrToLower(domain) == "engine")
    {
        // We should only unload the engine domain after others have been unloaded.
        Assert(mDomainContexts.size() == 1);
    }

    // TODO [Nathan] We need to think about how to do this safely. (Maybe mark as garbage)
    //               because if we unload a domain context, all asset references will be
    //               pointing to memory that doesn't belong to them.
    for (auto it = mDomainContexts.begin(); it != mDomainContexts.end(); ++it)
    {
        if (StrCompareAgnostic((*it)->mDomain, domain))
        {
            ReleaseDomainContext(*it);
            mDomainContexts.swap_erase(it);
            return true;
        }
    }
    return false;
}

bool AssetDataController::WriteDomain(const String& domain, AssetTypeMap& typeMap)
{
    ScopeRWSpinLockRead lock(mLock);
    // TODO: [PERF] Iterations times can likely be improved if we iterate on an array of types inside the domain context

    for (const auto& pair : mTable)
    {
        const AssetTypeInfo& type = pair.second.mType;
        // If there is no parent, the type is concrete and should not be saved.
        if (type.GetParent() == nullptr)
        {
            continue;
        }

        if (type.GetLoadState() == AssetLoadState::ALS_DELETED)
        {
            continue;
        }

        if (!StrCompareAgnostic(type.GetPath().GetDomain(), domain))
        {
            continue;
        }

        AssetTypeMapping mapping;
        mapping.mPath = type.GetPath().AsToken();
        mapping.mConcreteType = type.GetConcreteType()->GetFullName();
        mapping.mParent = type.GetParent()->GetPath().AsToken();
        CacheIndex cacheIndex = type.GetCacheIndex();
        mapping.mCacheUID = cacheIndex.mUID;
        mapping.mCacheObjectID = cacheIndex.mObjectID;
        mapping.mCacheBlobID = cacheIndex.mBlobID;
        mapping.mWeakReferences = type.GetWeakReferences();
        mapping.mStrongReferences = type.GetStrongReferences();
        typeMap.GetTypes().push_back(mapping);
    }

    return true;
}

void AssetDataController::Update()
{
    ScopeRWSpinLockRead lock(mLock);

    // TODO: [PERF] We can break up the garbage collection into cycles... eg; Collect Garbage for first 100, next 100 on next frame etc.
    for (auto& pair : mTable)
    {
        CollectGarbage(pair.second.mType);
    }
}

AssetDataController::QueryResult AssetDataController::CreateType(const AssetPath& assetName, const Type* concreteType, const AssetTypeInfo* parent)
{
    if (concreteType == nullptr || !concreteType->IsA(typeof(AssetObject)))
    {
        ReportBugMsg("Invalid argument 'concreteType'.");
        return QueryResult();
    }

    String domain = assetName.GetDomain();
    if (assetName.Empty() || domain.Empty())
    {
        ReportBugMsg("Invalid argument 'assetName'.");
        return QueryResult();
    }

    ScopeRWSpinLockWrite lock(mLock);
    DomainContext* context = FindDomain(domain);
    if (!context)
    {
        return QueryResult(); // domain unavailable.
    }

    if (mAliasTable.find(assetName.CStr()) != mAliasTable.end())
    {
        return QueryResult();
    }

    DynamicIterator it = mTable.emplace(assetName.CStr(), DynamicTuple()).first;
    mAliasTable.emplace(assetName.CStr(), it);

    AssetHandle& handle = it->second.mHandle;
    AssetTypeInfo& type = it->second.mType;
    type.mPath = assetName;
    type.mConcreteType = concreteType;
    type.mParent = parent ? parent : FindConcrete(concreteType);
    type.mCacheIndex = CacheIndex();
    type.mCacheIndex.mUID = GenerateUID(context);
    Assert(Valid(type.mCacheIndex.mUID));
    mIDTable.emplace(type.mCacheIndex.mUID, it);

    // #TODO [Nathan]: If we are making the TAssetReference TAssetType compatible with smart pointers
    //                 how do we guarantee that the smart pointer won't try to delete the 'node'
    handle.mPrototype = nullptr;
    handle.mType = &type;
    handle.mWeakRefs = 1;
    handle.mStrongRefs = 0;

    QueryResult result;
    result.mDynamicId = it;
    result.mHandle = &handle;
    result.mType = &type;
    result.mValid = true;

    type.mHandle = &handle;
    type.mInstances = &it->second.mInstances;
    type.mController = this;
    return result;
}

AssetDataController::QueryResult AssetDataController::Find(const AssetPath& assetName, bool includeDeleted) const
{
    ScopeRWSpinLockRead lock(mLock);
    const DomainContext* context = FindDomain(assetName.GetDomain());
    if (!context)
    {
        return QueryResult();
    }

    // AssetPath correctPath(assetName);

    auto aliasIterator = mAliasTable.find(assetName.CStr());
    if (
        aliasIterator != mAliasTable.end()
        && (includeDeleted || aliasIterator->second->second.mType.GetLoadState() != AssetLoadState::ALS_DELETED)
    )
    {
        DynamicIterator it = aliasIterator->second;

        QueryResult result;
        result.mDynamicId = it;
        result.mType = &it->second.mType;
        result.mHandle = &it->second.mHandle;
        result.mValid = true;
        return result;
    }
    return QueryResult();
}

AssetTypeInfoCPtr AssetDataController::Find(const Type* concreteType) const
{
    ScopeRWSpinLockRead lock(mLock);
    return FindConcrete(concreteType);
}

bool AssetDataController::UpdateCacheIndex(const AssetTypeInfo* assetType, const CacheIndex& index)
{
    // TODO [Nathan] What can go wrong here? Do we need validation?
    Assert(Find(assetType->GetPath(), true));
    const_cast<AssetTypeInfo*>(assetType)->mCacheIndex = index;
    return true;
}

bool AssetDataController::SetOp(const AssetTypeInfo* assetType, AssetOpState::Value value)
{
    if (!assetType || InvalidEnum(value))
    {
        return false;
    }

    AssetTypeInfo* type = const_cast<AssetTypeInfo*>(assetType);

    AssetOpState::Value expectedState = AssetOpState::AOS_IDLE;

    // Expected State:
    switch (value)
    {
        case AssetOpState::AOS_IDLE:
            return false;
        case AssetOpState::AOS_LOADING:
        case AssetOpState::AOS_UNLOADING:
        case AssetOpState::AOS_DOWNLOADING:
        case AssetOpState::AOS_CACHING:
        case AssetOpState::AOS_CREATING:
        case AssetOpState::AOS_DELETED:
        case AssetOpState::AOS_UNDO_DELETE:
            break;
        default:
            return false;
    }

    // ScopeRWLockWrite lock(type->mOpStateLock); // todo
    if (type->mOpState != expectedState)
    {
        return false;
    }
    type->mOpState = value;
    return true;
}

bool AssetDataController::ClearOp(const AssetTypeInfoCPtr& assetType)
{
    if (!assetType)
    {
        return false;
    }

    AssetTypeInfo& type = const_cast<AssetTypeInfo&>(*assetType);
    // ScopeRWLockWrite lock(type->mOpStateLock); // todo:
    switch (type.mOpState)
    {
        case AssetOpState::AOS_LOADING:
        case AssetOpState::AOS_UNLOADING:
        case AssetOpState::AOS_DOWNLOADING:
        case AssetOpState::AOS_CACHING:
        case AssetOpState::AOS_CREATING:
        case AssetOpState::AOS_DELETED:
        case AssetOpState::AOS_UNDO_DELETE:
        {
            type.mOpState = AssetOpState::AOS_IDLE;
        } break;
        default:
            return false;
    }
    return true;
}

bool AssetDataController::SetLoadState(const AssetTypeInfo* assetType, AssetLoadState::Value value)
{
    if (!assetType)
    {
        return false;
    }
    Assert(assetType->mController == this);
    Assert(assetType->GetLock().IsWrite());

    const_cast<AssetTypeInfo*>(assetType)->mLoadState = value;
    return true;
}

bool AssetDataController::AddDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency)
{
    if (!assetType || !dependant)
    {
        return false;
    }

    Assert(assetType->mController == this);
    Assert(dependant->mController == this);

    volatile Atomic32* refs = weakDependency ?
          &const_cast<AssetTypeInfo*>(assetType)->mWeakReferences
        : &const_cast<AssetTypeInfo*>(assetType)->mStrongReferences;
    
    Atomic32 value = AtomicIncrement32(refs);
    Assert(value >= 0);
    return true;
}

bool AssetDataController::RemoveDependency(const AssetTypeInfo* assetType, const AssetTypeInfo* dependant, bool weakDependency)
{
    if (!assetType || !dependant)
    {
        return false;
    }

    Assert(assetType->mController == this);
    Assert(dependant->mController == this);

    volatile Atomic32* refs = weakDependency ?
        &const_cast<AssetTypeInfo*>(assetType)->mWeakReferences
        : &const_cast<AssetTypeInfo*>(assetType)->mStrongReferences;

    Atomic32 value = AtomicDecrement32(refs);
    Assert(value >= 0);
    return true;
}

APIResult<bool> AssetDataController::CreatePrototype(const AssetTypeInfo* assetType, AssetHandle*& handle)
{
    // Silently ignore invalid asset type
    if (!assetType)
    {
        return ReportError(false, ArgumentNullError, "assetType");
    }
    Assert(assetType->mController == this);
    Assert(assetType->GetLock().IsWrite());

    handle = const_cast<AssetTypeInfo*>(assetType)->mHandle;
    ReportBug(handle); // All types should have handles
    if (!handle)
    {
        return ReportError(false, InvalidArgumentError, "handle", "All types should have handles, is it possible this one is not from this DataController?");
    }

    if (handle->mPrototype)
    {
        return APIResult<bool>(false); // Acceptable call
    }

    AssetProcessor* processor = GetProcessor(assetType);
    const Type* prototypeType = processor->GetPrototypeType(assetType->GetConcreteType());
    if (prototypeType == nullptr)
    {
        return ReportError(false, InvalidPrototypeTypeError, assetType); // Failed to find prototype type, verify the processor is configured correctly.
    }
    if (prototypeType->IsAbstract())
    {
        return ReportError(false, OperationFailureAbstractTypeError, prototypeType);
    }
    // The asset type should have the write lock acquired.
    handle->mPrototype = GetReflectionMgr().CreateUnsafe<AssetObject>(prototypeType);
    // #TODO [Nathan] We need to set the atomic ptr, relies on the weak_ptr <--> asset_handle conversion
    // handle->mPrototype->GetWeakPointer() = handle;
    // 
    // -- Althought this is valid, we do want to make a guarantee (strong == 
    // AssetObjectAtomicPtr smartPtr(handle->mPrototype);
    // handle->mPrototype->GetWeakPointer() = smartPtr;
    // 
    if (!handle->mPrototype)
    {
        return ReportError(false, OperationFailureCreatePrototype, assetType);
    }
    handle->mPrototype->SetAssetType(assetType);
    AtomicStore(&handle->mWeakRefs, 1);
#if defined(LF_DEBUG) || defined(LF_TEST)
    Assert(handle->mPrototype->GetType() != nullptr);
    Assert(handle->mPrototype->GetAssetType() != nullptr);
#endif
    processor->OnCreatePrototype(handle->mPrototype);
    return APIResult<bool>(true);
}

APIResult<bool> AssetDataController::UnloadPrototype(const AssetTypeInfo* assetType)
{
    if (!assetType)
    {
        return ReportError(false, ArgumentNullError, "assetType");
    }

    Assert(assetType->mController == this);
    Assert(assetType->GetLock().IsWrite());

    AssetHandle* handle = const_cast<AssetTypeInfo*>(assetType)->mHandle;
    ReportBug(handle); // All types should have handles
    if (!handle)
    {
        return ReportError(false, InvalidArgumentError, "handle", "All types should have handles, is it possible this one is not from this DataController?");
    }

    if (!handle->mPrototype)
    {
        return APIResult<bool>(false); // Acceptable call
    }

    if (AtomicLoad(&handle->mStrongRefs) > 0)
    {
        return APIResult<bool>(false); // Acceptable call, but we can't unload an asset that is referenced
    }

    // TODO: Instance lock
    ScopeLock lock(handle->mType->mInstanceLock);
    if (!handle->mType->mInstances->empty())
    {
        return APIResult<bool>(false); // Acceptable call, but we can't unload an asset that has instances.
    }

    SetLoadState(assetType, AssetLoadState::ALS_UNLOADED);

    AssetProcessor* processor = GetProcessor(assetType);

    // If there are OS handles or something that need to be cleaned up, we should keep track of that data in a separate data structure.
    // Unloads should be capable of occuring on any thread.

    processor->OnDestroyPrototype(handle->mPrototype);

    delete handle->mPrototype;

    return APIResult<bool>(true);
}

bool AssetDataController::DeleteType(const AssetTypeInfo* assetType)
{
    ReportBug(assetType != nullptr);
    if (!assetType || assetType->mLoadState == AssetLoadState::ALS_DELETED)
    {
        return false;
    }
    const_cast<AssetTypeInfo*>(assetType)->mLoadState = AssetLoadState::ALS_DELETED;
    // TODO: What happens to the existing instances?
    //   Marking an asset as deleted will not delete the instances but certain processes can check
    //   if the asset is deleted. (This should only affect editor/tools, regular gameplay should
    //   restart if there is deleted content)
    //     
    // TODO: What happens to the existing prototype?
    // TODO: Is it safe to delete the source?
    //   Once an asset has been deleted any runtime operations should fail.
    // 
    //
    // 
    // TODO: Is it safe to delete the cache?
    return true;
}

AssetObjectAtomicPtr AssetDataController::CreateInstance(const AssetTypeInfo* assetType)
{
    if (!assetType || assetType->mController != this)
    {
        return NULL_PTR;
    }

    ScopeRWLockRead lock(assetType->GetLock());
    if (assetType->GetLoadState() != AssetLoadState::ALS_LOADED)
    {
        return NULL_PTR;
    }
    Assert(assetType->mHandle && assetType->mHandle->mPrototype);

    AssetObjectAtomicPtr object = GetReflectionMgr().CreateAtomic<AssetObject>(assetType->mConcreteType);
    object->SetAssetType(assetType);
    
    // TODO: Check asset flags (or processor) for 'IsCloneable'
    MemoryBuffer buffer;
    BinaryStream bs(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    bs.BeginObject("x", "y");
    assetType->mHandle->mPrototype->Serialize(bs);
    bs.EndObject();
    bs.Close();

    bs.Open(Stream::MEMORY, &buffer, Stream::SM_READ);
    bs.BeginObject("x", "y");
    object->Serialize(bs);
    bs.EndObject();
    bs.Close();

    // object->Clone(*assetType->mHandle->mPrototype);

    ScopeLock instanceLock(assetType->mInstanceLock);
    assetType->mInstances->push_back(object);
    return object;
}

AssetHandle* AssetDataController::GetHandle(const AssetTypeInfo* assetType)
{
    if (!assetType || assetType->mController != this)
    {
        return nullptr;
    }
    return assetType->mHandle;
}

AssetProcessor* AssetDataController::GetProcessor(const AssetTypeInfo* assetType)
{
    ReportBug(assetType && assetType->GetConcreteType());
    return GetProcessor(assetType->GetConcreteType());
}

AssetProcessor* AssetDataController::GetProcessor(const Type* concreteType)
{
    ReportBug(concreteType && concreteType->IsA(typeof(AssetObject)));

    SizeT bestDistance = INVALID;
    AssetProcessor* bestProcessor = nullptr;

    for (AssetProcessor* processor : mProcessors)
    {
        const Type* targetType = processor->GetTargetType();
        SizeT distance = concreteType->Distance(targetType);
        if (Valid(distance) && distance < bestDistance)
        {
            bestDistance = distance;
            bestProcessor = processor;
        }
    }
    return bestProcessor;
}

AssetProcessor* AssetDataController::GetProcessor(const AssetPath& path)
{
    for (AssetProcessor* processor : mProcessors)
    {
        if (processor->AcceptImportPath(path))
        {
            return processor;
        }
    }
    return nullptr;
}

AssetProcessor* AssetDataController::GetProcessor(CacheBlockType::Value cacheBlock)
{
    SizeT bestScore = INVALID;
    AssetProcessor* bestProcessor = nullptr;

    for (AssetProcessor* processor : mProcessors)
    {
        SizeT score = processor->GetCacheBlockScore(cacheBlock);
        if (Valid(score) && score < bestScore)
        {
            bestScore = score;
            bestProcessor = processor;
        }
    }
    return bestProcessor;
}

void AssetDataController::UpdateType(const AssetTypeInfo* assetType, const AssetHash* hash, const DateTime* modifyDate, bool& outUpdate)
{
    outUpdate = false;
    if (assetType->mController != this)
    {
        return;
    }

    if (!hash && !modifyDate)
    {
        return;
    }

    if (hash)
    {
        outUpdate = outUpdate || (assetType->mModifyHash != *hash);
        const_cast<AssetTypeInfo*>(assetType)->mModifyHash = *hash;
    }
    if (modifyDate)
    {
        outUpdate = outUpdate || (assetType->mModifyDate != *modifyDate);
        const_cast<AssetTypeInfo*>(assetType)->mModifyDate = *modifyDate;
    }
}

void AssetDataController::UpdateInstances(const AssetTypeInfo* assetType, AssetObject* sourceObject)
{
    if (assetType->mController != this)
    {
        return;
    }

    MemoryBuffer buffer;

    BinaryStream bs;
    bs.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
    if (bs.BeginObject("a", "b"))
    {
        sourceObject->Serialize(bs);
        bs.EndObject();
    }
    bs.Close();

    bs.Open(Stream::MEMORY, &buffer, Stream::SM_READ);
    ScopeLock instanceLock(assetType->mInstanceLock);
    AssetTypeInfo::WeakAssetInstanceArray& instances = *const_cast<AssetTypeInfo*>(assetType)->mInstances;
    for (auto& instance : instances)
    {
        AssetObjectAtomicPtr pinned = instance;
        if (pinned)
        {
            if(bs.BeginObject("a", "b"))
            {
                pinned->Serialize(bs);
                bs.EndObject();
            }
        }
    }
    bs.Close();
}

bool AssetDataController::HasDomain(const String& domain)
{
    ScopeRWSpinLockRead lock(mLock);
    for (DomainContext* context : mDomainContexts)
    {
        if (StrCompareAgnostic(context->mDomain, domain))
        {
            return true;
        }
    }
    return false;
}

TVector<String> AssetDataController::GetDomains()
{
    TVector<String> domains;
    ScopeRWSpinLockRead lock(mLock);
    domains.reserve(mDomainContexts.size());
    for (DomainContext* context : mDomainContexts)
    {
        domains.push_back(context->mDomain);
    }
    return domains;
}

TVector<AssetTypeInfoCPtr> AssetDataController::GetTypes(const String& domain)
{
    TVector<AssetTypeInfoCPtr> types;

    ScopeRWSpinLockRead lock(mLock);
    types.reserve(mTable.size());

    for (const auto& pair : mTable)
    {
        if (StrCompareAgnostic(domain, pair.second.mType.GetPath().GetDomain()))
        {
            types.push_back(AssetTypeInfoCPtr(&pair.second.mType));
        }
    }
    return types;
}

TVector<AssetTypeInfoCPtr> AssetDataController::GetTypes(const Type* concreteType)
{
    TVector<AssetTypeInfoCPtr> types;
    if (!concreteType || !concreteType->IsA(typeof(AssetObject)))
    {
        return types;
    }

    ScopeRWSpinLockRead lock(mLock);
    for (const auto& pair : mTable)
    {
        AssetTypeInfoCPtr type(&pair.second.mType);
        if (!IsDeleted(type) && !IsCorrupted(type) && type->GetConcreteType()->IsA(concreteType))
        {
            types.push_back(type);
        }
    }
    return types;
}

AssetTypeInfoCPtr AssetDataController::FindConcrete(const Type* concreteType) const
{
    if (!concreteType->IsA(typeof(AssetObject)))
    {
        return AssetTypeInfoCPtr();
    }

    auto aliasIterator = mAliasTable.find(concreteType->GetFullName().CStr());
    if (aliasIterator == mAliasTable.end())
    {
        return AssetTypeInfoCPtr();
    }

    return AssetTypeInfoCPtr(&aliasIterator->second->second.mType);
}

AssetDataController::DomainContext* AssetDataController::FindDomain(const String& domain)
{
    for (DomainContext* context : mDomainContexts)
    {
        if (StrCompareAgnostic(context->mDomain, domain))
        {
            return context;
        }
    }
    return nullptr;
}

const AssetDataController::DomainContext* AssetDataController::FindDomain(const String& domain) const
{
    for (const DomainContext* context : mDomainContexts)
    {
        if (StrCompareAgnostic(context->mDomain, domain))
        {
            return context;
        }
    }
    return nullptr;
}

UInt32 AssetDataController::GenerateUID(DomainContext* context)
{
    if (!context)
    {
        return INVALID32;
    }

    UInt32 id = 0;
    do {
        Crypto::SecureRandomBytes(reinterpret_cast<ByteT*>(&id), sizeof(id));

        if (mIDTable.find(id) == mIDTable.end())
        {
            return id;
        }

    } while (true);

    return INVALID32;
}

void AssetDataController::ReleaseDomainContext(DomainContext* context)
{
    // TODO: Loading and unloading 'mods' at runtime, how well should we support this.
    // TODO: What about the instances

    // Aggressively destroying prototypes might have unwanted consequences
    // but it should technically be correct/safe.

    for (auto& pair : mTable)
    {
        auto& type = pair.second.mType;
        auto& handle = pair.second.mHandle;
        if (handle.mPrototype && StrCompareAgnostic(type.GetPath().GetDomain(),context->mDomain))
        {
            // We can't Release a DC while someone holds reference to a handle.
            LFDelete(handle.mPrototype);
            handle.mPrototype = nullptr;
            type.mLoadState = AssetLoadState::ALS_UNLOADED;
        }
    }

    // Verify after all dependencies have been destroyed, the DC is gone.
    for (auto& pair : mTable)
    {
        auto& type = pair.second.mType;
        auto& handle = pair.second.mHandle;
        if (StrCompareAgnostic(type.GetPath().GetDomain(), context->mDomain))
        {
            Assert(handle.mWeakRefs <= 1);
            Assert(handle.mStrongRefs == 0);
        }
    }


}

void AssetDataController::CollectGarbage(AssetTypeInfo& assetType)
{
    ScopeLock instanceLock(assetType.mInstanceLock);
    AssetTypeInfo::WeakAssetInstanceArray& instances = *assetType.mInstances;
    for (auto it = instances.begin(); it != instances.end();)
    {
        if (*it == NULL_PTR)
        {
            gSysLog.Info(LogMessage("Instance of ") << assetType.GetPath().CStr() << " deleted.");
            it = instances.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace lf

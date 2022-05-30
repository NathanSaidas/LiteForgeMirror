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
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANYJsonStream CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#pragma once
#include "Core/Utility/StdMap.h"
#include "Core/Utility/StdVector.h"
#include "Core/Platform/Atomic.h"
#include "Core/Memory/SmartPointer.h"
#include "Runtime/Asset/CacheTypes.h"

namespace lf
{

class AssetTypeInfo;
class CacheDBHandle;
class CacheDB;

DECLARE_MANAGED_PTR(CacheDBHandle);
DECLARE_MANAGED_CPTR(CacheDBHandle);

class LF_RUNTIME_API CacheDBHandle
{
public:
    using DatabaseID = CacheTypes::DatabaseID;
    using DatabaseIDArray = TStackVector<DatabaseID, 16>;
    using DependencyArray = TStackVector<CacheDependencyListDBEntry, 16>;

    CacheDBHandle();
    ~CacheDBHandle();

    // Write -- Write information from AssetTypeInfo to the cache db
    bool WriteCacheInfo();
    bool WriteReferences();
    bool WriteStrongDependencies(const TVector<const AssetTypeInfo*>& dependencies);
    bool WriteWeakDependencies(const TVector<const AssetTypeInfo*>& dependencies);

    // Read -- Read information from the cache, you can use the data controller to update the type.
    bool ReadCacheInfo(CacheInfoDBEntry& outValue);
    bool ReadReferences(CacheReferenceCountDBEntry& outValue);
    bool ReadStrongDependencies(DependencyArray& outValue);
    bool ReadWeakDependencies(DependencyArray& outValue);

    DatabaseID GetTypeId() const { return mTypeId; }
    DatabaseID GetCacheInfoId() const { return mCacheInfoId; }
    DatabaseID GetReferenceCountId() const { return mReferenceCountId; }
    DatabaseIDArray GetStrongDependencyIds() const 
    { 
        DatabaseIDArray ids; 
        ids.insert(ids.end(), mStrongDependencyIds.begin(), mStrongDependencyIds.end()); 
        return ids; 
    }
    DatabaseIDArray GetWeakDependencyIds() const 
    {
        DatabaseIDArray ids;
        ids.insert(ids.end(), mWeakDependencyIds.begin(), mWeakDependencyIds.end());
        return ids;
    }

    void IncrementRef() const { AtomicIncrement32(&mRefs); }
    void DecrementRef() const { Assert(AtomicDecrement32(&mRefs) >= 0); }
    SizeT GetRefs() const { return static_cast<SizeT>(AtomicLoad(&mRefs)); }

    // Attempt to create CacheDBHandle, all data must not exist.
    static CacheDBHandle* Create(const AssetTypeInfo* type, CacheDB* cacheDB, MemDB* db);
    // Attempt to load CacheDBHandle, all data must exist
    static CacheDBHandle* Load(const AssetTypeInfo* type, CacheDB* cacheDB, MemDB* db);
private:
    bool CreateFromType();
    bool LoadFromType();
    UInt32 GetUID() const;

    template<typename EntryT, typename CallbackT>
    bool Write(MemDB::TableID table, DatabaseID& id, CallbackT& callback, SizeT indexOffset);

    template<typename EntryT, typename CallbackT>
    bool Read(MemDB::TableID table, DatabaseID& id, CallbackT& callback, SizeT indexOffset);

    

    const AssetTypeInfo*    mType;
    CacheDB*                mCacheDB;
    MemDB*                  mDB;

    DatabaseID              mTypeId;
    DatabaseID              mCacheInfoId;
    DatabaseID              mReferenceCountId;
    TVector<DatabaseID>      mStrongDependencyIds;
    TVector<DatabaseID>      mWeakDependencyIds;

    mutable volatile Atomic32 mRefs;
};

// The CacheDB stores meta information about types
//
// As a developer, any types created will be written to the CacheDB
// As a developer, builds can be created. This will contain [ Manifest, Patch Manifest, CacheDB, CacheContent ]
// 
// Manifest - Used to determine what content is needed
// Patch Manifest - A trimmed version of a full manifest, only containing changed content
// CacheDB - A database of meta-information about the content
// CacheContent - The actual content binary
//
// We'll initialize one CacheDB per domain
class LF_RUNTIME_API CacheDB
{
public:
    using DatabaseID = CacheTypes::DatabaseID;

    CacheDB();
    ~CacheDB();
    // Example Structure:
    // 
    // %ContentCache%/Directory/Cache.json                          ; Info on the database
    // %ContentCache%/Directory/Cache_Types.db                      ; Data for Types table
    // %ContentCache%/Directory/Cache_Types_UID.dbi                 ; Data for Type.UID index
    // %ContentCache%/Directory/Cache_Types_PathHash.dbi            ; Data for Type.PathHash index
    // %ContentCache%/Directory/Cache_Types_ParentUID.dbi           ; Data for Type.ParentUID index
    // %ContentCache%/Directory/Cache_Types_ConcreteTypeHash.dbi    ; Data for ConcreteTypeHash index
    // %ContentCache%/Directory/Cache_CacheInfo.db
    // %ContentCache%/Directory/Cache_ReferenceCount.db

    bool Initialize(const String& directory);
    void Shutdown();

    void Save();
    void Load();

    // Create a handle for a specific asset type, eg creating/importing asset
    CacheDBHandlePtr Create(const AssetTypeInfo* type);
    // Load a handle for a specific asset type, eg loading asset.
    CacheDBHandlePtr Load(const AssetTypeInfo* type);
    // Purge all data associated with this asset type.
    bool Purge(const AssetTypeInfo* type);

    CacheTypes::TableID GetTypeTable() const { return mTypeTable; }
    CacheTypes::TableID GetCacheInfoTable() const { return mCacheInfoTable; }
    CacheTypes::TableID GetReferenceCountTable() const { return mReferenceCountTable; }
    CacheTypes::TableID GetStrongDependencyTable() const { return mStrongDependencyTable; }
    CacheTypes::TableID GetWeakDependencyTable() const { return mWeakDependencyTable; }

    MemDB* GetDB() { return &mDB; }
private:

    MemDB mDB;
    String mDBDirectory;

    TMap<const AssetTypeInfo*, CacheDBHandlePtr> mHandles;

    CacheTypes::TableID mTypeTable;
    CacheTypes::TableID mCacheInfoTable;
    CacheTypes::TableID mReferenceCountTable;
    CacheTypes::TableID mStrongDependencyTable;
    CacheTypes::TableID mWeakDependencyTable;
};

template<typename EntryT, typename CallbackT>
bool CacheDBHandle::Write(MemDB::TableID table, DatabaseID& id, CallbackT& callback, SizeT indexOffset)
{
    if (mDB->SelectWrite<EntryT>(table, id, callback))
    {
        return true;
    }
    if (!mDB->FindOneIndexed(table, NumericalVariant(GetUID()), indexOffset, id))
    {
        return false;
    }
    return mDB->SelectWrite<EntryT>(table, id, callback);
}

template<typename EntryT, typename CallbackT>
bool CacheDBHandle::Read(MemDB::TableID table, DatabaseID& id, CallbackT& callback, SizeT indexOffset)
{
    if (mDB->SelectRead<EntryT>(table, id, callback))
    {
        return true;
    }
    if (!mDB->FindOneIndexed(table, NumericalVariant(GetUID()), indexOffset, id))
    {
        return false;
    }
    return mDB->SelectRead<EntryT>(table, id, callback);
}

} // namespace lf
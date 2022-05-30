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
#include "Runtime/PCH.h"
#include "CacheDB.h"
#include "Core/Reflection/Type.h"
#include "Core/Platform/FileSystem.h"
#include "Runtime/Asset/AssetTypeInfo.h"

namespace lf
{

CacheDBHandle::CacheDBHandle()
: mType(nullptr)
, mCacheDB(nullptr)
, mDB(nullptr)
, mTypeId(MemDBTypes::INVALID_ENTRY_ID)
, mCacheInfoId(MemDBTypes::INVALID_ENTRY_ID)
, mReferenceCountId(MemDBTypes::INVALID_ENTRY_ID)
, mStrongDependencyIds()
, mWeakDependencyIds()
, mRefs(0)
{}
CacheDBHandle::~CacheDBHandle()
{}

bool CacheDBHandle::WriteCacheInfo()
{
    auto update = [this](CacheInfoDBEntry* entry)
    {
        entry->mDate = mType->GetModifyDate().Encode();
        entry->mHash = Crypto::MD5Hash(); // TODO:
    };
    return Write<CacheInfoDBEntry>(mCacheDB->GetCacheInfoTable(), mCacheInfoId, update, LF_OFFSET_OF(CacheInfoDBEntry, mUID));
}
bool CacheDBHandle::WriteReferences()
{
    auto update = [this](CacheReferenceCountDBEntry* entry)
    {
        entry->mStrong = mType->GetStrongReferences();
        entry->mWeak = mType->GetWeakReferences();
    };
    return Write<CacheReferenceCountDBEntry>(mCacheDB->GetReferenceCountTable(), mReferenceCountId, update, LF_OFFSET_OF(CacheReferenceCountDBEntry, mUID));
}
bool CacheDBHandle::WriteStrongDependencies(const TVector<const AssetTypeInfo*>& dependencies)
{
    for (DatabaseID id : mStrongDependencyIds)
    {
        if (!mDB->Delete(mCacheDB->GetStrongDependencyTable(), id))
        {
            return false;
        }
    }
    mStrongDependencyIds.clear();

    UInt32 uid = GetUID();

    for (SizeT i = 0; i < dependencies.size(); ++i)
    {
        Assert(dependencies[i] != mType);

        CacheDependencyListDBEntry entry;
        entry.mUID = uid;
        entry.mType = dependencies[i]->GetCacheIndex().mUID;
        entry.mIndex = static_cast<UInt32>(i);
        DatabaseID id;
        if (!mDB->Insert(mCacheDB->GetStrongDependencyTable(), entry, id))
        {
            return false;
        }
        mStrongDependencyIds.push_back(id);
    }
    return true;
}
bool CacheDBHandle::WriteWeakDependencies(const TVector<const AssetTypeInfo*>& dependencies)
{
    for (DatabaseID id : mWeakDependencyIds)
    {
        if (!mDB->Delete(mCacheDB->GetWeakDependencyTable(), id))
        {
            return false;
        }
    }
    mWeakDependencyIds.clear();

    UInt32 uid = GetUID();

    for (SizeT i = 0; i < dependencies.size(); ++i)
    {
        Assert(dependencies[i] != mType); // TODO: This might be allowed...

        CacheDependencyListDBEntry entry;
        entry.mUID = uid;
        entry.mType = dependencies[i]->GetCacheIndex().mUID;
        entry.mIndex = static_cast<UInt32>(i);
        DatabaseID id;
        if (!mDB->Insert(mCacheDB->GetWeakDependencyTable(), entry, id))
        {
            return false;
        }
        mWeakDependencyIds.push_back(id);
    }
    return true;
}

bool CacheDBHandle::ReadCacheInfo(CacheInfoDBEntry& outValue)
{
    auto acquire = [&outValue](const CacheInfoDBEntry* entry)
    {
        outValue = *entry;
    };
    return Read<CacheInfoDBEntry>(mCacheDB->GetCacheInfoTable(), mCacheInfoId, acquire, LF_OFFSET_OF(CacheInfoDBEntry, mUID));
}
bool CacheDBHandle::ReadReferences(CacheReferenceCountDBEntry& outValue)
{
    auto acquire = [&outValue](const CacheReferenceCountDBEntry* entry)
    {
        outValue = *entry;
    };
    return Read<CacheReferenceCountDBEntry>(mCacheDB->GetReferenceCountTable(), mCacheInfoId, acquire, LF_OFFSET_OF(CacheReferenceCountDBEntry, mUID));
}
bool CacheDBHandle::ReadStrongDependencies(DependencyArray& outValue)
{
    outValue.clear();
    outValue.reserve(mStrongDependencyIds.size());

    auto acquire = [&outValue](const CacheDependencyListDBEntry* entry)
    {
        outValue.push_back(*entry);
    };

    for (DatabaseID id : mStrongDependencyIds)
    {
        if (!Read<CacheDependencyListDBEntry>(mCacheDB->GetStrongDependencyTable(), id, acquire, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID)))
        {
            return false;
        }
    }
    return true;
}
bool CacheDBHandle::ReadWeakDependencies(DependencyArray& outValue)
{
    outValue.clear();
    outValue.reserve(mWeakDependencyIds.size());

    auto acquire = [&outValue](const CacheDependencyListDBEntry* entry)
    {
        outValue.push_back(*entry);
    };

    for (DatabaseID id : mWeakDependencyIds)
    {
        if (!Read<CacheDependencyListDBEntry>(mCacheDB->GetStrongDependencyTable(), id, acquire, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID)))
        {
            return false;
        }
    }
    return true;
}

CacheDBHandle* CacheDBHandle::Create(const AssetTypeInfo* type, CacheDB* cacheDB, MemDB* db)
{
    if (!type || !cacheDB || !db)
    {
        return false;
    }

    // Can't cache built-in concrete-types.
    if (!type->GetParent())
    {
        return false;
    }

    CacheDBHandle* handle = LFNew<CacheDBHandle>();
    handle->mType = type;
    handle->mCacheDB = cacheDB;
    handle->mDB = db;

    if (!handle->CreateFromType())
    {
        LFDelete(handle);
        return nullptr;
    }
    return handle;
}
CacheDBHandle* CacheDBHandle::Load(const AssetTypeInfo* type, CacheDB* cacheDB, MemDB* db)
{
    if (!type || !cacheDB || !db)
    {
        return false;
    }

    // Can't cache built-in concrete-types.
    if (!type->GetParent())
    {
        return false;
    }

    CacheDBHandle* handle = LFNew<CacheDBHandle>();
    handle->mType = type;
    handle->mCacheDB = cacheDB;
    handle->mDB = db;

    if (!handle->LoadFromType())
    {
        LFDelete(handle);
        return nullptr;
    }
    return handle;
}
bool CacheDBHandle::CreateFromType()
{
    UInt32 uid = GetUID();

    {
        CacheDBEntry entry;
        entry.mUID = uid;
        entry.mPathHash = FNV::Hash(mType->GetPath().CStr(), mType->GetPath().Size());
        entry.mParentUID = mType->GetParent()->GetCacheIndex().mUID;
        entry.mConcreteTypeHash = FNV::Hash(mType->GetConcreteType()->GetFullName().CStr(), mType->GetConcreteType()->GetFullName().Size());
        entry.mPath.Assign(mType->GetPath().CStr());
        entry.mParent.Assign(mType->GetPath().CStr());
        entry.mConcreteType.Assign(mType->GetConcreteType()->GetFullName().CStr());

        if (!mDB->Insert(mCacheDB->GetTypeTable(), entry, mTypeId))
        {
            return false;
        }
    }

    {
        CacheInfoDBEntry entry;
        entry.mUID = uid;
        entry.mDate = mType->GetModifyDate().Encode();
        entry.mHash = Crypto::MD5Hash(); // mType->GetModifyHash();
        if (!mDB->Insert(mCacheDB->GetCacheInfoTable(), entry, mCacheInfoId))
        {
            return false;
        }
    }

    {
        CacheReferenceCountDBEntry entry;
        entry.mUID = uid;
        entry.mStrong = mType->GetStrongReferences();
        entry.mWeak = mType->GetWeakReferences();
        if (!mDB->Insert(mCacheDB->GetReferenceCountTable(), entry, mReferenceCountId))
        {
            return false;
        }
    }

    // TODO: Get Strong Dependencies...
    // TODO: Get Weak Dependencies...

    return true;
}
bool CacheDBHandle::LoadFromType()
{
    NumericalVariant uid(GetUID());

    if (!mDB->FindOneIndexed(mCacheDB->GetTypeTable(), uid, LF_OFFSET_OF(CacheDBEntry, mUID), mTypeId))
    {
        return false;
    }

    if (!mDB->FindOneIndexed(mCacheDB->GetCacheInfoTable(), uid, LF_OFFSET_OF(CacheInfoDBEntry, mUID), mCacheInfoId))
    {
        return false;
    }

    if (!mDB->FindOneIndexed(mCacheDB->GetReferenceCountTable(), uid, LF_OFFSET_OF(CacheReferenceCountDBEntry, mUID), mReferenceCountId))
    {
        return false;
    }

    if (!mDB->FindRangeIndexed(mCacheDB->GetStrongDependencyTable(), uid, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID), mStrongDependencyIds))
    {
        return false;
    }

    if (!mDB->FindRangeIndexed(mCacheDB->GetWeakDependencyTable(), uid, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID), mWeakDependencyIds))
    {
        return false;
    }

    return true;
}
UInt32 CacheDBHandle::GetUID() const
{
    return mType->GetCacheIndex().mUID;
}

CacheDB::CacheDB()
{}
CacheDB::~CacheDB()
{}

bool CacheDB::Initialize(const String& directory)
{
    if(!mDB.CreateTable<CacheDBEntry>("Types", mTypeTable)
    || !mDB.CreateTable<CacheInfoDBEntry>("Info", mCacheInfoTable)
    || !mDB.CreateTable<CacheReferenceCountDBEntry>("ReferenceCounts", mReferenceCountTable)
    || !mDB.CreateTable<CacheDependencyListDBEntry>("WeakDependency", mWeakDependencyTable)
    || !mDB.CreateTable<CacheDependencyListDBEntry>("StrongDependency", mStrongDependencyTable))
    {
        return false; // Failed to create tables...
    }

    if (!directory.Empty())
    {
        // mDB.ReadFromFile(mTypeTable, FileSystem::PathJoin(directory, "_Types.db"));
        // mDB.ReadFromFile(mCacheInfoTable, FileSystem::PathJoin(directory, "_Info.db"));
        // mDB.ReadFromFile(mReferenceCountTable, FileSystem::PathJoin(directory, "_ReferenceCounts.db"));
        // mDB.ReadFromFile(mWeakDependencyTable, FileSystem::PathJoin(directory, "_WeakDependency.db"));
        // mDB.ReadFromFile(mStrongDependencyTable, FileSystem::PathJoin(directory, "_StrongDependency.db"));
        // mDBDirectory = directory;
    }

    if (!mDB.CreateIndex(mTypeTable, NumericalVariant::VariantType::VT_U32, LF_OFFSET_OF(CacheDBEntry, mUID))
        || !mDB.CreateIndex(mCacheInfoTable, NumericalVariant::VariantType::VT_U32, LF_OFFSET_OF(CacheInfoDBEntry, mUID))
        || !mDB.CreateIndex(mReferenceCountTable, NumericalVariant::VariantType::VT_U32, LF_OFFSET_OF(CacheReferenceCountDBEntry, mUID))
        || !mDB.CreateIndex(mWeakDependencyTable, NumericalVariant::VariantType::VT_U32, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID))
        || !mDB.CreateIndex(mStrongDependencyTable, NumericalVariant::VariantType::VT_U32, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID)))
    {
        return false; // Failed to craete indices
    }

    mDB.Open(directory);

    return true;
}
void CacheDB::Shutdown()
{
    for (auto& pair : mHandles)
    {
        // TODO: Maybe commit data to DB
        
        // TODO: Detatch Type/Handle

        
        // Delete the handle
        CacheDBHandle* handle = pair.second;
        pair.second.Release();
        Assert(handle->GetRefs() == 0);
        LFDelete(handle);

        // Remove Type Reference
        pair.first->DecrementRef();
    }
    mHandles.clear();

    mDB.Close();

    // TODO: Save
    mDB.Release();

    mTypeTable = INVALID;
    mCacheInfoTable = INVALID;
    mReferenceCountTable = INVALID;
    mStrongDependencyTable = INVALID;
    mWeakDependencyTable = INVALID;
}

void CacheDB::Save()
{
    mDB.Save(MemDB::SAVE_DIRTY_LIST);
}
void CacheDB::Load()
{
    mDB.Load();
}

CacheDBHandlePtr CacheDB::Create(const AssetTypeInfo* type)
{
    auto it = mHandles.find(type);
    if (it != mHandles.end())
    {
        return it->second;
    }

    CacheDBHandlePtr handle(CacheDBHandle::Create(type, this, &mDB));
    if (!handle)
    {
        Purge(type);
        return CacheDBHandlePtr();
    }

    type->IncrementRef();
    mHandles.insert(std::make_pair(type, handle));

    return handle;
}
CacheDBHandlePtr CacheDB::Load(const AssetTypeInfo* type)
{
    auto it = mHandles.find(type);
    if (it != mHandles.end())
    {
        return it->second;
    }

    CacheDBHandlePtr handle(CacheDBHandle::Load(type, this, &mDB));
    if (!handle)
    {
        return CacheDBHandlePtr();
    }

    type->IncrementRef();
    mHandles.insert(std::make_pair(type, handle));

    return handle;
}
bool CacheDB::Purge(const AssetTypeInfo* type)
{
    bool failed = false;

    NumericalVariant uid(type->GetCacheIndex().mUID);
    DatabaseID typeId;
    DatabaseID cacheInfoId;
    DatabaseID referenceCountId;
    TVector<DatabaseID> strong;
    TVector<DatabaseID> weak;

    if (mDB.FindOneIndexed(GetTypeTable(), uid, LF_OFFSET_OF(CacheDBEntry, mUID), typeId))
    {
        failed = failed || !mDB.Delete(GetTypeTable(), typeId);
    }

    if (mDB.FindOneIndexed(GetCacheInfoTable(), uid, LF_OFFSET_OF(CacheInfoDBEntry, mUID), cacheInfoId))
    {
        failed = failed || !mDB.Delete(GetCacheInfoTable(), cacheInfoId);
    }

    if (mDB.FindOneIndexed(GetReferenceCountTable(), uid, LF_OFFSET_OF(CacheReferenceCountDBEntry, mUID), referenceCountId))
    {
        failed = failed || !mDB.Delete(GetReferenceCountTable(), referenceCountId);
    }

    if (mDB.FindRangeIndexed(GetStrongDependencyTable(), uid, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID), strong))
    {
        for (DatabaseID id : strong)
        {
            failed = failed || !mDB.Delete(GetStrongDependencyTable(), id);
        }
    }

    if (mDB.FindRangeIndexed(GetWeakDependencyTable(), uid, LF_OFFSET_OF(CacheDependencyListDBEntry, mUID), weak))
    {
        for (DatabaseID id : weak)
        {
            failed = failed || !mDB.Delete(GetWeakDependencyTable(), id);
        }
    }
    
    return !failed;
}

} // namespace lf
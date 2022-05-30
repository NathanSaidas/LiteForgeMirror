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
#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Common/Enum.h"
#include "Core/String/String.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/NumericalVariant.h"
#include "Core/Platform/RWSpinLock.h"

namespace lf 
{

namespace MemDBTypes
{
using EntryID = UInt32;
using EntryFlags = UInt32;

// ********************************************************************
// The base type all types must derive from to be used in the memdb.
// POD types only!
// ********************************************************************
struct Entry
{
    EntryID    mReservedID;
    EntryFlags mReservedFlags;
};

using TableID = SizeT;
struct Table;
using EntryFindCallback = bool(*)(const ByteT*, void*);
using EntryReadCallback = void(*)(const ByteT*, void*);
using EntryReadWriteCallback = void(*)(ByteT*, void*);

class EntryWriter
{
public:
    virtual bool BeginCommit(SizeT tableCapcity, SizeT alignment) = 0;
    virtual void Commit(const ByteT* bytes, SizeT size, SizeT alignment, SizeT offsetFromBase) = 0;
    virtual void EndCommit() = 0;
};

DECLARE_ENUM(OpTypes,
OP_FIND_ONE,
OP_FIND_ONE_INDEXED,
OP_FIND_RANGE_INDEXED,
OP_FIND_ALL,
OP_INSERT,
OP_BULK_INSERT,
OP_DELETE,
OP_UPDATE_ONE,
OP_SELECT_READ,
OP_SELECT_WRITE);

enum : EntryID { INVALID_ENTRY_ID = INVALID32 };

} // namespace MemDBTypes

struct MemDBStats
{
    SizeT mDataBytesReserved;
    SizeT mDataBytesUsed;
    SizeT mRuntimeBytesUsed;
    SizeT mRuntimeBytesReserved;

    SizeT mOpCounts[MemDBTypes::OpTypes::MAX_VALUE];

    SizeT mResizeCount;
};


template<typename CharT, SizeT CArrayLength>
class MemDBStringType
{
public:
    using CharType = CharT;
    enum { ArrayLength = CArrayLength + 1};

    MemDBStringType();
    explicit MemDBStringType(const CharT* string);

    void Clear();
    void Assign(const CharT* string);
    void Append(const CharT* string);
    bool Equals(const CharT* string) const;

    MemDBStringType& operator=(const MemDBStringType& other) { Assign(other.mData); return *this; }
    template<SizeT COtherArrayLength>
    MemDBStringType& operator=(const MemDBStringType<CharT, COtherArrayLength>& other) { Assign(other.CStr()); return *this; }
    
    bool operator==(const MemDBStringType& other) const { return Equals(other.CStr()); }
    template<SizeT COtherArrayLength>
    bool operator==(const MemDBStringType<CharT, COtherArrayLength>& other) const { return Equals(other.CStr()); }

    bool operator!=(const MemDBStringType& other) const { return !Equals(other.CStr()); }
    template<SizeT COtherArrayLength>
    bool operator!=(const MemDBStringType<CharT, COtherArrayLength>& other) const { return !Equals(other.CStr()); }

    bool Empty() const { return mData[0] == '\0'; }
    const CharT* CStr() const { return mData; }
    const SizeT Size() const { return StrLen(CStr()); }
    const SizeT Capacity() const { return ArrayLength; }
    const SizeT ByteSize() const { return Size() * sizeof(CharT); }

private:
    CharT mData[ArrayLength];
};

template<SizeT CArrayLength>
using MemDBChar = MemDBStringType<char, CArrayLength>;

template<SizeT CArrayLength>
using MemDBWChar = MemDBStringType<wchar_t, CArrayLength>;

using MemDBField = MemDBChar<80>;

// ********************************************************************
// TODO: 
// Table Stats:
// Preserve Table Properties in .json file
// MemDB Support Json Data.
// • Convert CacheIndex to use MemDB instead. 
// • AssetMgr use MemDB to store reference mappings
// • AssetMgr use MemDB to store types
// TableUpdateIndex/TableDeleteIndex could be optimized a bit if we save the data before 
// we apply the modification, and use the old data to update the index with new data.
//
// MemDB is a VERY lightweight database (or data store even). It supports
// very basic operations ( Insert/Update/Delete/Find ) however the functions
// are not exactly designed for optimized search or 'big data'. 
// 
// MemDB intended usage is going to be reading/writing binary data by editing 
// small portions of a file rather then re-writing the whole thing everytime.
//
// All operations are intended to be 'threadsafe' however they are not optimized
// for multithreaded access (eg theres only one lock on the whole DB)
// 
// Management of EntryID/TableID is on the user. The DB does not use any 
// type of serial to verify data integrity.
// 
// File Formats:
//      Compressed;
//      In-Memory;
//      Journal;
// 
// ********************************************************************
class LF_CORE_API MemDB
{
public:
    using Entry = MemDBTypes::Entry;
    using EntryID = MemDBTypes::EntryID;
    using EntryFlags = MemDBTypes::EntryFlags;
    using TableID = MemDBTypes::TableID;
    using Table = MemDBTypes::Table;
    using EntryReadCallback = MemDBTypes::EntryReadCallback;
    using EntryReadWriteCallback = MemDBTypes::EntryReadWriteCallback;
    using EntryFindCallback = MemDBTypes::EntryFindCallback;

    enum SaveMode
    {
        SAVE_FULL,      // Save the whole thing
        SAVE_DIRTY,     // Iterate through each entry and save the dirty version
        SAVE_DIRTY_LIST // Iterate through the smaller list of dirty entries
    };

    MemDB();
    ~MemDB();

    void WriteToFile(TableID table, const String& filename, bool fullFlush = false);
    void ReadFromFile(TableID table, const String& filename);

    // usage: SaveToFiles( "ContentCache/Cache" )
    void Open(const String& filename);
    void Close();

    void Save(SaveMode mode = SAVE_FULL);
    void Load();
    // void LoadFromFiles(const String filename);

    void CommitDirty(TableID table, MemDBTypes::EntryWriter* writer, SaveMode mode = SAVE_DIRTY);
    bool LoadTableData(TableID table, const ByteT* bytes, SizeT numBytes);

    void Release();

    bool CreateTable(const String& name, SizeT entrySize, SizeT entryAlignment, TableID& outIndex);
    bool CreateTable(const String& name, SizeT entrySize, SizeT entryAlignment, SizeT entryCapacity, TableID& outIndex);
    bool DeleteTable(const String& name);
    bool DeleteTable(TableID index);
    bool FindTable(const String& name, TableID& outIndex) const;
    

    bool CreateIndex(TableID table, NumericalVariant::VariantType dataType, SizeT dataOffset, bool allowDuplicates = false);

    template<typename EntryT>
    bool CreateTable(const String& name, TableID& outIndex)
    {
        return CreateTable(name, sizeof(EntryT), alignof(EntryT), outIndex);
    }

    template<typename EntryT>
    bool CreateTable(const String& name, SizeT entryCapacity, TableID& outIndex)
    {
        return CreateTable(name, sizeof(EntryT), alignof(EntryT), entryCapacity, outIndex);
    }

    // Read only operation, does not affect the index
    bool FindOne(TableID table, SizeT entrySize, SizeT entryAlignment, EntryFindCallback findCallback, void* findUserData, EntryID& outID) const;
    // Read only operation, does not affect the index
    bool FindOneIndexed(TableID table, NumericalVariant value, SizeT dataOffset, EntryID& outID);
    // Read only operation,
    bool FindRangeIndexed(TableID table, NumericalVariant value, SizeT dataOffset, TVector<EntryID>& outIDs);
    // Read only operation, does not affect the index
    bool FindAll(TableID table, SizeT entrySize, SizeT entryAlignment, EntryFindCallback findCallback, void* findUserData, TVector<EntryID>& outIDs) const;
    // Write operation, can affect the index
    bool Insert(TableID table, const Entry* entryData, SizeT entrySize, SizeT entryAlignment, EntryID& outIDs);
    bool BulkInsert(TableID table, const Entry* entryData, SizeT entrySize, SizeT entryAlignment, SizeT numEntries, TVector<EntryID>& outID);
    

    bool UpdateOne(TableID table, EntryID entryID, const Entry* entryData, SizeT entrySize, SizeT entryAlignment);
    bool Delete(TableID table, EntryID id);
    bool Select(TableID table, EntryID entryID, SizeT entrySize, SizeT entryAlignment, EntryReadWriteCallback selectCallback, void* selectUserData);
    bool Select(TableID table, EntryID entryID, SizeT entrySize, SizeT entryAlignment, EntryReadCallback selectCallback, void* selectUserData);

    

    template<typename EntryT, typename FindCallbackT>
    bool FindOne(TableID table, FindCallbackT findCallback, EntryID& outID) const
    {
        LF_STATIC_IS_A(EntryT, Entry);
        return FindOne(table, sizeof(EntryT), alignof(EntryT),
            [](const ByteT* item, void* userData) -> bool
            { 
                FindCallbackT& findCallback = *reinterpret_cast<FindCallbackT*>(userData);
                return findCallback(reinterpret_cast<const EntryT*>(item));
            },
            &findCallback, 
            outID);
    }

    template<typename EntryT, typename FindCallbackT>
    bool FindAll(TableID table, FindCallbackT findCallback, TVector<EntryID>& outIDs) const
    {
        LF_STATIC_IS_A(EntryT, Entry);
        return FindAll(table, sizeof(EntryT), alignof(EntryT),
            [](const ByteT* item, void* userData) -> bool
            {
                FindCallbackT& findCallback = *reinterpret_cast<FindCallbackT*>(userData);
                return findCallback(reinterpret_cast<const EntryT*>(item));
            },
            &findCallback,
            outIDs);
    }

    template<typename EntryT>
    bool Insert(TableID table, const EntryT& entryData, EntryID& outID)
    {
        LF_STATIC_IS_A(EntryT, Entry);
        return Insert(table, &entryData, sizeof(EntryT), alignof(EntryT), outID);
    }

    template<typename EntryT>
    bool BulkInsert(TableID table, const TVector<EntryT>& entryData, TVector<EntryID>& outIDs)
    {
        LF_STATIC_IS_A(EntryT, Entry);
        return BulkInsert(table, entryData.data(), sizeof(EntryT), alignof(EntryT), entryData.size(), outIDs);
    }

    template<typename EntryT>
    bool UpdateOne(TableID table, EntryID entryID, const EntryT* entryData)
    {
        LF_STATIC_IS_A(EntryT, Entry);
        return UpdateOne(table, entryID, entryData, sizeof(EntryT), alignof(EntryT));
    }

    template<typename EntryT, typename SelectCallbackT>
    bool SelectRead(TableID table, EntryID entryID, SelectCallbackT selectCallback)
    {
        LF_STATIC_IS_A(EntryT, Entry);
        return Select(table, entryID, sizeof(EntryT), alignof(EntryT),
            [](const ByteT* item, void* userData) -> void
            {
                SelectCallbackT& selectCallback = *reinterpret_cast<SelectCallbackT*>(userData);
                selectCallback(reinterpret_cast<const EntryT*>(item));
            },
            &selectCallback);
    }

    template<typename EntryT, typename SelectCallbackT>
    bool SelectWrite(TableID table, EntryID entryID, SelectCallbackT selectCallback)
    {
        LF_STATIC_IS_A(EntryT, Entry);
        return Select(table, entryID, sizeof(EntryT), alignof(EntryT),
            [](ByteT* item, void* userData) -> void
            {
                SelectCallbackT& selectCallback = *reinterpret_cast<SelectCallbackT*>(userData);
                selectCallback(reinterpret_cast<EntryT*>(item));
            },
            &selectCallback);
    }

    MemDBStats GetStats() const;
    MemDBStats GetTableStats(const String& table) const;
    MemDBStats GetTableStats(TableID table) const;
    // ********************************************************************
    // You can configure the internal 'free list' cache size to reduce the 
    // number of re-allocs & copying.
    // ********************************************************************
    void SetTableFreeCache(TableID table, SizeT cacheSize);
private:
    using TablePtr = TStrongPointer<Table>;

    MemDB(const MemDB&) = delete;
    MemDB& operator=(const MemDB&) = delete;

    Table* GetTable(TableID index);
    const Table* GetTable(TableID index) const;

    Entry* SelectUsedEntry(Table* t, SizeT size, SizeT alignment, EntryID entryID);

    void AllocateID(Table* t, ByteT*& ptr, EntryID& outID);
    bool TryBulkInsert(Table* table, const Entry* entryData, SizeT numEntries, TVector<EntryID>& outID);
    void CleanUpBulkInsert(Table* table, TVector<EntryID>& outID);
    
    mutable RWSpinLock mLock;
    TVector<TablePtr>   mTables;
    String             mFilePath;

    volatile Atomic64  mDataBytesReserved;
    volatile Atomic64  mDataBytesUsed;
    volatile Atomic64  mRuntimeBytesUsed;
    volatile Atomic64  mRuntimeBytesReserved;

};

template<typename CharT, SizeT CArrayLength>
MemDBStringType<CharT, CArrayLength>::MemDBStringType()
: mData()
{
    Clear();
}
template<typename CharT, SizeT CArrayLength>
MemDBStringType<CharT, CArrayLength>::MemDBStringType(const CharT* string)
: mData()
{
    Assign(string);
}

template<typename CharT, SizeT CArrayLength>
void MemDBStringType<CharT, CArrayLength>::Clear()
{
    memset(mData, 0, CArrayLength);
}

template<typename CharT, SizeT CArrayLength>
void MemDBStringType<CharT, CArrayLength>::Assign(const CharT* string)
{
    if (!string || *string == '\0')
    {
        Clear();
        return;
    }

    for (SizeT i = 0; i < (CArrayLength - 1); ++i)
    {
        mData[i] = *string;
        ++string;
        if (*string == '\0')
        {
            mData[i + 1] = '\0';
            break;
        }
    }
}
template<typename CharT, SizeT CArrayLength>
void MemDBStringType<CharT, CArrayLength>::Append(const CharT* string)
{
    if (!string || *string == '\0')
    {
        return;
    }

    for (SizeT i = Size(); i < (CArrayLength - 1); ++i)
    {
        mData[i] = *string;
        ++string;
        if (*string == '\0')
        {
            mData[i + 1] = '\0';
            break;
        }
    }
}

template<typename CharT, SizeT CArrayLength>
bool MemDBStringType<CharT, CArrayLength>::Equals(const CharT* string) const
{
    if (!string || *string == '\0')
    {
        return Empty();
    }

    const CharT* it = mData;
    while (*it != '\0' && *string != '\0')
    {
        if (*it != *string)
        {
            return false;
        }
        ++it;
        ++string;
    }

    return *it == *string;
}

} // namespace lf 
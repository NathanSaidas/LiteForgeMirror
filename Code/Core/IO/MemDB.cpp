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
#include "Core/PCH.h"
#include "MemDB.h"
#include "Core/IO/JsonStream.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/MappedFile.h"
#include "Core/Utility/Crc32.h"
#include "Core/Utility/Log.h"
#include <algorithm>


namespace lf
{
DECLARE_PTR(File);


namespace MemDBTypes
{

struct EntryIndex
{
    bool operator==(const EntryIndex& other) const
    {
        return mValue == other.mValue;
    }

    bool operator<(const EntryIndex& other) const
    {
        return mValue < other.mValue;
    }
    bool operator>(const EntryIndex& other) const
    {
        return mValue > other.mValue;
    }

    NumericalVariant mValue;
    EntryID          mID;
};

struct TableIndex
{
    FilePtr                         mFileHandle;

    // ** The offset in data where the data is located.
    SizeT                           mOffset;
    // ** How to interpret the data.
    NumericalVariant::VariantType   mDataType;
    // ** 
    TVector<EntryIndex>              mCollection;
    // ** 
    bool                            mSorted;

    bool                            mAllowDuplicates;
};

struct Table
{
    Table()
    : mName()
    , mEntrySize(0)
    , mEntryAlignment(0)
    , mEntryCapacity(0)
    , mBase(nullptr)
    , mEnd(nullptr)
    , mIndices()
    , mScratchEntry(nullptr)
    , mNextFree(0)
    , mFreeList()
    , mOpCounts{ 0 }
    , mCount(0)
    , mPendingWrites(0)
    , mResizeCount(0)
    {}
    // ** The name of the table
    String mName;
    // ** The size of each entry in the table (including the base Entry type)
    SizeT  mEntrySize;
    // ** 
    SizeT  mEntryAlignment;
    // ** The capacity of the table
    SizeT  mEntryCapacity;
    // ** Pointer to the beginning of the table.
    ByteT* mBase;
    // ** Pointer to the end of the table.
    ByteT* mEnd;

    TVector<TableIndex> mIndices;
    // ** Pointer to memory used for storing a 'prev' for delta checks
    ByteT* mScratchEntry;

    // ** [OPTIMIZATION], reference the possible 'next' item.
    EntryID mNextFree;
    // ** [OPTIMIZATION], keep track of released entries, to recycle later.
    TVector<EntryID> mFreeList;
    // ** [OPTIMIZATION], keep track of dirty entries to flush
    TVector<EntryID> mDirtyEntries;

    mutable volatile Atomic64 mOpCounts[MemDBTypes::OpTypes::MAX_VALUE];

    // ** The number of elements added to this table
    SizeT mCount;
    // ** The number of writes waiting to be commited.
    SizeT mPendingWrites;
    // ** [Optional] Open/Locked file handle for database read/write operations
    FilePtr mFileHandle;
    // ** The number of times the table had to be resized.
    SizeT mResizeCount;
};

enum EntryFlag
{
    EF_USED = 1 << 0,
    EF_DIRTY = 1 << 1
};

} // namespace MemDBTypes

static bool EntryUsed(const MemDBTypes::Entry& entry)
{
    return (entry.mReservedFlags & MemDBTypes::EF_USED) > 0;
}
static bool EntryDirty(const MemDBTypes::Entry& entry)
{
    return (entry.mReservedFlags & MemDBTypes::EF_DIRTY) > 0;
}
static void SetFlag(MemDBTypes::Entry& entry, MemDBTypes::EntryFlag flag)
{
    entry.mReservedFlags = entry.mReservedFlags | flag;
}
static void UnsetFlag(MemDBTypes::Entry& entry, MemDBTypes::EntryFlag flag)
{
    entry.mReservedFlags = entry.mReservedFlags & (~flag);
}

static void TableOp(const MemDBTypes::Table& table, MemDBTypes::OpTypes::Value type)
{
    AtomicIncrement64(&table.mOpCounts[type]);
}

static SizeT TableByteCapacity(MemDBTypes::Table& table)
{
    return table.mEntryCapacity * table.mEntrySize;
}

static void TableAlloc(MemDBTypes::Table& table)
{
    SizeT capacity = TableByteCapacity(table);
    table.mBase = static_cast<ByteT*>(LFAlloc(capacity, table.mEntryAlignment));
    table.mEnd = table.mBase + capacity;

    memset(table.mBase, 0, capacity);

    ByteT* ptr = table.mBase;
    for (SizeT i = 0; i < table.mEntryCapacity; ++i)
    {
        MemDBTypes::Entry entry;
        entry.mReservedID = static_cast<UInt32>(i);
        entry.mReservedFlags = UInt32(0);
        memcpy(ptr, &entry, sizeof(entry));
        ptr += table.mEntrySize;
    }

    if (table.mScratchEntry == nullptr)
    {
        table.mScratchEntry = static_cast<ByteT*>(LFAlloc(table.mEntrySize, table.mEntryAlignment));
    }
}

static void TableRelease(MemDBTypes::Table& table)
{
    if (table.mBase != nullptr)
    {
        LFFree(table.mBase);
        table.mBase = nullptr;
        table.mEnd = nullptr;
    }

    if (table.mScratchEntry != nullptr)
    {
        LFFree(table.mScratchEntry);
        table.mScratchEntry = nullptr;
    }
}

static void TableAllocCopy(MemDBTypes::Table& oldTable, MemDBTypes::Table& newTable)
{
    Assert(oldTable.mEntrySize == newTable.mEntrySize || oldTable.mEntryAlignment == newTable.mEntryAlignment)
    TableAlloc(newTable);
    SizeT capacity = TableByteCapacity(oldTable);
    memcpy(newTable.mBase, oldTable.mBase, capacity);
}

static bool TableGetEntry(MemDBTypes::Table& table, MemDBTypes::EntryID id, MemDBTypes::Entry& outEntry)
{
    if (id >= table.mEntryCapacity)
    {
        return false;
    }
    ByteT* ptr = table.mBase;
    ptr += (table.mEntrySize * id);
    memcpy(&outEntry, ptr, sizeof(MemDBTypes::Entry));
    return true;
}

// ** Use this before attempting to read from table (to ensure its sorted)
static void TableIndexReadBarrier(MemDBTypes::TableIndex& index)
{
    if (!index.mSorted)
    {
        std::sort(index.mCollection.begin(), index.mCollection.end());
        index.mSorted = true;
    }
}
// ** Use this after writing to barrier to ensure the sorted flags are set properly.
static void TableIndexWriteBarrier(MemDBTypes::TableIndex& index, bool doSort)
{
    if (doSort)
    {
        std::sort(index.mCollection.begin(), index.mCollection.end());
        index.mSorted = true;
    }
    else
    {
        index.mSorted = false;
    }
}

// ** Check the table to see if an entry can be inserted or if it will break the 'unique' rules required for
// an index.
static bool TableCheckIndex(MemDBTypes::Table& table, const ByteT* entryBytes)
{
    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        if (index.mAllowDuplicates)
        {
            continue;
        }

        // Ignore special case (IDs are always unique)
        if (index.mOffset == 0)
        {
            continue;
        }
        const ByteT* member = entryBytes + index.mOffset;
        NumericalVariant value = NumericalVariant::Cast(index.mDataType, member);

        MemDBTypes::EntryIndex searchKey;
        searchKey.mValue = value;
        TableIndexReadBarrier(index);

        auto it = std::lower_bound(index.mCollection.begin(), index.mCollection.end(), searchKey);
        auto result = (it != index.mCollection.end() && it->mValue == value) ? it : index.mCollection.end();
        if (result != index.mCollection.end())
        {
            return false;
        }
    }
    return true;
}

static void TableInsertIndex(MemDBTypes::Table& table, MemDBTypes::EntryID id, const ByteT* entryBytes, bool doSort = true)
{
    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        const ByteT* member = entryBytes + index.mOffset;
        NumericalVariant value = NumericalVariant::Cast(index.mDataType, member);

        MemDBTypes::EntryIndex indexValue;
        indexValue.mValue = value;
        indexValue.mID = id;

        index.mCollection.push_back(indexValue);
        TableIndexWriteBarrier(index, doSort);
    }
}

static void TableUpdateIndex(MemDBTypes::Table& table, MemDBTypes::EntryID id, const ByteT* beforeBytes, const ByteT* entryBytes, bool doSort = true)
{
    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        const ByteT* member = entryBytes + index.mOffset;
        const ByteT* beforePtr = beforeBytes + index.mOffset;
        NumericalVariant value = NumericalVariant::Cast(index.mDataType, member);
        NumericalVariant before = NumericalVariant::Cast(index.mDataType, beforePtr);

        TableIndexReadBarrier(index);

        MemDBTypes::EntryIndex searchKey;
        searchKey.mValue = before;

        auto it = std::lower_bound(index.mCollection.begin(), index.mCollection.end(), searchKey);
        auto result = (it != index.mCollection.end() && it->mValue == value) ? it : index.mCollection.end();
        if (result != index.mCollection.end())
        {
            if (result->mValue != value)
            {
                index.mCollection.swap_erase(result);
                TableIndexWriteBarrier(index, doSort);
            }
        }
        else
        {
            it = std::find_if(
                index.mCollection.begin(),
                index.mCollection.end(),
                [id](const MemDBTypes::EntryIndex& entryIndex)
                {
                    return entryIndex.mID == id;
                });

            if (it != index.mCollection.end() && it->mValue != value)
            {
                index.mCollection.swap_erase(it);
                TableIndexWriteBarrier(index, doSort);
            }
        }
    }
}

static void TableRemoveIndex(MemDBTypes::Table& table, MemDBTypes::EntryID id, bool doSort = true)
{
    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        auto it = std::find_if(
                        index.mCollection.begin(), 
                        index.mCollection.end(), 
                        [id](const MemDBTypes::EntryIndex& entryIndex) 
                        { 
                            return entryIndex.mID == id; 
                        });

        if (it != index.mCollection.end())
        {
            index.mCollection.swap_erase(it);
            TableIndexWriteBarrier(index, doSort);
        }
    }
}

// ** Ensure file handles are open
static bool TableOpenFiles(const String& rootPath, MemDBTypes::Table& table)
{
    if (!table.mFileHandle)
    {
        table.mFileHandle = FilePtr(LFNew<File>());
        String fullpath = rootPath + "_" + table.mName + ".db";
        if (!table.mFileHandle->Open(fullpath, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
        {
            gSysLog.Error(LogMessage("Failed to open path for db table ") << fullpath);
            return false;
        }
    }

    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        if (!index.mFileHandle)
        {
            String name = "_" + table.mName + "_" + TNumericalVariantType(index.mDataType).GetString() + "_" + ToString(index.mOffset) + ".idx";
            index.mFileHandle = FilePtr(LFNew<File>());
            String fullpath = rootPath + name;
            if (!index.mFileHandle->Open(fullpath, FF_READ | FF_WRITE, FILE_OPEN_ALWAYS))
            {
                gSysLog.Error(LogMessage("Failed to open path for db index ") << fullpath);
                return false;
            }
        }
    }

    return true;
}

// ** Ensure file handles are closed.
static bool TableCloseFiles(MemDBTypes::Table& table)
{
    table.mFileHandle.Release();
    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        index.mFileHandle.Release();
    }
    return true;
}

static void TableSave(MemDBTypes::Table& table, MemDB::SaveMode mode)
{
    if (!table.mFileHandle)
    {
        return;
    }

    if (!table.mBase)
    {
        return;
    }

    if (mode == MemDB::SAVE_FULL)
    {
        ByteT* ptr = table.mBase;
        for (SizeT i = 0; i < table.mEntryCapacity; ++i)
        {
            MemDBTypes::Entry* entry = reinterpret_cast<MemDBTypes::Entry*>(ptr);
            UnsetFlag(*entry, MemDBTypes::EF_DIRTY);
            ptr += table.mEntrySize;
        }

        table.mFileHandle->SetCursor(0, FILE_CURSOR_BEGIN);
        table.mFileHandle->Write(table.mBase, table.mEnd - table.mBase);
    }
    else if (mode == MemDB::SAVE_DIRTY)
    {
        ByteT* ptr = table.mBase;
        for (SizeT i = 0; i < table.mEntryCapacity; ++i)
        {
            MemDBTypes::Entry* entry = reinterpret_cast<MemDBTypes::Entry*>(ptr);
            if (EntryDirty(*entry))
            {
                UnsetFlag(*entry, MemDBTypes::EF_DIRTY);
                table.mFileHandle->SetCursor(static_cast<FileCursor>(ptr - table.mBase), FILE_CURSOR_BEGIN);
                table.mFileHandle->Write(ptr, table.mEntrySize);
            }
            ptr += table.mEntrySize;
        }
    }
    else if (mode == MemDB::SAVE_DIRTY_LIST)
    {
        for (MemDBTypes::EntryID id : table.mDirtyEntries)
        {
            ByteT* ptr = table.mBase + (id * table.mEntrySize);
            MemDBTypes::Entry* entry = reinterpret_cast<MemDBTypes::Entry*>(ptr);
            UnsetFlag(*entry, MemDBTypes::EF_DIRTY);
            table.mFileHandle->SetCursor(static_cast<FileCursor>(ptr - table.mBase), FILE_CURSOR_BEGIN);
            table.mFileHandle->Write(ptr, table.mEntrySize);
        }
    }

    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        if (index.mFileHandle && !index.mCollection.data())
        {
            index.mFileHandle->SetCursor(0, FILE_CURSOR_BEGIN);
            index.mFileHandle->Write(index.mCollection.data(), index.mCollection.size() * sizeof(MemDBTypes::EntryIndex));
        }
    }

    table.mDirtyEntries.resize(0);
    table.mPendingWrites = 0;
}

static void TableLoad(MemDBTypes::Table& table)
{
    if (!table.mFileHandle)
    {
        return;
    }

    SizeT fileSize = static_cast<SizeT>(table.mFileHandle->GetSize());
    if (fileSize == 0)
    {
        return;
    }
    SizeT capacity = fileSize / table.mEntrySize;
    ReportBug(fileSize == capacity * table.mEntrySize);
    // Allocate space.
    if (capacity != table.mEntryCapacity)
    {
        if (table.mBase)
        {
            LFFree(table.mBase);
            table.mBase = nullptr;
        }

        table.mBase = static_cast<ByteT*>(LFAlloc(capacity * table.mEntrySize, table.mEntryAlignment));
        table.mEnd = table.mBase + (capacity * table.mEntrySize);
        table.mEntryCapacity = capacity;
    }
    
    table.mFileHandle->SetCursor(0, FILE_CURSOR_BEGIN);
    table.mFileHandle->Read(table.mBase, capacity * table.mEntrySize);

    table.mCount = 0;

    ByteT* ptr = table.mBase;
    for (SizeT i = 0; i < table.mEntryCapacity; ++i)
    {
        MemDBTypes::Entry* entry = reinterpret_cast<MemDBTypes::Entry*>(ptr);
        if (EntryUsed(*entry))
        {
            ++table.mCount;
        }
        ptr += table.mEntrySize;
    }

    for (MemDBTypes::TableIndex& index : table.mIndices)
    {
        fileSize = static_cast<SizeT>(index.mFileHandle->GetSize());
        capacity = fileSize / sizeof(MemDBTypes::EntryIndex);
        Assert(fileSize == capacity * sizeof(MemDBTypes::EntryIndex));
        if (capacity > 0)
        {
            index.mCollection.resize(capacity);
            index.mFileHandle->SetCursor(0, FILE_CURSOR_BEGIN);
            index.mFileHandle->Read(index.mCollection.data(), capacity * sizeof(MemDBTypes::EntryIndex));
        }

        index.mSorted = false;
    }
}

bool TableWriteFullBinary(File& file, MemDBTypes::Table& table)
{
    SizeT numBytes = table.mEnd - table.mBase;
    return file.SetCursor(0, FILE_CURSOR_BEGIN) && file.Write(table.mBase, numBytes) == numBytes;
}

bool TableReadFullBinary(File& file, MemDBTypes::Table& table)
{
    SizeT currentSize = table.mEnd - table.mBase;
    SizeT size = static_cast<SizeT>(file.GetSize());
    if (size == 0)
    {
        return false;
    }

    if (size != currentSize)
    {
        if (table.mBase)
        {
            LFFree(table.mBase);
            table.mBase = table.mEnd = nullptr;
        }
        table.mBase = static_cast<ByteT*>(LFAlloc(size, table.mEntryAlignment));
        table.mEnd = table.mBase + size;
    }

    if (file.Read(table.mBase, size) != size)
    {
        return false;
    }

    if (!table.mScratchEntry)
    {
        table.mScratchEntry = static_cast<ByteT*>(LFAlloc(table.mEntrySize, table.mEntryAlignment));
    }

    return true;
}

MemDB::MemDB()
: mLock()
, mTables()
, mDataBytesReserved(0)
, mDataBytesUsed(0)
, mRuntimeBytesUsed(0)
, mRuntimeBytesReserved(0)
{

}

MemDB::~MemDB()
{
    Release();
}

void MemDB::WriteToFile(TableID index, const String& filename, bool fullFlush)
{
    ScopeRWSpinLockRead lock(mLock);
    Table* t = GetTable(index);
    if (!t)
    {
        return;
    }
    
    File file;
    if (!file.Open(filename, FF_WRITE | FF_SHARE_READ | FF_RANDOM_ACCESS, FILE_OPEN_ALWAYS))
    {
        return;
    }

    SizeT requiredSize = TableByteCapacity(*t);
    if (!file.SetCursor(static_cast<FileCursor>(requiredSize), FILE_CURSOR_BEGIN, true))
    {
        return;
    }
    

    if (fullFlush)
    {
        ByteT* ptr = t->mBase;
        for (SizeT i = 0; i < t->mEntryCapacity; ++i)
        {
            if (EntryDirty(*reinterpret_cast<Entry*>(ptr)))
            {
                if (file.SetCursor(static_cast<FileCursor>(t->mEntrySize * i), FILE_CURSOR_BEGIN))
                {
                    UnsetFlag(*reinterpret_cast<Entry*>(ptr), MemDBTypes::EF_DIRTY);
                    file.Write(ptr, t->mEntrySize);
                }
            }
            ptr += t->mEntrySize;
        }
    }
    else
    {
        for (EntryID id : t->mDirtyEntries)
        {
            ByteT* ptr = t->mBase + (t->mEntrySize * id);
            if (EntryDirty(*reinterpret_cast<Entry*>(ptr)))
            {
                if (file.SetCursor(static_cast<FileCursor>(t->mEntrySize * id), FILE_CURSOR_BEGIN))
                {
                    UnsetFlag(*reinterpret_cast<Entry*>(ptr), MemDBTypes::EF_DIRTY);
                    file.Write(ptr, t->mEntrySize);
                }
            }
            ptr += t->mEntrySize;
        }

    }
    file.Close();

    t->mPendingWrites = 0;
    t->mDirtyEntries.resize(0);
}

void MemDB::ReadFromFile(TableID index, const String& filename)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(index);
    if (!t)
    {
        return;
    }

    File file;
    if (!file.Open(filename, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        return;
    }

    file.Read(t->mBase, TableByteCapacity(*t));
    file.Close();


    // ByteT* ptr = t->mBase;
    // for (SizeT i = 0; i < t->mEntryCapacity; ++i)
    // {
    //     Entry entry;
    //     TableGetEntry(*t, static_cast<EntryID>(i), entry);
    //     if (EntryDirty(entry))
    //     {
    //         if (file.SetCursor(static_cast<FileCursor>(t->mEntrySize * i), FILE_CURSOR_BEGIN))
    //         {
    //             file.Write(ptr, t->mEntrySize);
    //         }
    //     }
    //     ptr += t->mEntrySize;
    // }
    // 
    // file.Close();
}

void MemDB::Open(const String& filename)
{
    ScopeRWSpinLockWrite lock(mLock);
    mFilePath = filename;

    for (Table* tbl : mTables)
    {
        if (tbl->mName.Empty())
        {
            continue;
        }

        TableOpenFiles(mFilePath, *tbl);
    }

}

void MemDB::Close()
{
    ScopeRWSpinLockWrite lock(mLock);
    for (Table* tbl : mTables)
    {
        if (tbl->mName.Empty())
        {
            continue;
        }

        TableCloseFiles(*tbl);
    }
}

void MemDB::Save(SaveMode mode)
{
    if (mFilePath.Empty())
    {
        return;
    }

    ScopeRWSpinLockRead lock(mLock);
    for (Table* tbl : mTables)
    {
        if (tbl->mName.Empty())
        {
            continue;
        }

        TableSave(*tbl, mode);
    }

    String fullpath = mFilePath + ".json";
    String text;
    JsonStream js(Stream::TEXT, &text, Stream::SM_PRETTY_WRITE);
    js.BeginObject("MemDB", "Native");

    for (Table* table : mTables)
    {
        if (table->mName.Empty())
        {
            continue;
        }
        js.Serialize(StreamPropertyInfo(table->mName));
        js.BeginStruct();
            SERIALIZE_NAMED(js, "EntrySize", table->mEntrySize, "");
            SERIALIZE_NAMED(js, "EntryAlignment", table->mEntryAlignment, "");
            SERIALIZE_NAMED(js, "EntryCapacity", table->mEntryCapacity, "");
            SERIALIZE_NAMED(js, "Count", table->mCount, "");

            js.Serialize(StreamPropertyInfo("Indices"));
            js.BeginArray();
            for (MemDBTypes::TableIndex& index : table->mIndices)
            {
                js.BeginStruct();
            
                TNumericalVariantType dataType(index.mDataType);
                SERIALIZE_NAMED(js, "DataType", dataType, "");
                SERIALIZE_NAMED(js, "Offset", index.mOffset, "");
                SERIALIZE_NAMED(js, "AllowDuplicates", index.mAllowDuplicates, "");

                js.EndStruct();
            }
            js.EndArray();
        js.EndStruct();
        
    }

    js.EndObject();
    js.Close();

    File file;
    file.Open(fullpath, FF_READ | FF_WRITE | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_CREATE_NEW);
    file.Write(text.CStr(), text.Size());
    file.Close();
}

void MemDB::Load()
{
    if (mFilePath.Empty())
    {
        return;
    }

    ScopeRWSpinLockRead lock(mLock);
    for (Table* tbl : mTables)
    {
        if (tbl->mName.Empty())
        {
            continue;
        }

        TableLoad(*tbl);
    }
}

void MemDB::CommitDirty(TableID table, MemDBTypes::EntryWriter* writer, SaveMode mode)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);

    if (!writer->BeginCommit(TableByteCapacity(*t), t->mEntryAlignment))
    {
        writer->EndCommit();
        return;
    }

    switch (mode)
    {
        case SaveMode::SAVE_DIRTY:
        {

            for (SizeT i = 0; i < t->mEntryCapacity; ++i)
            {
                ByteT* entryBytes = t->mBase + (i * t->mEntrySize);
                Entry* entry = reinterpret_cast<Entry*>(entryBytes);
                const bool dirty = EntryDirty(*entry);
                if (dirty)
                {
                    UnsetFlag(*entry, MemDBTypes::EF_DIRTY);
                    writer->Commit(entryBytes, t->mEntrySize, t->mEntryAlignment, i * t->mEntrySize);
                }
            }

            t->mDirtyEntries.clear();
            t->mPendingWrites = 0;
        } break;
        case SaveMode::SAVE_DIRTY_LIST:
        {
            Assert(t->mDirtyEntries.size() == t->mPendingWrites);

            for (EntryID entryID : t->mDirtyEntries)
            {
                SizeT index = static_cast<SizeT>(entryID);
                ByteT* entryBytes = t->mBase + (index * t->mEntrySize);
                Entry* entry = reinterpret_cast<Entry*>(entryBytes);
                const bool dirty = EntryDirty(*entry);
                if (dirty)
                {
                    UnsetFlag(*entry, MemDBTypes::EF_DIRTY);
                    writer->Commit(entryBytes, t->mEntrySize, t->mEntryAlignment, index * t->mEntrySize);
                }
            }

            t->mDirtyEntries.clear();
            t->mPendingWrites = 0;
        } break;
    }

    writer->EndCommit();
}

bool MemDB::LoadTableData(TableID table, const ByteT* bytes, SizeT numBytes)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);

    // Table Validation: (All 'entries' must be valid)
    const SizeT numEntries = numBytes / t->mEntrySize;
    EntryID nextFree = MemDBTypes::INVALID_ENTRY_ID;
    SizeT numUsed = 0;
    for (SizeT i = 0; i < numEntries; ++i)
    {
        const Entry* entry = reinterpret_cast<const Entry*>(bytes + (t->mEntrySize * i));
        if (entry->mReservedID != i)
        {
            return false; // Id must match index!
        }

        const bool used = EntryUsed(*entry);
        if (Invalid(nextFree) && !used)
        {
            nextFree = entry->mReservedID;
        }

        if (used)
        {
            ++numUsed;
        }
    }

    // Validate Indices
    TVector<NumericalVariant> indexItems;
    indexItems.reserve(numEntries);
    for (MemDBTypes::TableIndex& index : t->mIndices)
    {
        if (index.mAllowDuplicates)
        {
            continue;
        }

        indexItems.resize(0);
        for (SizeT i = 0; i < numEntries; ++i)
        {
            const ByteT* entryBytes = bytes + (t->mEntrySize * i);
            if (!EntryUsed(*reinterpret_cast<const Entry*>(entryBytes)))
            {
                continue;
            }
            const ByteT* itemBytes = entryBytes + index.mOffset;
            indexItems.push_back(NumericalVariant::Cast(index.mDataType, itemBytes));
        }

        std::sort(indexItems.begin(), indexItems.end());
        for (const NumericalVariant& variant : indexItems)
        {
            auto first = std::lower_bound(indexItems.begin(), indexItems.end(), variant);
            auto last = std::lower_bound(indexItems.begin(), indexItems.end(), variant);
            if (std::distance(first, last) > 1)
            {
                return false; // Can only contain unique items!
            }
        }
    }

    // Initialize the table
    TableRelease(*t);
    AtomicSub64(&mDataBytesUsed, t->mCount * t->mEntrySize);
    t->mEntryCapacity = numBytes / t->mEntrySize;
    TableAlloc(*t);
    memcpy(t->mBase, bytes, numBytes);

    // Create indices
    for (MemDBTypes::TableIndex& index : t->mIndices)
    {
        index.mCollection.clear();
        index.mCollection.reserve(numEntries);
        
        for (SizeT i = 0; i < numEntries; ++i)
        {
            const ByteT* entryBytes = bytes + (t->mEntrySize * i);
            const Entry* entry = reinterpret_cast<const Entry*>(entryBytes);
            if (!EntryUsed(*entry))
            {
                continue;
            }
            const ByteT* itemBytes = entryBytes + index.mOffset;

            MemDBTypes::EntryIndex item;
            item.mID = entry->mReservedID;
            item.mValue = NumericalVariant::Cast(index.mDataType, itemBytes);
            index.mCollection.push_back(item);
        }

        std::sort(index.mCollection.begin(), index.mCollection.end());
        index.mSorted = true;
    }

    t->mNextFree = nextFree;
    t->mFreeList.clear();
    t->mDirtyEntries.clear();
    t->mPendingWrites = 0;
    t->mCount = numUsed;
    // TODO: We can optionally clear stats.
    return true;
}

void MemDB::Release()
{
    ScopeRWSpinLockWrite lock(mLock);
    for (Table* tbl : mTables)
    {
        TableRelease(*tbl);
    }
    mTables.clear();

    mDataBytesReserved = 0;
    mDataBytesUsed = 0;
    AtomicRWBarrier();
}

bool MemDB::CreateTable(const String& name, SizeT entrySize, SizeT entryAlignment, TableID& outIndex)
{
    if (entrySize >= ToKB<SizeT>(8)) // can't use default capacity, specify
    {
        return false;
    }

    SizeT capacity = ToKB<SizeT>(8) / entrySize;
    return CreateTable(name, entrySize, entryAlignment, capacity, outIndex);
}

bool MemDB::CreateTable(const String& name, SizeT entrySize, SizeT entryAlignment, SizeT entryCapacity, TableID& outIndex)
{
    if (entryCapacity > ToGB<SizeT>(1))
    {
        return false;
    }

    if (name.Empty())
    {
        return false;
    }

    if (entrySize <= sizeof(Entry))
    {
        return false;
    }

    if (entryAlignment < alignof(Entry))
    {
        return false;
    }

    if ((entrySize % entryAlignment) != 0) // Must align to size, could possibly compress the data... but anyways.
    {
        return false;
    }

    TableID exists;
    if (FindTable(name, exists))
    {
        return false;
    }

    ScopeRWSpinLockWrite lock(mLock);

    // Recycle:
    Table* table = nullptr;
    for (SizeT i = 0; i < mTables.size(); ++i)
    {
        if (mTables[i]->mName.Empty())
        {
            table = mTables[i];
            outIndex = static_cast<TableID>(i);
            break;
        }
    }
    // Allocate:
    if (!table)
    {
        outIndex = static_cast<TableID>(mTables.size());
        mTables.push_back(TablePtr(LFNew<Table>()));
        table = mTables.back();
    }
    table->mName = name;
    table->mEntryCapacity = entryCapacity;
    table->mEntryAlignment = entryAlignment;
    table->mEntrySize = entrySize;
    TableAlloc(*table);
    if (!mFilePath.Empty())
    {
        TableOpenFiles(mFilePath, *table);
    }
    AtomicAdd64(&mDataBytesReserved, static_cast<Atomic64>(TableByteCapacity(*table)));
    AtomicAdd64(&mRuntimeBytesReserved, static_cast<Atomic64>(table->mEntrySize));
    AtomicAdd64(&mRuntimeBytesUsed, static_cast<Atomic64>(table->mEntrySize));
    return true;
}
bool MemDB::DeleteTable(const String& name)
{
    TableID index;
    return FindTable(name, index) && DeleteTable(index);
}
bool MemDB::DeleteTable(TableID index)
{
    ScopeRWSpinLockWrite lock(mLock);
    if (index >= mTables.size())
    {
        return false;
    }
    AtomicSub64(&mDataBytesReserved, static_cast<Atomic64>(TableByteCapacity(*mTables[index])));
    AtomicSub64(&mRuntimeBytesReserved, static_cast<Atomic64>(mTables[index]->mEntrySize));
    AtomicSub64(&mRuntimeBytesUsed, static_cast<Atomic64>(mTables[index]->mEntrySize));
    TableCloseFiles(*mTables[index]);
    TableRelease(*mTables[index]);
    *mTables[index] = Table();
    return true;
}
bool MemDB::FindTable(const String& name, TableID& outIndex) const
{
    if (name.Empty())
    {
        return false;
    }

    ScopeRWSpinLockRead lock(mLock);
    for(SizeT i = 0; i < mTables.size(); ++i)
    {
        if (mTables[i]->mName == name)
        {
            outIndex = static_cast<TableID>(i);
            return true;
        }
    }
    return false;

}

bool MemDB::CreateIndex(TableID table, NumericalVariant::VariantType dataType, SizeT dataOffset, bool allowDuplicates)
{
    using namespace MemDBTypes;

    SizeT dataSize = NumericalVariant::GetSize(dataType);
    if (dataSize == 0)
    {
        return false;
    }

    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }

    for (const MemDBTypes::TableIndex& index : t->mIndices)
    {
        if (index.mOffset == dataOffset)
        {
            return false;
        }
    }

    SizeT boundsCheck = t->mEntrySize;
    if (dataOffset > boundsCheck || (dataOffset + dataSize) > boundsCheck)
    {
        return false;
    }


    t->mIndices.push_back(MemDBTypes::TableIndex());
    MemDBTypes::TableIndex& tblIndex = t->mIndices.back();
    tblIndex.mDataType = dataType;
    tblIndex.mOffset = dataOffset;
    tblIndex.mSorted = false;
    tblIndex.mAllowDuplicates = allowDuplicates;
    if (!mFilePath.Empty())
    {
        TableOpenFiles(mFilePath, *t);
    }

    //
    ByteT* ptr = t->mBase;
    for (SizeT i = 0; i < t->mEntryCapacity; ++i)
    {
        Entry entry;
        memcpy(&entry, ptr, sizeof(Entry));
        Assert(entry.mReservedID == i);
        if (EntryUsed(entry))
        {
            ByteT* dataPtr = ptr + tblIndex.mOffset;
            NumericalVariant variant = NumericalVariant::Cast(tblIndex.mDataType, dataPtr);

            EntryIndex entryIndex;
            entryIndex.mValue = variant;
            entryIndex.mID = entry.mReservedID;

            tblIndex.mCollection.push_back(entryIndex);
        }
        
        ptr += t->mEntrySize;
    }
    
    TableIndexWriteBarrier(tblIndex, true);

    if (!tblIndex.mCollection.empty())
    {
        auto first = tblIndex.mCollection.begin();
        for (auto it = first + 1; it != tblIndex.mCollection.end(); ++it)
        {
            if (!(*first < *it))
            {
                tblIndex.mCollection.clear();
                return false;
            }
        }
    }

    return true;
}

bool MemDB::FindOne(TableID table, SizeT entrySize, SizeT entryAlignment, EntryFindCallback findCallback, void* findUserData, EntryID& outID) const
{
    ScopeRWSpinLockRead lock(mLock);
    const Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_FIND_ONE);

    if (t->mEntrySize != entrySize || t->mEntryAlignment != entryAlignment)
    {
        return false;
    }

    const ByteT* ptr = t->mBase;
    for (SizeT i = 0; i < t->mEntryCapacity; ++i)
    {
        MemDBTypes::Entry entry;
        memcpy(&entry, ptr,sizeof(entry));
        if (EntryUsed(entry))
        {
            if (findCallback(ptr, findUserData))
            {
                outID = entry.mReservedID;
                return true;
            }
        }

        ptr += t->mEntrySize;
    }
    return false;
}

bool MemDB::FindOneIndexed(TableID table, NumericalVariant value, SizeT dataOffset, EntryID& outID)
{
    using namespace MemDBTypes;
    ScopeRWSpinLockRead lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_FIND_ONE_INDEXED);

    TableIndex* tblIndex = nullptr;
    for (TableIndex& index : t->mIndices)
    {
        if (index.mOffset == dataOffset)
        {
            tblIndex = &index;
            break;
        }
    }

    if (tblIndex == nullptr)
    {
        return false;
    }

    if (tblIndex->mDataType != value.mType) // Type mismatch
    {
        return false; 
    }
    EntryIndex searchKey;
    searchKey.mValue = value;
    TableIndexReadBarrier(*tblIndex);

    auto it = std::lower_bound(tblIndex->mCollection.begin(), tblIndex->mCollection.end(), searchKey);
    auto result = (it != tblIndex->mCollection.end() && it->mValue == value) ? it : tblIndex->mCollection.end();
    if (result != tblIndex->mCollection.end())
    {
        outID = result->mID;
        return true;
    }
    return false;
}

bool MemDB::FindRangeIndexed(TableID table, NumericalVariant value, SizeT dataOffset, TVector<EntryID>& outIDs)
{
    using namespace MemDBTypes;
    ScopeRWSpinLockRead lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_FIND_RANGE_INDEXED);

    TableIndex* tblIndex = nullptr;
    for (TableIndex& index : t->mIndices)
    {
        if (index.mOffset == dataOffset)
        {
            tblIndex = &index;
            break;
        }
    }

    if (tblIndex == nullptr)
    {
        return false;
    }

    if (tblIndex->mDataType != value.mType) // Type mismatch
    {
        return false;
    }

    EntryIndex searchKey;
    searchKey.mValue = value;
    TableIndexReadBarrier(*tblIndex);

    auto it = std::lower_bound(tblIndex->mCollection.begin(), tblIndex->mCollection.end(), searchKey);
    auto result = (it != tblIndex->mCollection.end() && it->mValue == value) ? it : tblIndex->mCollection.end();
    if (result == tblIndex->mCollection.end())
    {
        return true;
    }
    auto last = std::upper_bound(tblIndex->mCollection.begin(), tblIndex->mCollection.end(), searchKey);
    for (; it != last; ++it)
    {
        outIDs.push_back(it->mID);
    }
    return true;

}

bool MemDB::FindAll(TableID table, SizeT entrySize, SizeT entryAlignment, EntryFindCallback findCallback, void* findUserData, TVector<EntryID>& outIDs) const
{
    outIDs.clear();

    ScopeRWSpinLockRead lock(mLock);
    const Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_FIND_ALL);

    if (t->mEntrySize != entrySize || t->mEntryAlignment != entryAlignment)
    {
        return false;
    }

    const ByteT* ptr = t->mBase;
    for (SizeT i = 0; i < t->mEntryCapacity; ++i)
    {
        MemDBTypes::Entry entry;
        memcpy(&entry, ptr, sizeof(entry));
        if (EntryUsed(entry))
        {
            if (findCallback(ptr, findUserData))
            {
                outIDs.push_back(entry.mReservedID);
            }
        }
        ptr += t->mEntrySize;
    }
    return !outIDs.empty();
}

bool MemDB::Insert(TableID table, const Entry* entryData, SizeT entrySize, SizeT entryAlignment, EntryID& outID)
{
    outID = INVALID32;

    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_INSERT);

    if (entryData == nullptr)
    {
        return false;
    }

    if (entrySize != t->mEntrySize || entryAlignment != t->mEntryAlignment)
    {
        return false;
    }

    // Indexes require unique elements.
    if (!t->mIndices.empty())
    {
        if (!TableCheckIndex(*t, reinterpret_cast<const ByteT*>(entryData)))
        {
            return false;
        }
    }

    ByteT* ptr = nullptr;
    AllocateID(t, ptr, outID);

    // Insert element
    if (Valid(outID))
    {
        Entry* entry = reinterpret_cast<Entry*>(ptr);
        SetFlag(*entry, MemDBTypes::EF_USED);
        SetFlag(*entry, MemDBTypes::EF_DIRTY);

        TableInsertIndex(*t, outID, reinterpret_cast<const ByteT*>(entryData), false);
        ++t->mCount;
        ++t->mPendingWrites;

        t->mDirtyEntries.push_back(outID);

        const ByteT* srcPtr = reinterpret_cast<const ByteT*>(entryData) + sizeof(Entry);
        ByteT* destPtr = ptr + sizeof(Entry);
        memcpy(destPtr, srcPtr, t->mEntrySize - sizeof(Entry));
        AtomicAdd64(&mDataBytesUsed, t->mEntrySize);
        return true;
    }
    return false;
}

bool MemDB::BulkInsert(TableID table, const Entry* entryData, SizeT entrySize, SizeT entryAlignment, SizeT numEntries, TVector<EntryID>& outIDs)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_BULK_INSERT);

    if (entryData == nullptr)
    {
        return false;
    }

    if (entrySize != t->mEntrySize || entryAlignment != t->mEntryAlignment)
    {
        return false;
    }

    if (numEntries == 0)
    {
        return true;
    }

    // Validate Index
    if (!t->mIndices.empty())
    {
        const ByteT* bytes = reinterpret_cast<const ByteT*>(entryData);
        TVector<NumericalVariant> indexItems;
        indexItems.reserve(numEntries);
        for (MemDBTypes::TableIndex& index : t->mIndices)
        {
            if (index.mAllowDuplicates)
            {
                continue;
            }

            indexItems.resize(0);
            for (SizeT i = 0; i < numEntries; ++i)
            {
                const ByteT* entryBytes = bytes + (t->mEntrySize * i);
                const ByteT* itemBytes = entryBytes + index.mOffset;
                indexItems.push_back(NumericalVariant::Cast(index.mDataType, itemBytes));
            }

            std::sort(indexItems.begin(), indexItems.end());
            for (const NumericalVariant& variant : indexItems)
            {
                auto first = std::lower_bound(indexItems.begin(), indexItems.end(), variant);
                auto last = std::lower_bound(indexItems.begin(), indexItems.end(), variant);
                if (std::distance(first, last) > 1)
                {
                    return false; // Can only contain unique items!
                }
            }
        }
    }

    outIDs.resize(0);
    outIDs.reserve(numEntries);

    if (!TryBulkInsert(t, entryData, numEntries, outIDs))
    {
        CleanUpBulkInsert(t, outIDs);
        return false;
    }
    return true;

}

bool MemDB::UpdateOne(TableID table, EntryID entryID, const Entry* entryData, SizeT entrySize, SizeT entryAlignment)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_UPDATE_ONE);

    if (entryData == nullptr)
    {
        return false;
    }

    if (entrySize != t->mEntrySize || entryAlignment != t->mEntryAlignment)
    {
        return false;
    }

    if (entryID >= t->mEntryCapacity)
    {
        return false;
    }

    ByteT* ptr = t->mBase;
    ptr += (t->mEntrySize * entryID);

    Entry* entry = reinterpret_cast<Entry*>(ptr);
    if (!EntryUsed(*entry))
    {
        return false; // Entry not used, use insert instead.
    }

    TableUpdateIndex(*t, entryID, ptr, reinterpret_cast<const ByteT*>(entryData));

    const ByteT* srcPtr = reinterpret_cast<const ByteT*>(entryData) + sizeof(Entry);
    ByteT* destPtr = ptr + sizeof(Entry);
    memcpy(destPtr, srcPtr, t->mEntrySize - sizeof(Entry));
    if(!EntryDirty(*entry))
    {
        ++t->mPendingWrites;
        t->mDirtyEntries.push_back(entryID);
    }
    SetFlag(*entry, MemDBTypes::EF_DIRTY);
    return true;

}

bool MemDB::Delete(TableID table, EntryID id)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_DELETE);

    if (id >= t->mEntryCapacity)
    {
        return false;
    }

    ByteT* ptr = t->mBase;
    ptr += (t->mEntrySize * id);

    Entry* entry = reinterpret_cast<Entry*>(ptr);
    if (!EntryUsed(*entry))
    {
        return false;
    }

    TableRemoveIndex(*t, id);

    UnsetFlag(*entry, MemDBTypes::EF_USED);
    if (!EntryDirty(*entry))
    {
        ++t->mPendingWrites;
        t->mDirtyEntries.push_back(id);
    }
    SetFlag(*entry, MemDBTypes::EF_DIRTY);
    t->mFreeList.push_back(id);
    --t->mCount;

    ByteT* destPtr = ptr + sizeof(Entry);
    memset(destPtr, 0, t->mEntrySize - sizeof(Entry));

    AtomicSub64(&mDataBytesUsed, t->mEntrySize);

    return true;
}

bool MemDB::Select(TableID table, EntryID entryID, SizeT entrySize, SizeT entryAlignment, EntryReadWriteCallback selectCallback, void* selectUserData)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_SELECT_WRITE);

    Entry* entry = SelectUsedEntry(t, entrySize, entryAlignment, entryID);
    if (!entry)
    {
        return false;
    }

    Entry safeEntry;
    memcpy(&safeEntry, entry, sizeof(Entry));

    memcpy(t->mScratchEntry, entry, t->mEntrySize);

    selectCallback(reinterpret_cast<ByteT*>(entry), selectUserData);
    Assert(entry->mReservedFlags == safeEntry.mReservedFlags);
    Assert(entry->mReservedID == safeEntry.mReservedID);
    entry->mReservedFlags = safeEntry.mReservedFlags;
    entry->mReservedID = safeEntry.mReservedID;

    if (memcmp(t->mScratchEntry, entry, t->mEntrySize) != 0)
    {
        if (!EntryDirty(*entry))
        {
            ++t->mPendingWrites;
            t->mDirtyEntries.push_back(entryID);
        }
        SetFlag(*entry, MemDBTypes::EF_DIRTY);
        TableUpdateIndex(*t, entryID, t->mScratchEntry, reinterpret_cast<const ByteT*>(entry));
    }

    return true;
}

bool MemDB::Select(TableID table, EntryID entryID, SizeT entrySize, SizeT entryAlignment, EntryReadCallback selectCallback, void* selectUserData)
{
    ScopeRWSpinLockRead lock(mLock);
    Table* t = GetTable(table);
    if (!t)
    {
        return false;
    }
    TableOp(*t, MemDBTypes::OpTypes::OP_SELECT_READ);
    const Entry* entry = SelectUsedEntry(t, entrySize, entryAlignment, entryID);
    if (!entry)
    {
        return false;
    }
#if !defined(LF_RELEASE)
    UInt32 before = Crc32(reinterpret_cast<const ByteT*>(entry), t->mEntrySize);
#endif
    selectCallback(reinterpret_cast<const ByteT*>(entry), selectUserData);
#if !defined(LF_RELEASE)
    UInt32 after = Crc32(reinterpret_cast<const ByteT*>(entry), t->mEntrySize);
    Assert(before == after);
#endif
    return true;
}

MemDBStats MemDB::GetStats() const
{
    using namespace MemDBTypes;

    MemDBStats stats;
    memset(&stats, 0, sizeof(stats));
    stats.mRuntimeBytesReserved = mRuntimeBytesReserved;
    stats.mRuntimeBytesUsed = mRuntimeBytesUsed;
    stats.mDataBytesReserved = mDataBytesReserved;
    stats.mDataBytesUsed = mDataBytesUsed;
    AtomicRWBarrier();

    ScopeRWSpinLockRead lock(mLock);
    for (const Table* t : mTables)
    {
        stats.mRuntimeBytesReserved += t->mFreeList.capacity() * sizeof(EntryID);
        stats.mRuntimeBytesUsed += t->mFreeList.size() * sizeof(EntryID);

        for (const TableIndex& index : t->mIndices)
        {
            SizeT dataSize = NumericalVariant::GetSize(index.mDataType);
            stats.mRuntimeBytesReserved += dataSize * index.mCollection.size();
            stats.mRuntimeBytesUsed += dataSize * index.mCollection.size();
        }

        for (SizeT i = 0; i < MemDBTypes::OpTypes::MAX_VALUE; ++i)
        {
            stats.mOpCounts[i] += AtomicLoad(&t->mOpCounts[i]);
        }

        stats.mResizeCount += t->mResizeCount;
    }

    return stats;
}

MemDBStats MemDB::GetTableStats(const String& table) const
{
    using namespace MemDBTypes;
    TableID id;
    if (!FindTable(table, id))
    {
        MemDBStats stats;
        memset(&stats, 0, sizeof(stats));
        return stats;
    }
    return GetTableStats(id);
}

MemDBStats MemDB::GetTableStats(TableID table) const
{
    using namespace MemDBTypes;

    MemDBStats stats;
    memset(&stats, 0, sizeof(stats));

    ScopeRWSpinLockRead lock(mLock);
    const Table* t = GetTable(table);

    stats.mRuntimeBytesReserved += t->mFreeList.capacity() * sizeof(EntryID);
    stats.mRuntimeBytesUsed += t->mFreeList.size() * sizeof(EntryID);

    for (const TableIndex& index : t->mIndices)
    {
        SizeT dataSize = NumericalVariant::GetSize(index.mDataType);
        stats.mRuntimeBytesReserved += dataSize * index.mCollection.size();
        stats.mRuntimeBytesUsed += dataSize * index.mCollection.size();
    }

    for (SizeT i = 0; i < MemDBTypes::OpTypes::MAX_VALUE; ++i)
    {
        stats.mOpCounts[i] += AtomicLoad(&t->mOpCounts[i]);
    }

    stats.mDataBytesReserved = t->mEntryCapacity * t->mEntrySize;
    stats.mDataBytesUsed = t->mCount * t->mEntrySize;

    stats.mResizeCount += t->mResizeCount;

    return stats;
}

void MemDB::SetTableFreeCache(TableID table, SizeT cacheSize)
{
    ScopeRWSpinLockWrite lock(mLock);
    Table* t = GetTable(table);
    if (t != nullptr)
    {
        t->mFreeList.reserve(cacheSize);
    }
}

MemDB::Table* MemDB::GetTable(TableID index)
{
    return (index < mTables.size()) ? mTables[index].AsPtr() : nullptr;
}
const MemDB::Table* MemDB::GetTable(TableID index) const
{
    return (index < mTables.size()) ? mTables[index].AsPtr() : nullptr;
}

MemDB::Entry* MemDB::SelectUsedEntry(Table* t, SizeT size, SizeT alignment, EntryID entryID)
{
    if (t->mEntrySize != size || t->mEntryAlignment != alignment)
    {
        return nullptr;
    }

    if (entryID >= t->mEntryCapacity)
    {
        return nullptr;
    }

    ByteT* ptr = t->mBase;
    ptr += (t->mEntrySize * entryID);

    Entry* entry = reinterpret_cast<Entry*>(ptr);
    if (!EntryUsed(*entry))
    {
        return nullptr;
    }
    return entry;
}

void MemDB::AllocateID(Table* t, ByteT*& ptr, EntryID& outID)
{
    outID = INVALID32;

    ptr = t->mBase;
    MemDBTypes::EntryID tryNext = 0;
    // Optimization Step: Try the 'next free', invalidate if we fail.
    MemDBTypes::Entry next;
    if (!TableGetEntry(*t, t->mNextFree, next) || EntryUsed(next))
    {
        tryNext = t->mNextFree;
        t->mNextFree = MemDBTypes::INVALID_ENTRY_ID;
    }
    else
    {
        outID = t->mNextFree;
        ptr += (t->mEntrySize * outID);
        ++t->mNextFree;
    }

    // Try to reuse the free list if we fail 'next free'
    if (Invalid(outID))
    {
        while (!t->mFreeList.empty())
        {
            EntryID id = t->mFreeList.back();
            // t->mFreeList.erase(t->mFreeList.rbegin().base());
            t->mFreeList.pop_back();
            Assert(TableGetEntry(*t, id, next));
            if (!EntryUsed(next))
            {
                outID = id;
                t->mNextFree = outID + 1;
                break;
            }
        }
    }

    // Try starting from where the 'next free' was.
    if (Invalid(outID))
    {
        ptr = t->mBase + tryNext;
        for (SizeT i = static_cast<SizeT>(tryNext); i < t->mEntryCapacity; ++i)
        {
            MemDBTypes::Entry entry;
            memcpy(&entry, ptr, sizeof(entry));
            if (!EntryUsed(entry))
            {
                outID = static_cast<EntryID>(i);
                t->mNextFree = outID + 1;
                break;
            }
            ptr += t->mEntrySize;
        }
    }

    // Ok we're out of options, check the whole table
    if (Invalid(outID))
    {
        if (t->mCount != t->mEntryCapacity)
        {
            ptr = t->mBase;
            for (SizeT i = 0; i < t->mEntryCapacity; ++i)
            {
                MemDBTypes::Entry entry;
                memcpy(&entry, ptr, sizeof(entry));
                if (!EntryUsed(entry))
                {
                    outID = static_cast<EntryID>(i);
                    t->mNextFree = outID + 1;
                    break;
                }
                ptr += t->mEntrySize;
            }
        }
    }

    // Ok we've hit rock bottom, resize it
    if (Invalid(outID) && TableByteCapacity(*t) < ToGB<SizeT>(1))
    {
        SizeT oldCapacity = t->mEntryCapacity;

        Table oldTable = *t;
        t->mEntryCapacity *= 2;
        t->mBase = nullptr;
        t->mEnd = nullptr;
        t->mScratchEntry = nullptr;

        AtomicSub64(&mDataBytesReserved, TableByteCapacity(oldTable));
        AtomicAdd64(&mDataBytesReserved, TableByteCapacity(*t));
        TableAllocCopy(oldTable, *t);
        TableRelease(oldTable);

        // Allocate Again:
        ptr = t->mBase;
        ptr += (t->mEntrySize * oldCapacity);

        for (SizeT i = oldCapacity; i < t->mEntryCapacity; ++i)
        {
            MemDBTypes::Entry entry;
            memcpy(&entry, ptr, sizeof(entry));
            if (!EntryUsed(entry))
            {
                outID = static_cast<EntryID>(i);
                t->mNextFree = outID + 1;
                break;
            }
            ptr += t->mEntrySize;
        }

        ++t->mResizeCount;
    }
}

bool MemDB::TryBulkInsert(Table* t, const Entry* entryData, SizeT numEntries, TVector<EntryID>& outIDs)
{
    if (!t->mIndices.empty())
    {
        for (SizeT i = 0; i < numEntries; ++i)
        {
            if (!TableCheckIndex(*t, reinterpret_cast<const ByteT*>(&entryData[i])))
            {
                return false;
            }
        }
    }

    for (SizeT i = 0; i < numEntries; ++i)
    {
        EntryID outID;
        ByteT* ptr = nullptr;
        AllocateID(t, ptr, outID);

        // Insert element
        if (Valid(outID))
        {
            Entry* entry = reinterpret_cast<Entry*>(ptr);
            SetFlag(*entry, MemDBTypes::EF_USED);
            SetFlag(*entry, MemDBTypes::EF_DIRTY);

            const ByteT* srcBasePtr = reinterpret_cast<const ByteT*>(entryData) + (t->mEntrySize * i);
            const ByteT* srcPtr = srcBasePtr + sizeof(Entry);

            TableInsertIndex(*t, outID, srcBasePtr, false);
            ++t->mCount;
            ++t->mPendingWrites;

            t->mDirtyEntries.push_back(outID);

            ByteT* destPtr = ptr + sizeof(Entry);
            memcpy(destPtr, srcPtr, t->mEntrySize - sizeof(Entry));
            AtomicAdd64(&mDataBytesUsed, t->mEntrySize);
            outIDs.push_back(outID);
        }
        else
        {
            return false;
        }
    }

    return true;
}

void MemDB::CleanUpBulkInsert(Table* t, TVector<EntryID>& outIDs)
{
    for (EntryID id : outIDs)
    {
        ByteT* ptr = t->mBase;
        ptr += (t->mEntrySize * id);
        Entry* entry = reinterpret_cast<Entry*>(ptr);
        TableRemoveIndex(*t, id);
        UnsetFlag(*entry, MemDBTypes::EF_USED);
        if (!EntryDirty(*entry))
        {
            ++t->mPendingWrites;
            t->mDirtyEntries.push_back(id);
        }
        SetFlag(*entry, MemDBTypes::EF_DIRTY);
        t->mFreeList.push_back(id);
        --t->mCount;

        ByteT* destPtr = ptr + sizeof(Entry);
        memset(destPtr, 0, t->mEntrySize - sizeof(Entry));

        AtomicSub64(&mDataBytesUsed, t->mEntrySize);
    }
    outIDs.resize(0);
}

} // namespace lf
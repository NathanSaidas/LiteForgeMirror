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
#include "Core/Test/Test.h"
#include "Core/IO/MemDB.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"
#include "Core/Utility/NumericalVariant.h"
#include "Core/Math/Random.h"
#include "Core/Platform/FileSystem.h"

#include <algorithm>

namespace lf 
{
using namespace MemDBTypes;

static void LogStats(MemDB& db)
{
    auto stats = db.GetStats();
    gTestLog.Info(LogMessage("Database stats:"));
    gTestLog.Info(LogMessage("  Data Bytes Reserved:") << stats.mDataBytesReserved);
    gTestLog.Info(LogMessage("  Data Bytes Used:") << stats.mDataBytesUsed);
    gTestLog.Info(LogMessage("  Runtime Bytes Reserved:") << stats.mRuntimeBytesReserved);
    gTestLog.Info(LogMessage("  Runtime Bytes Used:") << stats.mRuntimeBytesUsed);
    gTestLog.Info(LogMessage("  Operations:"));
    for (SizeT i = 0; i < OpTypes::MAX_VALUE; ++i)
    {
        gTestLog.Info(LogMessage("    ") << TOpTypes::GetString(i) << ":" << stats.mOpCounts[i]);
    }
}

struct TestInfo_DO : public MemDBTypes::Entry
{
    TestInfo_DO& Generate(Int32& seed)
    {
        mItemID = static_cast<UInt32>(Random::Range(seed, 0, 50000));
        mParentID = static_cast<UInt32>(Random::Range(seed, 0, 50000));
        mWeakReference = static_cast<UInt32>(Random::Range(seed, 0, 300));
        mStrongReference = static_cast<UInt32>(Random::Range(seed, 5, 84));
        mInstanceCount = static_cast<UInt32>(Random::Range(seed, 24, 600));
        return *this;
    }

    bool operator==(const TestInfo_DO& other) const
    {
        return mItemID == other.mItemID
            && mParentID == other.mParentID
            && mWeakReference == other.mWeakReference
            && mStrongReference == other.mStrongReference
            && mInstanceCount == other.mInstanceCount;
    }

    UInt32 mItemID;
    UInt32 mParentID;
    UInt32 mWeakReference;
    UInt32 mStrongReference;
    UInt32 mInstanceCount;
};
struct TestReference_DO : public MemDBTypes::Entry
{
    TestReference_DO& Generate(Int32& seed)
    {
        mItemID = static_cast<UInt32>(Random::Range(seed, 0, 50000));
        mParentID = static_cast<UInt32>(Random::Range(seed, 0, 50000));
        return *this;
    }

    bool operator==(const TestInfo_DO& other) const
    {
        return mItemID == other.mItemID
            && mParentID == other.mParentID;
    }

    UInt32 mItemID;
    UInt32 mParentID;
};

struct TestIDToName_DO : public MemDBTypes::Entry
{
    bool operator==(const TestIDToName_DO& other) const
    {
        return mItemID == other.mItemID
            && mName == other.mName;
    }
    UInt32        mItemID;
    MemDBChar<64> mName;
};

struct CacheIndex_DO : public MemDBTypes::Entry
{
    UInt32 mUID;
    UInt32 mBlobID;
    UInt32 mObjectID;
    UInt32 mLocation;
    UInt32 mSize;
    UInt32 mCapacity;

    char   mName[64];
};

// BENCHMARKS AND STRES TESTING
// By default tables carry the capacity for 1KB of the items, in order to get better performance
// (less spikes during table resizing) we can specify how much we 'think' we'll need. 
// [SIGNIFANT IMPACT]
const SizeT OPT_TABLE_RESERVE = ToMB<SizeT>(1) / sizeof(TestInfo_DO);
// One of the optimization features of the table is to use a 'free cache' which is 
// basically a list of free entries the DB will attempt to reuse if it fails
// the 'next' allocation check. The bigger this cache the less resizing.
// [MINOR IMPACT]
const SizeT OPT_TABLE_FREE_CACHE = 15000;

// 

REGISTER_TEST(MemDBString_BufferSize_Test, "Core.IO.MemDBString")
{
    MemDBChar<5> str;
    str.Assign("hello");
    TEST(str.Equals("hello") == false); // We can't assign Hello because the buffer doesnt pad for null terminator (Explicit Size)
    TEST(str.Equals("hell") == true);   // So all that was stored was 'hell'

    str.Clear();
    TEST(str.Equals("") == true);

    str.Append("hello");
    TEST(str.Equals("hello") == false); // We can't assign Hello because the buffer doesnt pad for null terminator (Explicit Size)
    TEST(str.Equals("hell") == true);   // So all that was stored was 'hell'
}

REGISTER_TEST(MemDBString_Assign_Test, "Core.IO.MemDBString")
{
    MemDBChar<32> str;
    str.Assign("Hello World");
    TEST(str.Equals("Hello World") == true);
}

REGISTER_TEST(MemDBString_Append_Test, "Core.IO.MemDBString")
{
    MemDBChar<32> str;
    str.Assign("Hello");
    str.Append(" World");
    TEST(str.Equals("Hello World") == true);
}

REGISTER_TEST(MemDBString_Equals_Tets, "Core.IO.MemDBString")
{
    MemDBChar<32> str;

    str.Assign("Hello");
    TEST(str.Equals("Hello"));
    TEST(!str.Equals("Helloo"));
    TEST(!str.Equals("Hell"));
}

REGISTER_TEST(MemDBString_OpAssign_Test, "Core.IO.MemDBString")
{
    MemDBChar<32> foo("foo");
    MemDBChar<32> bar("bar");
    MemDBChar<16> baz("baz");

    TEST(foo.Equals("foo"));
    TEST(bar.Equals("bar"));
    TEST(baz.Equals("baz"));

    foo = bar;
    TEST(foo.Equals(bar.CStr()));

    baz = bar;
    TEST(baz.Equals(bar.CStr()));
}

REGISTER_TEST(MemDBString_OpEqaulity_Test, "Core.IO.MemDBString")
{
    MemDBChar<32> foo("foo");
    MemDBChar<32> bar("bar");
    MemDBChar<16> baz("baz");

    TEST(foo.Equals("foo"));
    TEST(bar.Equals("bar"));
    TEST(baz.Equals("baz"));

    foo = bar;
    TEST(foo == MemDBField("bar"));
    TEST(foo != MemDBField("baz"));
    baz = bar;
    TEST(baz == MemDBField("bar"));
    TEST(baz != MemDBField("baz"));
}

// Test that we can create tables, and that tables are unique.
REGISTER_TEST(MemDB_CreateTable_Test, "Core.IO")
{
    MemDB db;

    TableID info;
    TEST(db.CreateTable<TestInfo_DO>("info", info));

    TableID reference;
    TEST(db.CreateTable<TestReference_DO>("reference", reference));

    TableID dummy;
    TEST(!db.CreateTable<TestInfo_DO>("info", dummy));
    TEST(!db.CreateTable<TestInfo_DO>("reference", dummy));

}

// Test that we can destroy tables, and recreate them
REGISTER_TEST(MemDB_CreateDestroy_Test, "Core.IO")
{
    MemDB db;

    TableID info;
    TEST(db.CreateTable<TestInfo_DO>("info", info));

    TableID reference;
    TEST(db.CreateTable<TestReference_DO>("reference", reference));

    TableID dummy;
    TEST(!db.CreateTable<TestInfo_DO>("info", dummy));
    TEST(!db.CreateTable<TestInfo_DO>("reference", dummy));

    TEST(db.DeleteTable(info));

    // indicies shouldn't change
    TEST(db.FindTable("reference", dummy));
    TEST(dummy == reference);

    TEST(!db.FindTable("info", dummy));;

    TEST(!db.CreateTable<TestInfo_DO>("reference", dummy));

    TEST(db.CreateTable<TestInfo_DO>("info", info));
}

// Test basic CRUD operations.
REGISTER_TEST(MemDB_CRUD_Test, "Core.IO")
{
    Int32 seed = 0x3876239;

    MemDB db;

    TableID info;
    TEST_CRITICAL(db.CreateTable<TestInfo_DO>("info", info));

    TableID reference;
    TEST_CRITICAL(db.CreateTable<TestReference_DO>("reference", reference));


    TestInfo_DO infoDO;
    EntryID infoID;
    TEST(db.Insert(info, infoDO.Generate(seed), infoID));

    TestReference_DO referenceDO;
    EntryID referenceID;
    TEST(db.Insert(reference, referenceDO.Generate(seed), referenceID));

    EntryID testID;
    TEST(db.FindOne<TestInfo_DO>(info, [&infoDO](const TestInfo_DO* item) { return item->mItemID == infoDO.mItemID; }, testID));
    TEST(testID == infoID);

    TEST(db.FindOne<TestReference_DO>(reference, [&referenceDO](const TestReference_DO* item) { return item->mItemID == referenceDO.mItemID; }, testID));
    TEST(testID == referenceID);

    // Wrong table size!
    TEST(!db.FindOne<TestInfo_DO>(reference, [&infoDO](const TestInfo_DO* item) { return item->mItemID == infoDO.mItemID; }, testID));

    // Wrong table size!
    TEST(!db.FindOne<TestReference_DO>(info, [&referenceDO](const TestReference_DO* item) { return item->mItemID == referenceDO.mItemID; }, testID));

    TEST(db.Delete(info, infoID));
    TEST(!db.Delete(info, infoID));
    TEST(!db.UpdateOne(info, infoID, &infoDO.Generate(seed)));
    TEST(db.Insert(info, infoDO, infoID));
}

REGISTER_TEST(MemDB_String_Test, "Core.IO")
{
    MemDB db;
    TableID idToName;
    TEST(db.CreateTable<TestIDToName_DO>("idToName", idToName));

    TestIDToName_DO obj;
    obj.mItemID = 500;
    obj.mName = MemDBField("engine//builtin/editor/ButtonMaterial.lob");
    TEST(obj.mName.Equals("engine//builtin/editor/ButtonMaterial.lob"));

    EntryID objID;
    TEST(db.Insert(idToName, obj, objID));

    obj.mItemID = 200;
    TEST(db.UpdateOne(idToName, objID, &obj));

    TestIDToName_DO resultObj;
    EntryID resultID;

    TEST(db.FindOne<TestIDToName_DO>(idToName,
        [&resultObj, &obj](const TestIDToName_DO* item)
        {
            if (item->mName == obj.mName)
            {
                resultObj = *item;
            }
            return item->mName == obj.mName;
        }, resultID));

    TEST(resultObj.mItemID == 200);
}

// Basic test to add a lot of items.
REGISTER_TEST(MemDB_MiniStress_Test, "Core.IO")
{
    Int32 seed = 0x3876239;

    MemDB db;
    TableID info;
    TEST_CRITICAL(db.CreateTable<TestInfo_DO>("info", info));

    TVector<TestInfo_DO> items;

    TestInfo_DO infoDO;
    EntryID infoID;
    for (SizeT i = 0; i < 1000; ++i)
    {
        infoDO.Generate(seed);
        infoDO.mItemID = static_cast<UInt32>(i);
        TEST(db.Insert(info, infoDO, infoID));

        infoDO.mReservedID = infoID;
        items.push_back(infoDO);

        if (i != 0 && i % 100 == 0)
        {
            auto stats = db.GetStats();
            gTestLog.Debug(LogMessage("Data Reserved ") << stats.mDataBytesReserved);
        }
    }

    for (SizeT i = 0; i < 1000; ++i)
    {
        TestInfo_DO& current = items[i];
        TestInfo_DO outObject;
        EntryID result;
        TEST(db.FindOne<TestInfo_DO>(info, [&current, &outObject](const TestInfo_DO* item) 
            {  
                if (item->mItemID == current.mItemID)
                {
                    outObject = *item;
                }
                return item->mItemID == current.mItemID; 
            }, result));

        TEST(result == current.mReservedID);
        TEST(outObject == current);
    }
}

template<typename T>
static void InsertRandomExclude(MemDB& db, TableID table, Int32 seed, SizeT count, const TVector<T>& objs)
{
    T generated;
    EntryID id;
    for (SizeT i = 0; i < count;)
    {
        generated.Generate(seed);
        auto it = std::find_if(objs.begin(), objs.end(), [&generated](const T& item) { return item.mItemID == generated.mItemID || item.mParentID == generated.mParentID; });
        if (it != objs.end())
        {
            continue;
        }
        TEST(db.Insert(table, generated, id));
        ++i;
    }
}

REGISTER_TEST(MemDB_FindOne_Test, "Core.IO")
{
    Int32 seed = 0x3876239;

    MemDB db;
    TableID table;
    TEST_CRITICAL(db.CreateTable<TestReference_DO>("table", table));

    TestReference_DO obj;
    obj.mItemID = 200;
    obj.mParentID = 500;
    InsertRandomExclude<TestReference_DO>(db, table, seed, 200, { obj });

    EntryID objID;
    TEST(db.Insert(table, obj, objID));
    
    UInt32 searchID = 200;
    TestReference_DO resultObj;
    EntryID resultID;

    TEST(db.FindOne<TestReference_DO>(table,
        [searchID, &resultObj](const TestReference_DO* item)
        {
            if (item->mItemID == searchID)
            {
                resultObj = *item;
                return true;
            }
            return false;
        }, resultID));

    TEST(resultID == objID);

    TEST(!db.FindOne<TestInfo_DO>(table,
        [searchID](const TestInfo_DO* item)
        {
            return searchID == item->mItemID;
        }, resultID));
}


REGISTER_TEST(MemDB_FindAll_Test, "Core.IO")
{
    Int32 seed = 0x3876239;

    MemDB db;
    TableID table;
    TEST_CRITICAL(db.CreateTable<TestReference_DO>("table", table));

    TVector<TestReference_DO> objs;

    TestReference_DO obj;
    obj.mItemID = 200;
    obj.mParentID = 500;
    objs.push_back(obj);

    obj.mItemID = 215;
    obj.mParentID = 500;
    objs.push_back(obj);

    obj.mItemID = 500;
    obj.mParentID = 725;
    objs.push_back(obj);

    InsertRandomExclude(db, table, seed, 200, objs);

    for (TestReference_DO& item : objs)
    {
        EntryID objID;
        TEST(db.Insert(table, item, objID));
        item.mReservedID = objID;
    }

    UInt32 parentID = 500;
    TVector<TestReference_DO> results;
    TVector<EntryID> resultIDs;

    TEST(db.FindAll<TestReference_DO>(table,
        [parentID, &results](const TestReference_DO* item)
        {
            if (item->mParentID == parentID)
            {
                results.push_back(*item);
                return true;
            }
            return false;
        }, resultIDs));

    TEST(results.size() >= 2);
    for (TestReference_DO& item : objs)
    {
        if (item.mParentID != parentID)
        {
            continue;
        }
        TEST(results.end() != std::find_if(results.begin(), results.end(), [&item](const TestReference_DO& resultItem) { return resultItem.mItemID == item.mItemID; }));
        TEST(resultIDs.end() != std::find_if(resultIDs.begin(), resultIDs.end(), [&item](EntryID resultID) { return resultID == item.mReservedID; }));
    }
}


REGISTER_TEST(MemDB_UpdateOne_Test, "Core.IO")
{
    Int32 seed = 0x3876239;

    MemDB db;
    TableID table;
    TEST_CRITICAL(db.CreateTable<TestReference_DO>("table", table));

    TestReference_DO foo;
    foo.mItemID = 200;
    foo.mParentID = 500;

    TestReference_DO bar;
    bar.mItemID = 200;
    bar.mParentID = 303;
    InsertRandomExclude<TestReference_DO>(db, table, seed, 200, { bar, foo });

    EntryID objID;
    TEST(db.Insert(table, foo, objID));

    EntryID resultID;
    TEST(db.FindOne<TestReference_DO>(table, [](const TestReference_DO* item) { return item->mParentID == 500; }, resultID));

    TEST(db.UpdateOne(table, resultID, &bar));

    TEST(!db.FindOne<TestReference_DO>(table, [](const TestReference_DO* item) { return item->mParentID == 500; }, resultID));
}

REGISTER_TEST(MemDB_SelectUpdateAll_Test, "Core.IO")
{
    Int32 seed = 0x3876239;

    MemDB db;
    TableID table;
    TEST_CRITICAL(db.CreateTable<TestReference_DO>("table", table));

    TestReference_DO foo;
    foo.mItemID = 200;
    foo.mParentID = 500;

    TestReference_DO bar;
    bar.mItemID = 200;
    bar.mParentID = 303;

    TestReference_DO baz;
    baz.mItemID = 201;
    baz.mParentID = 500;
    InsertRandomExclude<TestReference_DO>(db, table, seed, 200, { bar, foo, baz });

    EntryID resultID;
    TEST(db.Insert(table, foo, resultID));
    TEST(db.Insert(table, baz, resultID));

    TVector<EntryID> resultIDs;
    TEST(db.FindAll<TestReference_DO>(table, [](const TestReference_DO* item) { return item->mParentID == 500; }, resultIDs));

    TEST(resultIDs.size() == 2);
    for (EntryID id : resultIDs)
    {
        TEST(db.SelectWrite<TestReference_DO>(table, id, [](TestReference_DO* item) { item->mParentID = 303; return true; }));
    }

    TEST(!db.FindAll<TestReference_DO>(table, [](const TestReference_DO* item) { return item->mParentID == 500; }, resultIDs));
    TEST(resultIDs.empty());

    TEST(db.FindAll<TestReference_DO>(table, [](const TestReference_DO* item) { return item->mParentID == 303; }, resultIDs));
    TEST(resultIDs.size() == 2);

    for (EntryID id : resultIDs)
    {
        TEST(db.SelectRead<TestReference_DO>(table, id, [](const TestReference_DO* item) { TEST(item->mItemID == 200 || item->mItemID == 201); return true; }));
    }
}

// Basic test to write items out to a file.
REGISTER_TEST(MemDB_ReadWriteFile_Test, "Core.IO")
{
    Int32 seed = 0x3876239;
    MemDB db;

    TableID info;
    TEST(db.CreateTable<TestInfo_DO>("info", info));

    TVector<TestInfo_DO> objects;

    TestInfo_DO infoDO;
    EntryID infoID;
    for (SizeT i = 0; i < 3; ++i)
    {
        TEST(db.Insert(info, infoDO.Generate(seed), infoID));
        infoDO.mReservedID = infoID;
        objects.push_back(infoDO);
    }


    String path = FileSystem::PathJoin(TestFramework::GetTempDirectory(), "MemDB_ReadWriteFile_Test.db");
    if (FileSystem::FileExists(path))
    {
        FileSystem::FileDelete(path);
    }
    db.WriteToFile(info, path, true);
    
    db.Release();
    db.CreateTable<TestInfo_DO>("info", info);

    db.ReadFromFile(info, path);

    for (SizeT i = 0; i < objects.size(); ++i)
    {
        TestInfo_DO& obj = objects[i];
        TestInfo_DO resultObj;
        EntryID resultID;
        TEST(db.FindOne<TestInfo_DO>(info, [&obj, &resultObj](const TestInfo_DO* item)
            {
                if (item->mReservedID == obj.mReservedID)
                {
                    resultObj = *item;
                }
                return item->mReservedID == obj.mReservedID;
            },
            resultID));

        TEST(resultObj == obj);
    }
    LogStats(db);
}

REGISTER_TEST(MemDB_ReadWriteFileMiniStress_Test, "Core.IO")
{
    Int32 seed = 0x3876239;
    MemDB db;

    TableID info;
    TEST(db.CreateTable<TestInfo_DO>("info", info));

    TVector<TestInfo_DO> objects;

    TestInfo_DO infoDO;
    EntryID infoID;
    for (SizeT i = 0; i < 3; ++i)
    {
        TEST(db.Insert(info, infoDO.Generate(seed), infoID));
        infoDO.mReservedID = infoID;
        objects.push_back(infoDO);
    }



    String path = FileSystem::PathJoin(TestFramework::GetTempDirectory(), "MemDB_ReadWriteFileMiniStress_Test.db");
    if (FileSystem::FileExists(path))
    {
        FileSystem::FileDelete(path);
    }

    String testCsv = FileSystem::PathJoin(TestFramework::GetTempDirectory(), "MemDB_ReadWriteFileMiniStress_Test.csv");
    File file;
    gTestLog.Info(LogMessage("Attempting to open file: ") << testCsv);
    TEST_CRITICAL(file.Open(testCsv, FF_WRITE, FILE_OPEN_CREATE_NEW));

    const SizeT ITEMS_ADDED_PER_FRAME = 50;
    const SizeT COUNT = ToMB<SizeT>(5) / sizeof(TestInfo_DO) / ITEMS_ADDED_PER_FRAME;
    Timer timer;
    TVector<TimeTypes::Seconds> times;
    times.reserve(COUNT);

    for (SizeT i = 0; i < COUNT; ++i)
    {
        InsertRandomExclude<TestInfo_DO>(db, info, seed, ITEMS_ADDED_PER_FRAME, {});
        timer.Start();
        db.WriteToFile(info, path);
        timer.Stop();

        times.push_back(TimeTypes::Seconds(static_cast<Float32>(timer.GetDelta())));
    }

    SStream ss;
    for (SizeT i = 0; i < COUNT; ++i)
    {
        ss << ToMicroseconds(times[i]).mValue << ",\r\n";
    }
    file.Write(ss.CStr(), ss.Size());
    file.Close();

    LogStats(db);
}

// Test the performance of linear-insert operations.
REGISTER_TEST(MemDB_InsertStress_Test, "Core.IO")
{
    Int32 seed = 0x3876239;

    MemDB db;
    TableID info;
    TEST_CRITICAL(db.CreateTable<TestInfo_DO>("info", info));

    String path = FileSystem::PathJoin(TestFramework::GetTempDirectory(), "MemDB_InsertStress_Test.csv");
    File file;
    gTestLog.Info(LogMessage("Attempting to open file: ") << path);
    TEST_CRITICAL(file.Open(path, FF_WRITE, FILE_OPEN_CREATE_NEW));

    const SizeT COUNT = 50000;
    Timer timer;
    TVector<TimeTypes::Seconds> times;
    times.reserve(COUNT);

    TestInfo_DO infoDO;
    EntryID infoID;

    for (SizeT i = 0; i < COUNT; ++i)
    {
        infoDO.Generate(seed);
        infoDO.mItemID = static_cast<UInt32>(i);

        timer.Start();
        TEST(db.Insert(info, infoDO, infoID));
        timer.Stop();

        times.push_back(TimeTypes::Seconds(static_cast<Float32>(timer.GetDelta())));
    }

    SStream ss;
    for (SizeT i = 0; i < COUNT; ++i)
    {
        ss << ToMicroseconds(times[i]).mValue << ",\r\n";
    }
    file.Write(ss.CStr(), ss.Size());
    file.Close();
    LogStats(db);
}

// Test the performance of insert into memdb which has 
// random deletions
REGISTER_TEST(MemDB_RandomInsertStress_Test, "Core.IO")
{
    Int32 seed = 0x3876239;
    MemDB db;
    TableID info;

    TEST_CRITICAL(db.CreateTable<TestInfo_DO>("info", OPT_TABLE_RESERVE, info));
    db.SetTableFreeCache(info, OPT_TABLE_FREE_CACHE);

    String path = FileSystem::PathJoin(TestFramework::GetTempDirectory(), "MemDB_RandomInsertStress_Test.csv");
    File file;
    gTestLog.Info(LogMessage("Attempting to open file: ") << path);
    TEST_CRITICAL(file.Open(path, FF_WRITE, FILE_OPEN_CREATE_NEW));

    const SizeT COUNT = 50000;
    Timer timer;
    TVector<TimeTypes::Seconds> times;
    times.reserve(COUNT);

    TestInfo_DO infoDO;
    EntryID infoID;

    TVector<EntryID> toRemove;
    // Insert a bunch.
    gTestLog.Info(LogMessage("Inserting ") << COUNT << " items...");
    for (SizeT i = 0; i < COUNT; ++i)
    {
        infoDO.Generate(seed);
        infoDO.mItemID = static_cast<UInt32>(i);

        TEST(db.Insert(info, infoDO, infoID));
        
        if (toRemove.size() < 10000 && Random::Range(seed, 0.0f, 1.0f) > 0.5f)
        {
            toRemove.push_back(infoID);
        }
    }

    SizeT numToAdd = toRemove.size();
    gTestLog.Info(LogMessage("Removing ") << numToAdd << " items...");
    // Remove at random
    for (EntryID id : toRemove)
    {
        TEST(db.Delete(info, id));
    }
    TEST(numToAdd < COUNT);

    // Re-insert
    gTestLog.Info(LogMessage("Inserting ") << numToAdd << " items...");
    for (SizeT i = 0; i < COUNT; ++i)
    {
        infoDO.Generate(seed);
        infoDO.mItemID = static_cast<UInt32>(i);

        timer.Start();
        TEST(db.Insert(info, infoDO, infoID));
        timer.Stop();

        if (i != 0 && i % 1000 == 0)
        {
            auto stats = db.GetStats();
            gTestLog.Debug(LogMessage("Iteration ") << i);
            gTestLog.Debug(LogMessage("  Data Reserved ") << stats.mDataBytesReserved);
            gTestLog.Debug(LogMessage("  Data Used ") << stats.mDataBytesUsed);
            gTestLog.Debug(LogMessage("  Runtime Reserved ") << stats.mRuntimeBytesReserved);
            gTestLog.Debug(LogMessage("  Runtime Used ") << stats.mRuntimeBytesUsed);
        }

        times.push_back(TimeTypes::Seconds(static_cast<Float32>(timer.GetDelta())));
    }


    SStream ss;
    for (SizeT i = 0; i < COUNT; ++i)
    {
        ss << ToMicroseconds(times[i]).mValue << ",\r\n";
    }
    file.Write(ss.CStr(), ss.Size());
    file.Close();
    LogStats(db);
}

REGISTER_TEST(MemDB_FindStress_Test, "Core.IO", TestFlags::TF_STRESS)
{
    Int32 seed = 0x3876239;
    MemDB db;
    TableID info;

    TEST_CRITICAL(db.CreateTable<TestInfo_DO>("info", OPT_TABLE_RESERVE, info));
    db.SetTableFreeCache(info, OPT_TABLE_FREE_CACHE);


    String path = FileSystem::PathJoin(TestFramework::GetTempDirectory(), "MemDB_FindStress_Test.csv");
    File file;
    gTestLog.Info(LogMessage("Attempting to open file: ") << path);
    TEST_CRITICAL(file.Open(path, FF_WRITE, FILE_OPEN_CREATE_NEW));

    const SizeT COUNT = 50000;
    Timer timer;
    TVector<TimeTypes::Seconds> times;
    times.reserve(COUNT);

    TVector<TestInfo_DO> objects;

    TestInfo_DO infoDO;
    EntryID infoID;

    // Insert a bunch.
    gTestLog.Info(LogMessage("Inserting ") << COUNT << " items...");
    for (SizeT i = 0; i < COUNT; ++i)
    {
        infoDO.Generate(seed);
        infoDO.mItemID = static_cast<UInt32>(i);

        TEST(db.Insert(info, infoDO, infoID));

        infoDO.mReservedID = infoID;
        objects.push_back(infoDO);
    }

    for (SizeT i = 0; i < COUNT; ++i)
    {
        auto& object = objects[static_cast<SizeT>(Random::Mod(seed, static_cast<UInt32>(COUNT)))];
        EntryID resultID;

        timer.Start();
        TEST(db.FindOne<TestInfo_DO>(info,
            [&object](const TestInfo_DO* item)
            {
                return item->mItemID == object.mItemID;
            },
            resultID));
        timer.Stop();

        TEST(object.mReservedID == resultID);
        times.push_back(TimeTypes::Seconds(static_cast<Float32>(timer.GetDelta())));
    }

    SStream ss;
    for (SizeT i = 0; i < COUNT; ++i)
    {
        ss << ToMicroseconds(times[i]).mValue << ",\r\n";
    }
    file.Write(ss.CStr(), ss.Size());
    file.Close();
    LogStats(db);
}

REGISTER_TEST(MemDB_FindIndexedStress_Test, "Core.IO")
{
    Int32 seed = 0x3876239;
    MemDB db;
    TableID info;

    TEST_CRITICAL(db.CreateTable<TestInfo_DO>("info", OPT_TABLE_RESERVE, info));
    db.SetTableFreeCache(info, OPT_TABLE_FREE_CACHE);


    String path = FileSystem::PathJoin(TestFramework::GetTempDirectory(), "MemDB_FindIndexedStress_Test.csv");
    File file;
    gTestLog.Info(LogMessage("Attempting to open file: ") << path);
    TEST_CRITICAL(file.Open(path, FF_WRITE, FILE_OPEN_CREATE_NEW));

    const SizeT COUNT = 50000;
    Timer timer;
    TVector<TimeTypes::Seconds> times;
    times.reserve(COUNT);

    TVector<TestInfo_DO> objects;

    TestInfo_DO infoDO;
    EntryID infoID;

    // Insert a bunch.
    gTestLog.Info(LogMessage("Inserting ") << COUNT << " items...");
    for (SizeT i = 0; i < COUNT; ++i)
    {
        infoDO.Generate(seed);
        infoDO.mItemID = static_cast<UInt32>(i);

        TEST(db.Insert(info, infoDO, infoID));

        infoDO.mReservedID = infoID;
        objects.push_back(infoDO);
    }

    gSysLog.Info(LogMessage("Creating Index..."));
    TEST(db.CreateIndex(info, NumericalVariantType::VT_U32, offsetof(TestInfo_DO, mItemID)));
    
    gSysLog.Info(LogMessage("Executing searches..."));
    for (SizeT i = 0; i < COUNT; ++i)
    {
        auto& object = objects[static_cast<SizeT>(Random::Mod(seed, static_cast<UInt32>(COUNT)))];
        EntryID resultID;

        timer.Start();
        TEST(db.FindOneIndexed(info, NumericalVariant(object.mItemID), offsetof(TestInfo_DO, mItemID), resultID));
        timer.Stop();

        TEST(db.SelectWrite<TestInfo_DO>(info, resultID,
            [](TestInfo_DO* item)
            {
                item->mInstanceCount = 0;
                return true;
            }));

        TEST(object.mReservedID == resultID);
        times.push_back(TimeTypes::Seconds(static_cast<Float32>(timer.GetDelta())));
    }

    SStream ss;
    for (SizeT i = 0; i < COUNT; ++i)
    {
        ss << ToMicroseconds(times[i]).mValue << ",\r\n";
    }
    file.Write(ss.CStr(), ss.Size());
    file.Close();

    LogStats(db);
}

} // namespace lf 
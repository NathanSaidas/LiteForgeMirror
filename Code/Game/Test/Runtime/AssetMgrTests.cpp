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
#include "Core/IO/EngineConfig.h"
#include "Core/IO/TextStream.h"
#include "Core/IO/JsonStream.h"
#include "Core/IO/BinaryStream.h"
#include "Core/IO/DependencyStream.h"
#include "Core/Math/Random.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Platform/FileSystem.h"
#include "Core/String/StringHashTable.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Guid.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/Utility.h"
#include "Core/Utility/Crc32.h"
#include "Runtime/Asset/AssetMgr.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetOp.h"
#include "Runtime/Asset/AssetTypeInfo.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "Game/Test/StressDataAsset.h"
#include "Game/Test/TestUtils.h"

#include <algorithm>
#include <memory>


// See DefaultInitialize below
//
// All the tests here are built to run in a test environment, so that if we have any test failures we can easily delete cache/content
// in addition to it running in a test environment we can execute certain tasks rather quickly, which might take a long time with 
// full content.
// 

// Tests to complete...
// 
// 1. [Done] Verify we can create an asset.
// 2. [Done] Verify we can delete an asset.
// 3. [Done] Verify we can load an asset.
// 4. Verify we can create an asset that references another (weak and strong)
// 5. Verify we can NOT load recursive assets
// 6. Verify we can load from source
// 7. Verify we can load from cache
// 8. Verify we can load multiple domains
// 9.


// Mod loading..
// While in game we should be able to enable/disable mods (only in 'mod safe' state)
//
// Game States:
// [ Program Init ]
// [ App Init ] 
// [ Game Splash ] => [ Game Title ] => [ Game World:Activate ]
// [ App Shutdown ]
// 
// Going from Game Title to Game World 
//     -> Mod:Register
//     -> Mod:InitializeLoop
//     -> Mod:PostInit
//
// Going from Game World to Game Title
//     -> Mod:DestroyObjects
//     -> Mod:Shutdown
// 

namespace lf {

template<typename T>
using TestAsset = TAsset<T, TestAssetMgrProvider>;
template<typename T>
using TestAssetType = TAssetType<T, TestAssetMgrProvider>;

class AssetMgrTestObject : public AssetObject
{
    DECLARE_CLASS(AssetMgrTestObject, AssetObject);
public:
    AssetMgrTestObject() {}
    ~AssetMgrTestObject() override = default;

    void Serialize(Stream& s) override
    {
        SERIALIZE(s, mBaseHealth, "");
        SERIALIZE(s, mBaseMana, "");
    }

protected:
    void OnClone(const Object& other) override
    {
        const AssetMgrTestObject& o = static_cast<const AssetMgrTestObject&>(other);
        mBaseHealth = o.mBaseHealth;
        mBaseMana = o.mBaseMana;
    }
public:

    Int32 mBaseHealth;
    Int32 mBaseMana;

    // How we want to store asset references
    //
    // CarAssetType;
    // AssetType<Car>;

    // CarAsset;
    // Asset<Car>;
    // 
    // In each case the template is just a wrapper around AssetHandle where we have access to the following members
    //
    //   TypeInfo
    //   Prototype
    //   StrongRef
    //   WeakRef
    //
    // AcquireStrong
    // AcquireWeak
    // ReleaseStrong
    // ReleaseWeak

};
DEFINE_CLASS(lf::AssetMgrTestObject) { NO_REFLECTION; }

class AssetMgrTestContainer : public AssetObject
{
    DECLARE_CLASS(AssetMgrTestContainer, AssetObject);
public:
    AssetMgrTestContainer() {}
    ~AssetMgrTestContainer() override = default;

    void Serialize(Stream& s) override
    {
        SERIALIZE(s, mObject, "");
    }

protected:
    void OnClone(const Object& other) override
    {
        const AssetMgrTestContainer& o = static_cast<const AssetMgrTestContainer&>(other);
        mObject = o.mObject;
    }

public:

    TestAsset<AssetMgrTestObject> mObject;
};

DEFINE_CLASS(lf::AssetMgrTestContainer) { NO_REFLECTION; }

static void DefaultInitialize(AssetMgr& mgr)
{
    String projectDir = TestFramework::GetConfig().mEngineConfig->GetProjectDirectory();
    String cacheDir = TestFramework::GetConfig().mEngineConfig->GetCacheDirectory();
    if (!FileSystem::PathExists(projectDir))
    {
        FileSystem::PathCreate(projectDir);
        FileSystem::FileCreate(FileSystem::PathJoin(projectDir, "delete_folder_for_release.txt"));
    }
    if (!FileSystem::PathExists(cacheDir))
    {
        FileSystem::PathCreate(cacheDir);
        FileSystem::FileCreate(FileSystem::PathJoin(cacheDir, "delete_folder_for_release.txt"));
    }

    TEST_CRITICAL(mgr.Initialize(projectDir, cacheDir, true));
}

struct TestAssetDB
{
    struct {
        AssetPath mTestObjectA;
        AssetPath mTestObjectB;
        AssetPath mTestContainer;
    } mPaths;

    struct {
        AssetMgrTestObject mTestObjectA;
        AssetMgrTestObject mTestObjectB;
    } mObjects;
};

static bool WaitForOp(AssetMgr& mgr, AssetOpAtomicWPtr op)
{
    SizeT debugFrame = 0;
    while (op && !op->TimedOut() && !op->IsComplete())
    {
        mgr.Update();
        ++debugFrame;
    }

    if (op && !op->IsSuccess())
    {
        gTestLog.Error(LogMessage("Failed to complete asset OP. Reason=") << op->GetFailReason());
    }

    return op && op->IsSuccess();
}
static void WaitForAsset(AssetMgr& mgr, AssetHandle* handle, Float32 timeout = 5 * 60.0f)
{
    Timer t;
    t.Start();
    SizeT debugFrame = 0;
    while (t.PeekDelta() < timeout && handle->mType->GetLoadState() != AssetLoadState::ALS_LOADED)
    {
        mgr.Update();
        ++debugFrame;
    }

}

bool ConfigureTestDB(AssetMgr& mgr, TestAssetDB& testDB)
{
    // Configure the test object with defaults
    
    if (testDB.mPaths.mTestObjectA.Empty())
    {
        testDB.mPaths.mTestObjectA = AssetPath("engine//test/testObjectA.lob");
        testDB.mObjects.mTestObjectA.mBaseHealth = 1010;
        testDB.mObjects.mTestObjectA.mBaseMana = 2020;
    }

    if (testDB.mPaths.mTestObjectB.Empty())
    {
        testDB.mPaths.mTestObjectB = AssetPath("engine//test/testObjectB.lob");
        testDB.mObjects.mTestObjectB.mBaseHealth = 3030;
        testDB.mObjects.mTestObjectB.mBaseMana = 4040;
    }
    
    if (testDB.mPaths.mTestContainer.Empty())
    {
        // TODO: This string caused low level bug in string! We should add this to a unit test!
        // COW -> Copy the COW
        testDB.mPaths.mTestContainer = AssetPath("test_mod//test/testContainer.lob");
    }

    // Create the domains
    if (!WaitForOp(mgr, mgr.CreateDomain(testDB.mPaths.mTestObjectA)))
    {
        return false;
    }
    if (!WaitForOp(mgr, mgr.CreateDomain(testDB.mPaths.mTestObjectB)))
    {
        return false;
    }
    if (!WaitForOp(mgr, mgr.CreateDomain(testDB.mPaths.mTestContainer)))
    {
        return false;
    }

    // Create the objects
    {
        auto object = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
        object->SetType(typeof(AssetMgrTestObject));
        object->mBaseHealth = testDB.mObjects.mTestObjectA.mBaseHealth;
        object->mBaseMana = testDB.mObjects.mTestObjectA.mBaseMana;

        if (!WaitForOp(mgr, mgr.Create(testDB.mPaths.mTestObjectA, object, nullptr)))
        {
            return false;
        }
    }

    {
        auto object = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
        object->SetType(typeof(AssetMgrTestObject));
        object->mBaseHealth = testDB.mObjects.mTestObjectB.mBaseHealth;
        object->mBaseMana = testDB.mObjects.mTestObjectB.mBaseMana;

        if (!WaitForOp(mgr, mgr.Create(testDB.mPaths.mTestObjectB, object, nullptr)))
        {
            return false;
        }
    }

    {
        const AssetLoadFlags::Value LOAD_FLAGS = AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_IMMEDIATE_PROPERTIES;

        auto object = MakeConvertibleAtomicPtr<AssetMgrTestContainer>();
        object->SetType(typeof(AssetMgrTestContainer));
        object->mObject = TestAsset<AssetMgrTestObject>(testDB.mPaths.mTestObjectA, LOAD_FLAGS);
        
        if (!WaitForOp(mgr, mgr.Create(testDB.mPaths.mTestContainer, object, nullptr)))
        {
            return false;
        }
    }

    // Save the domains

    if (!WaitForOp(mgr, mgr.SaveDomain(testDB.mPaths.mTestObjectA.GetDomain()))
     || !WaitForOp(mgr, mgr.SaveDomainCache(testDB.mPaths.mTestObjectA.GetDomain())))
    {
        return false;
    }

    if (!WaitForOp(mgr, mgr.SaveDomain(testDB.mPaths.mTestObjectB.GetDomain()))
     || !WaitForOp(mgr, mgr.SaveDomainCache(testDB.mPaths.mTestObjectB.GetDomain())))
    {
        return false;
    }

    if (!WaitForOp(mgr, mgr.SaveDomain(testDB.mPaths.mTestContainer.GetDomain()))
     || !WaitForOp(mgr, mgr.SaveDomainCache(testDB.mPaths.mTestContainer.GetDomain())))
    {
        return false;
    }

    return true;
}

bool ShutdownTestDB(AssetMgr& mgr, TestAssetDB& testDB)
{
    // Find the types
    auto objectTypeA = mgr.FindType(testDB.mPaths.mTestObjectA);
    auto objectTypeB = mgr.FindType(testDB.mPaths.mTestObjectB);
    auto containerType = mgr.FindType(testDB.mPaths.mTestContainer);

    // Delete them
    if (objectTypeA)
    {
        if (!WaitForOp(mgr, mgr.Delete(objectTypeA)))
        {
            return false;
        }
    }

    if (objectTypeB)
    {
        if (!WaitForOp(mgr, mgr.Delete(objectTypeB)))
        {
            return false;
        }
    }

    if (containerType)
    {
        if (!WaitForOp(mgr, mgr.Delete(containerType)))
        {
            return false;
        }
    }

    // Save the domain
    if (!WaitForOp(mgr, mgr.SaveDomain(testDB.mPaths.mTestObjectA.GetDomain()))
        || !WaitForOp(mgr, mgr.SaveDomainCache(testDB.mPaths.mTestObjectA.GetDomain())))
    {
        return false;
    }

    if (!WaitForOp(mgr, mgr.SaveDomain(testDB.mPaths.mTestObjectB.GetDomain()))
        || !WaitForOp(mgr, mgr.SaveDomainCache(testDB.mPaths.mTestObjectB.GetDomain())))
    {
        return false;
    }

    if (!WaitForOp(mgr, mgr.SaveDomain(testDB.mPaths.mTestContainer.GetDomain()))
        || !WaitForOp(mgr, mgr.SaveDomainCache(testDB.mPaths.mTestContainer.GetDomain())))
    {
        return false;
    }

    return true;
}

AssetPath GetStandardAssetPath()
{
    return AssetPath("engine//test/TestObject.lob");
}

static bool CreateStandardAsset(AssetMgr& mgr)
{
    AssetPath path(GetStandardAssetPath());
    auto testObj = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    testObj->SetType(typeof(AssetMgrTestObject));
    testObj->mBaseHealth = 4200;
    testObj->mBaseMana = 1600;

    String filepath = mgr.GetFullPath(path);
    if (FileSystem::FileExists(filepath))
    {
        return false;
    }

    // Create it
    AssetOpAtomicPtr op = mgr.Create(path, testObj, nullptr);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());
    return true;
}
static void DeleteStandardAsset(AssetMgr& mgr)
{
    AssetPath path(GetStandardAssetPath());
    AssetOpAtomicPtr op = mgr.Delete(mgr.FindType(path));
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());
}
static void LoadStandardAsset(AssetMgr& mgr)
{
    AssetPath path(GetStandardAssetPath());
    AssetLoadFlags::Value flags = AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_ASYNC;
    AssetOpAtomicPtr op = mgr.Load(mgr.FindType(path), flags);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());
}

REGISTER_TEST(AssetMgr_Initialize, "Runtime.Asset")
{
    AssetMgr mgr;
    DefaultInitialize(mgr);
    
    mgr.Shutdown();
}

REGISTER_TEST(AssetMgr_CreateDeleteType, "Runtime.Asset")
{
    AssetMgr mgr;
    DefaultInitialize(mgr);

    AssetPath path("engine//test/TestObject.lob");
    auto testObj = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    testObj->mBaseHealth = 4200;
    testObj->mBaseMana = 1600;
    testObj->SetType(typeof(AssetMgrTestObject));

    // Create the domain
    AssetOpAtomicWPtr op = mgr.CreateDomain(path);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    // Test it doesn't exist.
    String filepath = mgr.GetFullPath(path);
    TEST(FileSystem::FileExists(filepath) == false);
    TEST(mgr.FindType(path) == nullptr);

    // Create it
    op = mgr.Create(path, testObj, nullptr);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    // Save the domain (since this is not done after every create)
    op = mgr.SaveDomain(path.GetDomain());
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    // Verify we've created this source.
    TEST(FileSystem::FileExists(filepath) == true);
    TEST_CRITICAL(mgr.FindType(path) != nullptr);
    TEST(Valid(mgr.FindType(path)->GetCacheIndex()));
    mgr.Shutdown();

    // Reload asset mgr to verify we can load with it
    DefaultInitialize(mgr);
    TEST(FileSystem::FileExists(filepath) == true);
    TEST_CRITICAL(mgr.FindType(path) != nullptr);
    TEST(Valid(mgr.FindType(path)->GetCacheIndex()));

    // Delete
    op = mgr.Delete(mgr.FindType(path));
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    op = mgr.SaveDomain(path.GetDomain());
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    op = mgr.SaveDomainCache(path.GetDomain());
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    TEST(FileSystem::FileExists(filepath) == false);
    TEST(mgr.FindType(path) == nullptr);

    mgr.Shutdown();
}

REGISTER_TEST(AssetMgr_CreateAlreadyCreated, "Runtime.Asset")
{
    AssetMgr mgr;
    DefaultInitialize(mgr);

    AssetPath path("engine//test/TestObject.lob");
    auto testObj = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    testObj->mBaseHealth = 4200;
    testObj->mBaseMana = 1600;
    testObj->SetType(typeof(AssetMgrTestObject));

    // Create it
    AssetOpAtomicWPtr op = mgr.Create(path, testObj, nullptr);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    // Create it but fail because we've created it already.
    op = mgr.Create(path, testObj, nullptr);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsFailed());

    mgr.Shutdown();

    DefaultInitialize(mgr);

    // Create it but fail because we've created it already.
    op = mgr.Create(path, testObj, nullptr);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsFailed());

    // Cleanup/Delete it.
    op = mgr.Delete(mgr.FindType(path));
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    mgr.Shutdown();
}

REGISTER_TEST(AssetMgr_Import, "Runtime.Asset")
{
    AssetMgr mgr;
    DefaultInitialize(mgr);

    AssetPath path("engine//test/TestObject.lob");
    auto testObj = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    testObj->SetType(typeof(AssetMgrTestObject));
    testObj->mBaseHealth = 4200;
    testObj->mBaseMana = 1600;

    // Test it doesn't exist.
    String filepath = mgr.GetFullPath(path);
    TEST_CRITICAL(!FileSystem::FileExists(filepath));

    {
        TextStream ts;
        ts.Open(Stream::FILE, filepath, Stream::SM_WRITE);
        ts.BeginObject(path.GetName(), "Engine//Types/lf/AssetMgrTestObject");
        testObj->Serialize(ts);
        ts.EndObject();
        ts.Close();
    }
    TEST(FileSystem::FileExists(filepath));

    AssetOpAtomicWPtr op = mgr.Import(path) ;
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    // Cleanup/Delete it.
    op = mgr.Delete(mgr.FindType(path));
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    mgr.Shutdown();
}

REGISTER_TEST(AssetMgr_Load, "Runtime.Asset")
{
    // AssetLoadFlags::LF_ACQUIRE : 0 or 1
    // AssetLoadFlags::LF_IMMEDIATE_PROPERTIES : 0 or 1 
    // AssetLoadFlags::LF_RECURSIVE_PROPERTIES : 0 or 1

    AssetMgr mgr;
    DefaultInitialize(mgr);

    AssetPath path("engine//test/TestObject.lob");
    auto testObj = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    testObj->SetType(typeof(AssetMgrTestObject));
    testObj->mBaseHealth = 4200;
    testObj->mBaseMana = 1600;

    String filepath = mgr.GetFullPath(path);
    TEST_CRITICAL(!FileSystem::FileExists(filepath));

    {
        // Create it
        AssetOpAtomicPtr op = mgr.Create(path, testObj, nullptr);
        TEST(op->IsRunning());
        WaitForOp(mgr, op);
        TEST(op->IsSuccess());

        AssetLoadFlags::Value flags = AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_ASYNC;
        op = mgr.Load(mgr.FindType(path), flags);
        TEST(op->IsRunning());
        WaitForOp(mgr, op);
        TEST(op->IsSuccess());

        // Cleanup/Delete it.
        op = mgr.Delete(mgr.FindType(path));
        TEST(op->IsRunning());
        WaitForOp(mgr, op);
        TEST(op->IsSuccess());

    }
    mgr.Shutdown();
}


REGISTER_TEST(AssetMgr_CreateInstance, "Runtime.Asset")
{
    AssetMgr mgr;
    DefaultInitialize(mgr);
    TEST_CRITICAL(CreateStandardAsset(mgr));
    AssetObjectAtomicPtr instance = mgr.CreateAssetInstance(mgr.FindType(GetStandardAssetPath()));
    TEST(instance == NULL_PTR);

    LoadStandardAsset(mgr);

    instance = mgr.CreateAssetInstance(mgr.FindType(GetStandardAssetPath()));
    TEST(instance->IsA(typeof(AssetMgrTestObject)));
    AssetMgrTestObject* testObject = StaticCast<TStrongPointer<AssetMgrTestObject>>(instance);
    TEST(testObject->mBaseHealth == 4200);
    TEST(testObject->mBaseMana == 1600);
    instance = NULL_PTR;
    DeleteStandardAsset(mgr);
    mgr.Shutdown();
}

template<typename PointerStorage>
class Foo
{
public:
    typename PointerStorage mObject;
};


REGISTER_TEST(AssetMgr_LoadFlags, "Runtime.Asset")
{


    // Setup a test environment where we have 3 assets.
    // 

    // AssetLoadFlags::LF_ACQUIRE                   // TODO: Verify that we don't incur a load for non-loaded asset
    // AssetLoadFlags::LF_IMMEDIATE_PROPERTIES      // TODO: Verify properties are not loaded into prototype.
    // AssetLoadFlags::LF_RECURSIVE_PROPERTIES;     // TODO: Verify children are not loaded
    // AssetLoadFlags::LF_ASYNC;                    // TODO: Verify non-async calls are completed in frame.

    AssetMgr mgr;
    TestAssetDB testDB;
    TestAssetMgrProvider::sInstance = &mgr;

    // Setup
    DefaultInitialize(mgr);
    TEST(ConfigureTestDB(mgr, testDB));
    mgr.Shutdown();

    // Load Test
    DefaultInitialize(mgr);
    {
        const AssetLoadFlags::Value LOAD_FLAGS = AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_SOURCE;
        // 
        TestAsset<AssetMgrTestObject> testObject(testDB.mPaths.mTestObjectA, LOAD_FLAGS);
        

        auto instance = mgr.CreateInstance<AssetMgrTestObject>(testObject.GetType());

    }
    mgr.Shutdown();

    // Shutdown
    DefaultInitialize(mgr);
    TEST(ShutdownTestDB(mgr, testDB));
    mgr.Shutdown();
}

REGISTER_TEST(AssetMgr_Mod_CreateStress, "Runtime.Asset")
{
    AssetMgr mgr;
    DefaultInitialize(mgr);

    TestAssetMgrProvider::sInstance = &mgr;

    AssetPath engineDomain("engine//");
    AssetPath modDomain("test_mod//");



    // Create the domain
    AssetOpAtomicWPtr op = mgr.CreateDomain(engineDomain);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    op = mgr.CreateDomain(modDomain);
    TEST(op->IsRunning());
    WaitForOp(mgr, op);
    TEST(op->IsSuccess());

    AssetPath testObjectPathA("engine//test/TestObjectA.lob");
    AssetPath testObjectPathB("engine//test/TestObjectB.lob");
    AssetPath testContainerPath("test_mod//test/Container.lob");

    auto testObjectA = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    testObjectA->mBaseHealth = 1010;
    testObjectA->mBaseMana = 2020;
    testObjectA->SetType(typeof(AssetMgrTestObject));

    auto testObjectB = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    testObjectB->mBaseHealth = 3030;
    testObjectB->mBaseMana = 4040;
    testObjectB->SetType(typeof(AssetMgrTestObject));

    if (!mgr.FindType(testObjectPathA))
    {
        op = mgr.Create(testObjectPathA, testObjectA, nullptr);
        TEST(op->IsRunning());
        WaitForOp(mgr, op);
        TEST(op->IsSuccess());
    }

    if (!mgr.FindType(testObjectPathB))
    {
        op = mgr.Create(testObjectPathB, testObjectB, nullptr);
        TEST(op->IsRunning());
        WaitForOp(mgr, op);
        TEST(op->IsSuccess());
    }

    TEST(mgr.FindType(testObjectPathA));
    TEST(mgr.FindType(testObjectPathB));

    const AssetLoadFlags::Value LOAD_FLAGS = AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES;
    {
        TestAsset<AssetMgrTestObject> objectA(testObjectPathA, LOAD_FLAGS);
        TestAsset<AssetMgrTestObject> objectB(testObjectPathB, LOAD_FLAGS);

        TEST(objectA.IsLoaded());
        TEST(objectB.IsLoaded());

        auto testContainer = MakeConvertibleAtomicPtr<AssetMgrTestContainer>();
        testContainer->mObject = objectA;
        testContainer->SetType(typeof(AssetMgrTestContainer));

        if (!mgr.FindType(testContainerPath))
        {
            op = mgr.Create(testContainerPath, testContainer, nullptr);
            TEST(op->IsRunning());
            WaitForOp(mgr, op);
            TEST(op->IsSuccess());
        }
    }

    // AssetPath path("engine//test/TestObjectA.lob");
    // auto testObj = MakeConvertibleAtomicPtr<AssetMgrTestObject>();
    // testObj->mBaseHealth = 4200;
    // testObj->mBaseMana = 1600;
    // testObj->SetType(typeof(AssetMgrTestObject));
    // 
    // 
    // 
    // // Test it doesn't exist.
    // String filepath = mgr.GetFullPath(path);
    // TEST(FileSystem::FileExists(filepath) == false);
    // TEST(mgr.FindType(path) == nullptr);
    // 
    // // Create it
    // op = mgr.Create(path, testObj, nullptr);
    // TEST(op->IsRunning());
    // WaitForOp(mgr, op);
    // TEST(op->IsSuccess());
    // 
    
    // 
    // // Verify we've created this source.
    // TEST(FileSystem::FileExists(filepath) == true);
    // TEST_CRITICAL(mgr.FindType(path) != nullptr);
    // TEST(Valid(mgr.FindType(path)->GetCacheIndex()));

    mgr.Shutdown();

    DefaultInitialize(mgr);

    {
        TestAsset<AssetMgrTestObject> objectA(testObjectPathA, LOAD_FLAGS | AssetLoadFlags::LF_ACQUIRE);
        TestAsset<AssetMgrTestObject> objectB(testObjectPathB, LOAD_FLAGS | AssetLoadFlags::LF_ACQUIRE);

        TEST(!objectA.IsLoaded());
        TEST(!objectB.IsLoaded());

        TestAsset<AssetMgrTestContainer> container(testContainerPath, LOAD_FLAGS);
        TEST(container.IsLoaded());
        TEST(objectA.IsLoaded());
        TEST(!objectB.IsLoaded());
    }

    mgr.Shutdown();

    TestAssetMgrProvider::sInstance = nullptr;
}

REGISTER_TEST(AssetMgr_LoadStress, "Runtime.Asset", TestFlags::TF_STRESS)
{
    const SizeT COUNT = 10000;
    Int32 seed = 0xdba33230;
    TMap<Token, TAtomicStrongPointer<StressDataAsset>> assets;

    for (SizeT i = 0; i < COUNT; ++i)
    {
        ByteT bytes[16];
        for (SizeT k = 0; k < LF_ARRAY_SIZE(bytes); ++k)
        {
            bytes[k] = static_cast<ByteT>(Random::Mod(seed, 0xFF));
        }

        Token name(String("engine//StressAsset_") + ToString(bytes, sizeof(bytes)) + ".lob");
        auto& asset = assets[name];
        asset = MakeConvertibleAtomicPtr<StressDataAsset>();
        asset->SetType(typeof(StressDataAsset));
        asset->Generate(seed);
    }

    AssetMgr mgr;
    DefaultInitialize(mgr);
    TVector<AssetOpAtomicPtr> ops;

    ops.resize(assets.size());
    SizeT id = 0;
    
    for (const auto& pair : assets)
    {
        AssetPath path(pair.first);
        ops[id] = mgr.Create(path, pair.second, nullptr);
        TEST(ops[id]->IsRunning());
        // mgr.Update();
        ++id;
    }
    
    // Wait for all completed.
    SizeT completed = 0;
    while (completed != assets.size())
    {
        completed = 0;
        for (const auto& op : ops)
        {
            if (op->IsComplete())
            {
                TEST(op->IsSuccess());
                ++completed;
            }
        }
        mgr.Update();
    }
    
    id = 0;
    for (const auto& pair : assets)
    {
        AssetPath path(pair.first);
        ops[id] = mgr.Load(mgr.FindType(path), AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_ASYNC);
        TEST(ops[id]->IsRunning());
        ++id;
    }
    
    Timer t;
    t.Start();
    
    // Wait for all completed.
    completed = 0;
    while (completed != assets.size())
    {
        completed = 0;
        for (const auto& op : ops)
        {
            if (op->IsComplete())
            {
                TEST(op->IsSuccess());
                ++completed;
            }
        }
        mgr.Update();
    }
    
    t.Stop();
    
    gTestLog.Info(LogMessage("Loaded all types in ") << t.GetDelta() << " seconds.");
    
    id = 0;
    for (const auto& pair : assets)
    {
        AssetPath path(pair.first);
        ops[id] = mgr.Delete(mgr.FindType(path));
        TEST(ops[id]->IsRunning());
        mgr.Update();
        ++id;
    }
    
    // Wait for all completed.
    completed = 0;
    while (completed != assets.size())
    {
        completed = 0;
        for (const auto& op : ops)
        {
            if (op->IsComplete())
            {
                TEST(op->IsSuccess());
                ++completed;
            }
        }
        mgr.Update();
    }

    mgr.Shutdown();
}

REGISTER_TEST(AssetMgr_AssetHandleFunctions, "Runtime.Asset")
{
    const AssetLoadFlags::Value flags = AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_ASYNC;

    // By default all handles should be initialized to nullptr then use the asset mgr 'AcquireStrongNull' or 'AcquireWeakNull'.

    // Acquire Strong, AssetName
    {
        AssetMgr mgr;
        DefaultInitialize(mgr);
        TEST_CRITICAL(CreateStandardAsset(mgr));

        AssetHandle* handle = nullptr;
        TEST(!mgr.IsNull(handle)); // 'IsNull' in asset mgr terms is not exactly nullptr.
        mgr.AcquireStrongNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireStrong(reinterpret_cast<UnknownAssetHandle*&>(handle), GetStandardAssetPath(), typeof(AssetMgrTestObject), flags);
        TEST(!mgr.IsNull(handle));
        TEST(handle != nullptr);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_UNLOADED);
        WaitForAsset(mgr, handle);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_LOADED);
        TEST(handle->mPrototype != nullptr);

        mgr.AcquireStrongNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireStrong(reinterpret_cast<UnknownAssetHandle*&>(handle), GetStandardAssetPath(), typeof(AssetMgrTestObject), flags);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_LOADED);
        TEST(handle->mPrototype != nullptr);
        mgr.ReleaseStrong((reinterpret_cast<UnknownAssetHandle*&>(handle)));

        DeleteStandardAsset(mgr);
        mgr.Shutdown();
    }
    // Acquire Strong, AssetType
    {
        AssetMgr mgr;
        DefaultInitialize(mgr);
        TEST_CRITICAL(CreateStandardAsset(mgr));

        AssetHandle* handle = nullptr;
        TEST(!mgr.IsNull(handle));
        mgr.AcquireStrongNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireStrong(reinterpret_cast<UnknownAssetHandle*&>(handle), mgr.FindType(GetStandardAssetPath()), typeof(AssetMgrTestObject), flags);
        TEST(!mgr.IsNull(handle));
        TEST(handle != nullptr);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_UNLOADED);
        WaitForAsset(mgr, handle);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_LOADED);
        TEST(handle->mPrototype != nullptr);

        mgr.AcquireStrongNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireStrong(reinterpret_cast<UnknownAssetHandle*&>(handle), mgr.FindType(GetStandardAssetPath()), typeof(AssetMgrTestObject), flags);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_LOADED);
        TEST(handle->mPrototype != nullptr);
        mgr.ReleaseStrong((reinterpret_cast<UnknownAssetHandle*&>(handle)));

        DeleteStandardAsset(mgr);
        mgr.Shutdown();
    }
    // Acquire Weak, AssetName
    {
        AssetMgr mgr;
        DefaultInitialize(mgr);
        TEST_CRITICAL(CreateStandardAsset(mgr));

        AssetHandle* handle = nullptr;
        TEST(!mgr.IsNull(handle));
        mgr.AcquireWeakNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireWeak(reinterpret_cast<UnknownAssetHandle*&>(handle), mgr.FindType(GetStandardAssetPath()), typeof(AssetMgrTestObject));
        TEST(!mgr.IsNull(handle));
        TEST(handle != nullptr);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_UNLOADED);

        mgr.AcquireWeakNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireWeak(reinterpret_cast<UnknownAssetHandle*&>(handle), mgr.FindType(GetStandardAssetPath()), typeof(AssetMgrTestObject));
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_UNLOADED);

        mgr.ReleaseWeak((reinterpret_cast<UnknownAssetHandle*&>(handle)));

        DeleteStandardAsset(mgr);
        mgr.Shutdown();
    }
    // Acquire Weak, AssetType
    {
        AssetMgr mgr;
        DefaultInitialize(mgr);
        TEST_CRITICAL(CreateStandardAsset(mgr));

        AssetHandle* handle = nullptr;
        TEST(!mgr.IsNull(handle));
        mgr.AcquireWeakNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireWeak(reinterpret_cast<UnknownAssetHandle*&>(handle), GetStandardAssetPath(), typeof(AssetMgrTestObject));
        TEST(!mgr.IsNull(handle));
        TEST(handle != nullptr);
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_UNLOADED);

        mgr.AcquireWeakNull(reinterpret_cast<UnknownAssetHandle*&>(handle));
        TEST(mgr.IsNull(handle));
        mgr.AcquireWeak(reinterpret_cast<UnknownAssetHandle*&>(handle), GetStandardAssetPath(), typeof(AssetMgrTestObject));
        TEST(handle->mType->GetLoadState() == AssetLoadState::ALS_UNLOADED);

        mgr.ReleaseWeak((reinterpret_cast<UnknownAssetHandle*&>(handle)));

        DeleteStandardAsset(mgr);
        mgr.Shutdown();
    }
}

REGISTER_TEST(TAsset_Constructor, "Runtime.Asset")
{
    AssetMgr mgr;
    TestAssetMgrProvider::sInstance = &mgr;
    DefaultInitialize(mgr);
    TEST_CRITICAL(CreateStandardAsset(mgr));
    {
        TAsset<AssetMgrTestObject, TestAssetMgrProvider> asset(GetStandardAssetPath(), AssetLoadFlags::LF_IMMEDIATE_PROPERTIES | AssetLoadFlags::LF_RECURSIVE_PROPERTIES | AssetLoadFlags::LF_ASYNC);
        SizeT frame = 0;
        while (asset.GetType()->GetLoadState() != AssetLoadState::ALS_LOADED)
        {
            mgr.Update();
            ++frame;
        }

    }
    DeleteStandardAsset(mgr);
    mgr.Shutdown();
    TestAssetMgrProvider::sInstance = nullptr;
}

REGISTER_TEST(TAssetType_Constructor, "Runtime.Asset")
{
    AssetMgr mgr;
    TestAssetMgrProvider::sInstance = &mgr;
    DefaultInitialize(mgr);
    TEST_CRITICAL(CreateStandardAsset(mgr));
    {
        using TestBaseType = TestAssetType<AssetObject>;
        using TestType = TestAssetType<AssetMgrTestObject>;

        {
            TestType instance;
            TEST(instance == NULL_PTR);
            TEST(!instance);
            TEST(instance.GetType() == nullptr);
            TEST(instance.GetConcreteType() == nullptr);
            TEST(instance.GetPath().Empty());
            TEST(instance.GetWeakRefs() > 0);
            TEST(instance.GetStrongRefs());
            instance.Release();
            instance.Acquire(nullptr);

            instance = TestType();
            TEST(instance == NULL_PTR);
            TEST(!instance);
            TEST(instance.GetType() == nullptr);
            TEST(instance.GetConcreteType() == nullptr);
            TEST(instance.GetPath().Empty());
            TEST(instance.GetWeakRefs() > 0);
            TEST(instance.GetStrongRefs());
            instance.Release();
            instance.Acquire(nullptr);

            instance = TestType(NULL_PTR);
            TEST(instance == NULL_PTR);
            TEST(!instance);
            TEST(instance.GetType() == nullptr);
            TEST(instance.GetConcreteType() == nullptr);
            TEST(instance.GetPath().Empty());
            TEST(instance.GetWeakRefs() > 0);
            TEST(instance.GetStrongRefs());
            instance.Release();
            instance.Acquire(nullptr);

            instance = TestType(EMPTY_PATH);
            TEST(instance == NULL_PTR);
            TEST(!instance);
            TEST(instance.GetType() == nullptr);
            TEST(instance.GetConcreteType() == nullptr);
            TEST(instance.GetPath().Empty());
            TEST(instance.GetWeakRefs() > 0);
            TEST(instance.GetStrongRefs());
            instance.Release();
            instance.Acquire(nullptr);

            instance = TestType(nullptr);
            TEST(instance == NULL_PTR);
            TEST(!instance);
            TEST(instance.GetType() == nullptr);
            TEST(instance.GetConcreteType() == nullptr);
            TEST(instance.GetPath().Empty());
            TEST(instance.GetWeakRefs() > 0);
            TEST(instance.GetStrongRefs());
            instance.Release();
            instance.Acquire(nullptr);

            instance = TestType(TestType::StrongType());
            TEST(instance == NULL_PTR);
            TEST(!instance);
            TEST(instance.GetType() == nullptr);
            TEST(instance.GetConcreteType() == nullptr);
            TEST(instance.GetPath().Empty());
            TEST(instance.GetWeakRefs() > 0);
            TEST(instance.GetStrongRefs());
            instance.Release();
            instance.Acquire(nullptr);

            instance = TestType(TestType::WeakType());
            TEST(instance == NULL_PTR);
            TEST(!instance);
            TEST(instance.GetType() == nullptr);
            TEST(instance.GetConcreteType() == nullptr);
            TEST(instance.GetPath().Empty());
            TEST(instance.GetWeakRefs() > 0);
            TEST(instance.GetStrongRefs());
            instance.Release();
            instance.Acquire(nullptr);
        }
        
        {
            TestBaseType base(GetStandardAssetPath());
            TEST(base != NULL_PTR);

            TestType instance(GetStandardAssetPath());
            TEST(instance != NULL_PTR);

            TEST(instance.GetType() == base.GetType());
            TEST(instance == StaticCast<TestType>(base));

            TEST(base == instance);
        }
    }
    DeleteStandardAsset(mgr);
    mgr.Shutdown();
    TestAssetMgrProvider::sInstance = nullptr;
}

REGISTER_TEST(AssetDependencies, "Runtime.Asset")
{
    AssetMgr mgr;
    TestAssetMgrProvider::sInstance = &mgr;
    DefaultInitialize(mgr);

    TEST(TestUtils::CreateDataAsset(mgr, "engine//test//DataA.lob", TestData(5)));
    TEST(TestUtils::CreateDataAsset(mgr, "engine//test//DataB.lob", TestData(15)));
    TEST(TestUtils::CreateDataAsset(mgr, "engine//test//DataC.lob", TestData(60)));
    TEST(TestUtils::Flush(mgr));
    {
        TestDataAssetType DataA(AssetPath("engine//test//DataA.lob"));
        TestDataAssetType DataB(AssetPath("engine//test//DataB.lob"));
        TestDataAssetType DataC(AssetPath("engine//test//DataC.lob"));

        TestDataAsset StrongDataA(DataA, AssetLoadFlags::LF_ACQUIRE);
        TestDataAsset StrongDataB(DataB, AssetLoadFlags::LF_ACQUIRE);
        TestDataAsset StrongDataC(DataC, AssetLoadFlags::LF_ACQUIRE);

        TEST(TestUtils::CreateDataOwnerAssetType(mgr, "engine//test//OwnerDataA.lob", TestDataOwner(StrongDataA, DataA)));
        TEST(TestUtils::CreateDataOwnerAssetType(mgr, "engine//test//OwnerDataB.lob", TestDataOwner(StrongDataB, DataB)));
        TEST(TestUtils::CreateDataOwnerAssetType(mgr, "engine//test//OwnerDataC.lob", TestDataOwner(StrongDataC, DataC)));
        TEST(TestUtils::Flush(mgr));

        TestDataOwnerAsset StrongDataOwnerA(AssetPath("engine//test//OwnerDataA.lob"), AssetLoadFlags::LF_IMMEDIATE_PROPERTIES);
        TestDataOwnerAsset StrongDataOwnerB(AssetPath("engine//test//OwnerDataB.lob"), AssetLoadFlags::LF_IMMEDIATE_PROPERTIES);
        TestDataOwnerAsset StrongDataOwnerC(AssetPath("engine//test//OwnerDataC.lob"), AssetLoadFlags::LF_IMMEDIATE_PROPERTIES);

        TVector<Token> weak;
        TVector<Token> strong;

        DependencyStream ds(&weak, &strong);
        StrongDataOwnerA->Serialize(ds);
        ds.Close();

        TEST(weak.size() == 1);
        TEST(strong.size() == 1);

        TEST(TestUtils::DeleteAsset(mgr, DataA.GetType()));
        TEST(TestUtils::DeleteAsset(mgr, DataB.GetType()));
        TEST(TestUtils::DeleteAsset(mgr, DataC.GetType()));

        TEST(TestUtils::DeleteAsset(mgr, StrongDataOwnerA.GetType()));
        TEST(TestUtils::DeleteAsset(mgr, StrongDataOwnerB.GetType()));
        TEST(TestUtils::DeleteAsset(mgr, StrongDataOwnerC.GetType()));
    }

    mgr.Shutdown();
    TestAssetMgrProvider::sInstance = nullptr;
}

enum class ItemSoundSet
{
    None,
    Weapon,
    Ambient,
    Interact
};

void GenerateItemSoundSet(const String& itemName, TVector<String>& out, ItemSoundSet soundSet)
{
    switch (soundSet)
    {
    case ItemSoundSet::Weapon:
        out.push_back(itemName + "_attack0.wav");
        out.push_back(itemName + "_attack0.json");
        out.push_back(itemName + "_attack1.wav");
        out.push_back(itemName + "_attack1.json");
        out.push_back(itemName + "_attack2.wav");
        out.push_back(itemName + "_attack2.json");
        out.push_back(itemName + "_mix.json");
        break;
    case ItemSoundSet::Ambient:
        out.push_back(itemName + "_sounds.wav");
        out.push_back(itemName + "_sounds.json");
        out.push_back(itemName + "_mix.json");
        break;
    case ItemSoundSet::Interact:
        out.push_back(itemName + "_on_use.wav");
        out.push_back(itemName + "_on_use.json");
        out.push_back(itemName + "_mix.json");
        break;
    }
}

void GenerateItem(const String& itemName, TVector<String>& out, ItemSoundSet soundSet = ItemSoundSet::None)
{
    // Texture used for the item
    out.push_back(itemName + ".png");
    out.push_back(itemName + "_texture.json");
    // Mesh of the item
    out.push_back(itemName + ".fbx");
    out.push_back(itemName + "_mesh.json");
    // Material used to setup shaders for rendering item in world
    out.push_back(itemName + "_material.json");
    // Model data (Reference Mesh/Material)
    out.push_back(itemName + "_model.json");
    // Icon for the item
    out.push_back(itemName + "_icon.png");
    out.push_back(itemName + "_icon.json");
    // Configuration of the item
    out.push_back(itemName + ".json");

    GenerateItemSoundSet(itemName, out, soundSet);
}

void GenerateItemOverride(const String& itemName, TVector<String>& out, ItemSoundSet soundSet = ItemSoundSet::None)
{
    out.push_back(itemName + ".png");
    out.push_back(itemName + "_icon.png");
    out.push_back(itemName + ".json");
    GenerateItemSoundSet(itemName, out, soundSet);
}

void GenerateSpell(const String& spellName, TVector<String>& out, ItemSoundSet soundSet = ItemSoundSet::None)
{
    out.push_back(spellName + "_effect_texture.png");
    out.push_back(spellName + "_effect_color_map.png");
    out.push_back(spellName + "_effect.shader");
    out.push_back(spellName + "_effect_material.json");
    out.push_back(spellName + ".json");
    out.push_back(spellName + ".icon");
    GenerateItemSoundSet(spellName, out, soundSet);
}

void GenerateNpc(const String& npcName, TVector<String>& out)
{
    out.push_back(npcName + ".png");
    out.push_back(npcName + "_material.json");
    out.push_back(npcName + "_model.json");
    out.push_back(npcName + "_mesh.fbx");
    out.push_back(npcName + "_anim_walk.fbx");
    out.push_back(npcName + "_anim_run.fbx");
    out.push_back(npcName + "_anim_sprint.fbx");
    out.push_back(npcName + "_anim_attack0.fbx");
    out.push_back(npcName + "_anim_attack1.fbx");
    out.push_back(npcName + "_anim_jump.fbx");
    out.push_back(npcName + "_anim_land.fbx");
    out.push_back(npcName + "_anim_knockdown.fbx");
    out.push_back(npcName + "_anim_parry.fbx");
    out.push_back(npcName + "_anim_fire0.fbx");
    out.push_back(npcName + "_anim_fire1.fbx");
    out.push_back(npcName + "_anim_fire3.fbx");
    out.push_back(npcName + "_anim_kick.fbx");
    out.push_back(npcName + "_anim_dance.fbx");
    out.push_back(npcName + "_anim_cry.fbx");
    out.push_back(npcName + "_anim_emote0.fbx");
    out.push_back(npcName + "_anim_emote1.fbx");
    out.push_back(npcName + "_anim_emote2.fbx");
    out.push_back(npcName + "_anim_emote3.fbx");
    out.push_back(npcName + ".json");
    out.push_back(npcName + ".icon");

    out.push_back(npcName + "_default_inventory.json");
    out.push_back(npcName + "_drop_table.json");
    out.push_back(npcName + "_dialog.json");
    out.push_back(npcName + "_animation.json");

    out.push_back(npcName + "_footstep.wav");
    out.push_back(npcName + "_footstep.json");
    out.push_back(npcName + "_land.wav");
    out.push_back(npcName + "_land.json");
    out.push_back(npcName + "_emote0.wav");
    out.push_back(npcName + "_emote0.json");
    out.push_back(npcName + "_emote1.wav");
    out.push_back(npcName + "_emote1.json");
    out.push_back(npcName + "_emote2.wav");
    out.push_back(npcName + "_emote2.json");
    out.push_back(npcName + "_emote3.wav");
    out.push_back(npcName + "_emote3.json");
    out.push_back(npcName + "_soundmix.json");

    out.push_back(npcName + "_react.wav");
    out.push_back(npcName + "_react.json");
}

void HashTest(const TVector<String>& assetNames, const TVector<String>& assetScopes, const TVector<String>& domains)
{
    // This proves we should use StringHashTable over the TokenHashTable (it scales better)
    const SizeT COUNT = domains.size() * assetScopes.size() * assetNames.size();

    TVector<Float32> times;
    Float32 max = -99999999.0f;
    Float32 min = 99999999.0f;
    Float32 total = 0.0f;
    Float32 avg = 0.0f;

    SizeT ITERATION = 5;
    for (SizeT i = 0; i < ITERATION; ++i)
    {
        Timer t;
        t.Start();
        TVector<AssetPath> paths;
        for (const String& domain : domains)
        {
            for (const String& scope : assetScopes)
            {
                for (const String& name : assetNames)
                {
                    paths.push_back(AssetPath(domain + scope + name));
                }
            }
        }
        t.Stop();
        Float32 dt = ToMilliseconds(TimeTypes::Seconds(t.GetDelta())).mValue;
        max = Max(dt, max);
        min = Min(dt, min);
        total += dt;
        times.push_back(total);
    }

    avg = total / ITERATION;
    gTestLog.Info(LogMessage("TokenTable [") << COUNT << "] Avg=" << avg << ", Min=" << min << ", Max=" << max << ", Total=" << total);

    times.clear();
    max = -99999999.0f;
    min = 99999999.0f;
    total = 0.0f;

    for (SizeT i = 0; i < ITERATION; ++i)
    {
        StringHashTable tbl;
        Timer t;
        t.Start();
        TVector<StringHashTable::HashedString> paths;
        for (const String& domain : domains)
        {
            for (const String& scope : assetScopes)
            {
                for (const String& name : assetNames)
                {
                    String fullpath(domain + scope + name);
                    paths.push_back(tbl.Create(fullpath.CStr(), fullpath.Size()));
                }
            }
        }
        t.Stop();
        Float32 dt = ToMilliseconds(TimeTypes::Seconds(t.GetDelta())).mValue;
        max = Max(dt, max);
        min = Min(dt, min);
        total += dt;
        times.push_back(total);

        if (i == 0)
        {
            gTestLog.Info(LogMessage("Collisions=") << tbl.Collisions());
        }
    }
    avg = total / ITERATION;
    gTestLog.Info(LogMessage("StringHashTable [") << COUNT << "] Avg=" << avg << ", Min=" << min << ", Max=" << max << ", Total=" << total);
}

class AssetCacheVirtualFileWriter : public MemDBTypes::EntryWriter
{
public:
    AssetCacheVirtualFileWriter()
    {}
    ~AssetCacheVirtualFileWriter()
    {
        Release();
    }

    bool BeginCommit(SizeT tableCapacity, SizeT alignment) override
    {
        if (tableCapacity > mBuffer.GetCapacity())
        {
            if (!mBuffer.Reallocate(tableCapacity, alignment))
            {
                return false;
            }
        }
        return true;
    }

    void Commit(const ByteT* bytes, SizeT size, SizeT , SizeT offsetFromBase) override
    {
        SizeT targetSize = offsetFromBase + size;
        if (targetSize > mBuffer.GetCapacity())
        {
            return;
        }

        ByteT* dest = reinterpret_cast<ByteT*>(mBuffer.GetData()) + offsetFromBase;
        memcpy(dest, bytes, size);
    }

    void EndCommit() override
    {
        
    }

    void Release()
    {
        mBuffer.Free();
    }

private:
    MemoryBuffer mBuffer;
};

class AssetCacheFileWriter : public MemDBTypes::EntryWriter
{
public:
    AssetCacheFileWriter() {}
    AssetCacheFileWriter(const String& filename) { Open(filename); }
    ~AssetCacheFileWriter() { Release(); }

    void Open(const String& filename)
    {
        if (!mFile.Open(filename, FF_READ | FF_WRITE | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_ALWAYS))
        {
            gSysLog.Info(LogMessage("Failed to open CacheFileWriter ") << filename);
        }
    }

    bool BeginCommit(SizeT , SizeT ) override
    {
        return mFile.IsOpen();
    }

    void Commit(const ByteT* bytes, SizeT size, SizeT , SizeT offsetFromBase) override
    {
        if (!mFile.IsOpen())
        {
            return;
        }

        if (mFile.SetCursor(static_cast<FileCursor>(offsetFromBase), FILE_CURSOR_BEGIN))
        {
            mFile.Write(bytes, size);
        }
    }

    void EndCommit() override
    {
        if (mFile.IsOpen())
        {
            mFile.Close();
        }
    }

    void Release()
    {
        mFile.Close();
    }
private:
    File mFile;
};

struct AssetCacheSettings
{
    struct IndexSettings
    {
        void Serialize(Stream& s)
        {
            SERIALIZE(s, mDataType, "");
            SERIALIZE(s, mOffset, "");
            SERIALIZE(s, mUnique, "");
            SERIALIZE(s, mDescription, "");
            SERIALIZE(s, mClass, "");
            SERIALIZE(s, mMember, "");
        }

        friend LF_INLINE Stream& operator<<(Stream& s, IndexSettings& d)
        {
            d.Serialize(s);
            return s;
        }

        TNumericalVariantType mDataType;
        SizeT mOffset;
        bool mUnique;
        String mDescription;
        String mClass;
        String mMember;
    };

    struct TableSettings
    {
        void Serialize(Stream& s)
        {
            SERIALIZE(s, mName, "");
            SERIALIZE(s, mDefaultCapacity, "");
            SERIALIZE(s, mEntrySize, "");
            SERIALIZE(s, mEntryAlignment, "");
            SERIALIZE_STRUCT_ARRAY(s, mIndices, "");
        }

        bool ContainsIndex(NumericalVariantType::Value dataType, SizeT offset, bool unique) const
        {
            return std::find_if(mIndices.begin(), mIndices.end(),
                [dataType, offset, unique](const IndexSettings& settings) -> bool
                {
                    return settings.mDataType == dataType
                        && settings.mOffset == offset
                        && settings.mUnique == unique;
                }) != mIndices.end();
        }

        String mName;
        SizeT  mDefaultCapacity;
        SizeT  mEntrySize;
        SizeT  mEntryAlignment;
        TVector<IndexSettings> mIndices;

        friend LF_INLINE Stream& operator<<(Stream& s, TableSettings& d)
        {
            d.Serialize(s);
            return s;
        }
    };

    void Serialize(Stream& s)
    {
        SERIALIZE(s, mMultiFile, "");
        SERIALIZE(s, mCompressed, "");
        SERIALIZE_STRUCT_ARRAY(s, mTables, "");
    }

    TVector<TableSettings>::const_iterator FindTable(const String& name) const
    {
        return std::find_if(mTables.begin(), mTables.end(),
            [&name](const AssetCacheSettings::TableSettings& tableSettings)
            {
                return tableSettings.mName == name;;
            });
    }

    TVector<TableSettings> mTables;
    bool mMultiFile;
    bool mCompressed;
};

class AssetCacheDB
{
public:
    using PathHash = FNV::HashT;
    using DBPath = MemDBChar<140>;
    using CacheLocation = UInt64;
    using DatabaseID = MemDBTypes::EntryID;
    using WriterPtr = TStrongPointer<MemDBTypes::EntryWriter>;

    struct IndexTableEntry : public MemDBTypes::Entry
    {
        DatabaseID  mRuntimeID;
        DatabaseID  mMetaID;
        PathHash    mCacheHash;
        DBPath      mCachePath;
    };

    struct RuntimeTableEntry : public MemDBTypes::Entry
    {
        DatabaseID     mSuperID;
        PathHash       mSuperHash;
        CacheLocation  mCacheLocation;
        UInt32         mCacheMagicHeader;
        UInt32         mCacheMagicFooter;
        UInt32         mCacheOffset;
        UInt32         mCacheSize;
    };

    struct MetaTableEntry : public MemDBTypes::Entry
    {
        UInt32 mSizeRaw;
        UInt32 mSizeSource;
    };

    struct ReferenceTableBaseEntry : public MemDBTypes::Entry
    {
        DatabaseID mIndexID;
        DatabaseID mReferenceID;
    };
    using StrongReferenceTableEntry = ReferenceTableBaseEntry;
    using WeakReferenceTableEntry = ReferenceTableBaseEntry;

    bool Initialize();
    bool OpenFiles(const String& filename);
    void CloseFiles();

    // Operations

    // type = full asset path name
    DatabaseID GetAsset(const String& type);
    DatabaseID GetAsset(const String& type, PathHash hash);
    bool GetAssetInfo(DatabaseID id, IndexTableEntry& outInfo);
    // id = IndexTable
    DatabaseID GetRuntimeID(DatabaseID id);
    // id = IndexTable
    DatabaseID GetMetaID(DatabaseID id);
    // id = IndexTable
    TVector<DatabaseID> GetStrongReferences(DatabaseID id);
    // id = IndexTable
    bool DeleteStrongReferences(DatabaseID id);
    // id = IndexTable
    bool UpdateStrongReferences(DatabaseID id, const TVector<DatabaseID>& ids);

    TVector<DatabaseID> GetWeakReferences(DatabaseID id);
    bool DeleteWeakReferences(DatabaseID id);
    bool UpdateWeakReferences(DatabaseID id, const TVector<DatabaseID>& ids);

    bool GetRuntimeInfo(DatabaseID id, RuntimeTableEntry& outEntry);
    bool UpdateRuntimeInfo(DatabaseID id, const RuntimeTableEntry& entry);

    bool GetMetaInfo(DatabaseID id, MetaTableEntry& outEntry);
    bool UpdateMetaInfo(DatabaseID id, const MetaTableEntry& entry);

    bool CreateAsset(const String& type, const String& parent);
    bool CreateAssets(const TVector<std::pair<String, String>>& assets);

    void LogStats(Log& log);

    bool ValidateSettings(const AssetCacheSettings& settings);
    void CommitToVirtualFile();
private:
    enum : SizeT { EstimatedCount = ToKB<SizeT>(8) / 48 };

    static PathHash ComputeHash(const String& type)
    {
        return FNV::Hash(type.CStr(), type.Size());
    }

    struct IndexTable
    {
        LF_INLINE IndexTable() : mID(INVALID) {}

        static const char* GetName() { return "IndexTable"; }

        LF_INLINE bool CreateTable(MemDB& db)
        {
            return db.CreateTable<IndexTableEntry>(GetName(), EstimatedCount, mID)
                && db.CreateIndex(mID, NumericalVariantType::VT_U64, LF_OFFSET_OF(IndexTableEntry, mCacheHash));
        }
        LF_INLINE bool FindOne(MemDB& db, const String& type, DatabaseID& outID)
        {
            return FindOne(db, ComputeHash(type), type, outID);
        }

        LF_INLINE bool FindOne(MemDB& db, PathHash hash, const String& type, DatabaseID& outID)
        {
            outID = MemDBTypes::INVALID_ENTRY_ID;

            TVector<DatabaseID> ids;
            if (db.FindRangeIndexed(mID, NumericalVariant(hash), LF_OFFSET_OF(IndexTableEntry, mCacheHash), ids))
            {
                for (DatabaseID id : ids)
                {
                    db.SelectRead<IndexTableEntry>(mID, id,
                        [&outID, &type](const IndexTableEntry* entry)
                        {
                            if (type == entry->mCachePath.CStr())
                            {
                                outID = entry->mReservedID;
                            }
                        });

                    if (Valid(outID))
                    {
                        break;
                    }
                }
            }
            return Valid(outID);
        }

        LF_INLINE void InitializeSettings(AssetCacheSettings::TableSettings& settings)
        {
            settings.mName = GetName();
            settings.mEntrySize = sizeof(IndexTableEntry);
            settings.mEntryAlignment = alignof(IndexTableEntry);
            settings.mDefaultCapacity = 0;
            settings.mIndices.resize(0);
            
            AssetCacheSettings::IndexSettings indexSettings;
            indexSettings.mClass = "IndexTableEntry";
            indexSettings.mMember = "mCacheHash";
            indexSettings.mOffset = LF_OFFSET_OF(IndexTableEntry, mCacheHash);
            indexSettings.mDataType = NumericalVariantType::VT_U64;
            indexSettings.mDescription = "CacheHash";
            indexSettings.mUnique = true;
            settings.mIndices.push_back(indexSettings);
        }

        LF_INLINE bool ValidateSettings(const AssetCacheSettings& settings)
        {
            auto tableSettings = settings.FindTable(GetName());
            return tableSettings != settings.mTables.end()
                && tableSettings->mEntrySize == sizeof(IndexTableEntry)
                && tableSettings->mEntryAlignment == alignof(IndexTableEntry)
                && tableSettings->ContainsIndex(NumericalVariantType::VT_U64, LF_OFFSET_OF(IndexTableEntry, mCacheHash), true);
        }

        MemDB::TableID mID;
        WriterPtr      mWriter;
    };

    struct RuntimeTable
    {
        static const char* GetName() { return "RuntimeTable"; }

        LF_INLINE RuntimeTable() : mID(INVALID) {}
        LF_INLINE bool CreateTable(MemDB& db)
        {
            return db.CreateTable<RuntimeTableEntry>(GetName(), EstimatedCount, mID);
        }

        LF_INLINE void InitializeSettings(AssetCacheSettings::TableSettings& settings)
        {
            settings.mName = GetName();
            settings.mEntrySize = sizeof(RuntimeTableEntry);
            settings.mEntryAlignment = alignof(RuntimeTableEntry);
            settings.mDefaultCapacity = 0;
            settings.mIndices.resize(0);
        }

        LF_INLINE bool ValidateSettings(const AssetCacheSettings& settings)
        {
            auto tableSettings = settings.FindTable(GetName());
            return tableSettings != settings.mTables.end()
                && tableSettings->mEntrySize == sizeof(RuntimeTableEntry)
                && tableSettings->mEntryAlignment == alignof(RuntimeTableEntry);
        }

        MemDB::TableID mID;
        WriterPtr      mWriter;
    };

    struct MetaTable
    {
        static const char* GetName() { return "MetaTable"; }

        LF_INLINE MetaTable() : mID(INVALID) {}
        LF_INLINE bool CreateTable(MemDB& db)
        {
            return db.CreateTable<MetaTableEntry>(GetName(), EstimatedCount, mID);
        }

        LF_INLINE void InitializeSettings(AssetCacheSettings::TableSettings& settings)
        {
            settings.mName = GetName();
            settings.mEntrySize = sizeof(MetaTableEntry);
            settings.mEntryAlignment = alignof(MetaTableEntry);
            settings.mDefaultCapacity = 0;
            settings.mIndices.resize(0);
        }

        LF_INLINE bool ValidateSettings(const AssetCacheSettings& settings)
        {
            auto tableSettings = settings.FindTable(GetName());
            return tableSettings != settings.mTables.end()
                && tableSettings->mEntrySize == sizeof(MetaTableEntry)
                && tableSettings->mEntryAlignment == alignof(MetaTableEntry);
        }

        MemDB::TableID mID;
        WriterPtr      mWriter;
    };

    
    struct ReferenceTableBase : public MemDBTypes::Entry
    {
        LF_INLINE ReferenceTableBase() : mID(INVALID) {}
        LF_INLINE bool CreateTable(MemDB& db, const String& name)
        {
            return db.CreateTable<ReferenceTableBaseEntry>(name, mID)
                && db.CreateIndex(mID, NumericalVariantType::VT_U32, LF_OFFSET_OF(ReferenceTableBaseEntry, mIndexID), true)
                && db.CreateIndex(mID, NumericalVariantType::VT_U32, LF_OFFSET_OF(ReferenceTableBaseEntry, mReferenceID), true);
        }

        // Find all the references owned by 'id'
        LF_INLINE bool FindAll(MemDB& db, DatabaseID id, TVector<DatabaseID>& outIDs)
        {
            return db.FindRangeIndexed(mID, NumericalVariant(id), LF_OFFSET_OF(ReferenceTableBaseEntry, mIndexID), outIDs);
        }

        // Find all references to 'id'
        LF_INLINE bool FindAllReferences( MemDB& db, DatabaseID id, TVector<DatabaseID>& outIDs)
        {
            return db.FindRangeIndexed(mID, NumericalVariant(id), LF_OFFSET_OF(ReferenceTableBaseEntry, mReferenceID), outIDs);
        }

        LF_INLINE void InitializeSettings(AssetCacheSettings::TableSettings& settings, const String& name)
        {
            settings.mName = name;
            settings.mEntrySize = sizeof(ReferenceTableBaseEntry);
            settings.mEntryAlignment = alignof(ReferenceTableBaseEntry);
            settings.mDefaultCapacity = 0;
            settings.mIndices.resize(0);

            AssetCacheSettings::IndexSettings indexSettings;
            indexSettings.mClass = "ReferenceTableBaseEntry";
            indexSettings.mMember = "mIndexID";
            indexSettings.mOffset = LF_OFFSET_OF(ReferenceTableBaseEntry, mIndexID);
            indexSettings.mDataType = NumericalVariantType::VT_U32;
            indexSettings.mDescription = "IndexID";
            indexSettings.mUnique = false;
            settings.mIndices.push_back(indexSettings);

            indexSettings.mClass = "ReferenceTableBaseEntry";
            indexSettings.mMember = "mReferenceID";
            indexSettings.mOffset = LF_OFFSET_OF(ReferenceTableBaseEntry, mReferenceID);
            indexSettings.mDataType = NumericalVariantType::VT_U32;
            indexSettings.mDescription = "ReferenceID";
            indexSettings.mUnique = false;
            settings.mIndices.push_back(indexSettings);
        }

        LF_INLINE bool ValidateSettings(const AssetCacheSettings& settings, const String& name)
        {
            auto tableSettings = settings.FindTable(name);
            return tableSettings != settings.mTables.end()
                && tableSettings->mEntrySize == sizeof(ReferenceTableBaseEntry)
                && tableSettings->mEntryAlignment == alignof(ReferenceTableBaseEntry)
                && tableSettings->ContainsIndex(NumericalVariantType::VT_U32, LF_OFFSET_OF(ReferenceTableBaseEntry, mIndexID), false)
                && tableSettings->ContainsIndex(NumericalVariantType::VT_U32, LF_OFFSET_OF(ReferenceTableBaseEntry, mReferenceID), false);
        }

        MemDB::TableID mID;
        WriterPtr      mWriter;
    };
    
    struct StrongReferenceTable : public ReferenceTableBase
    {
        static const char* GetName() { return "StrongReferenceTable"; }

        LF_INLINE bool CreateTable(MemDB& db)
        {
            return ReferenceTableBase::CreateTable(db, GetName());
        }

        LF_INLINE void InitializeSettings(AssetCacheSettings::TableSettings& settings)
        {
            ReferenceTableBase::InitializeSettings(settings, GetName());
        }

        LF_INLINE bool ValidateSettings(const AssetCacheSettings& settings)
        {
            return ReferenceTableBase::ValidateSettings(settings, GetName());
        }
    };

    struct WeakReferenceTable : public ReferenceTableBase
    {
        static const char* GetName() { return "WeakReferenceTable"; }

        LF_INLINE bool CreateTable(MemDB& db)
        {
            return ReferenceTableBase::CreateTable(db, GetName());
        }

        LF_INLINE void InitializeSettings(AssetCacheSettings::TableSettings& settings)
        {
            ReferenceTableBase::InitializeSettings(settings, GetName());
        }

        LF_INLINE bool ValidateSettings(const AssetCacheSettings& settings)
        {
            return ReferenceTableBase::ValidateSettings(settings, GetName());
        }
    };

    

    AssetCacheSettings GetDefaultSettings();
    bool LoadTableData(const String& tableName, MemDB::TableID tableID);

    String                  mFilename;
    AssetCacheSettings      mSettings;

    MemDB                   mDB;
    IndexTable              mIndexTable;
    RuntimeTable            mRuntimeTable;
    MetaTable               mMetaTable;
    StrongReferenceTable    mStrongReferenceTable;
    WeakReferenceTable      mWeakReferenceTable;

}; // namespace AssetCacheDB

bool AssetCacheDB::Initialize()
{
    return mIndexTable.CreateTable(mDB)
        && mRuntimeTable.CreateTable(mDB)
        && mMetaTable.CreateTable(mDB)
        && mStrongReferenceTable.CreateTable(mDB)
        && mWeakReferenceTable.CreateTable(mDB);
}


bool AssetCacheDB::OpenFiles(const String& filename)
{
    bool newFile = true;
    AssetCacheSettings settings;

    String json;
    if (!filename.Empty() && File::ReadAllText(filename + ".json", json))
    {
        JsonStream js(Stream::TEXT, &json, Stream::SM_READ);
        if (js.IsReading())
        {
            settings.Serialize(js);
            newFile = false;
        }
    }

    // Verify we're compatible with the settings.
    if (newFile)
    {
        settings = GetDefaultSettings();
    }
    else
    {
        if (!ValidateSettings(settings))
        {
            return false;
        }

    }
    mSettings = settings;
    mFilename = filename;

    

    bool tablesLoaded = LoadTableData(mIndexTable.GetName(), mIndexTable.mID)
        && LoadTableData(mRuntimeTable.GetName(), mRuntimeTable.mID)
        && LoadTableData(mMetaTable.GetName(), mMetaTable.mID)
        && LoadTableData(mStrongReferenceTable.GetName(), mStrongReferenceTable.mID)
        && LoadTableData(mWeakReferenceTable.GetName(), mWeakReferenceTable.mID);

    if (!tablesLoaded)
    {
        return false;
    }

    mIndexTable.mWriter = WriterPtr(LFNew<AssetCacheFileWriter>(mFilename + "_" + mIndexTable.GetName() + ".db"));
    mRuntimeTable.mWriter = WriterPtr(LFNew<AssetCacheFileWriter>(mFilename + "_" + mRuntimeTable.GetName() + ".db"));
    mMetaTable.mWriter = WriterPtr(LFNew<AssetCacheFileWriter>(mFilename + "_" + mMetaTable.GetName() + ".db"));
    mStrongReferenceTable.mWriter = WriterPtr(LFNew<AssetCacheFileWriter>(mFilename + "_" + mStrongReferenceTable.GetName() + ".db"));
    mWeakReferenceTable.mWriter = WriterPtr(LFNew<AssetCacheFileWriter>(mFilename + "_" + mWeakReferenceTable.GetName() + ".db"));

    return true;
}

void AssetCacheDB::CloseFiles()
{
    String json;
    JsonStream js(Stream::TEXT, &json, Stream::SM_PRETTY_WRITE);
    mSettings.Serialize(js);
    js.Close();

    File::WriteAllText(mFilename + ".json", json);

    mDB.CommitDirty(mIndexTable.mID, mIndexTable.mWriter);
    mDB.CommitDirty(mRuntimeTable.mID, mRuntimeTable.mWriter);
    mDB.CommitDirty(mMetaTable.mID, mMetaTable.mWriter);
    mDB.CommitDirty(mStrongReferenceTable.mID, mStrongReferenceTable.mWriter);
    mDB.CommitDirty(mWeakReferenceTable.mID, mWeakReferenceTable.mWriter);
    // mDB.CommitIndex(mIndexTable.mID, mIndexTable.mWriter);
}

AssetCacheDB::DatabaseID AssetCacheDB::GetAsset(const String& type)
{
    DatabaseID id = MemDBTypes::INVALID_ENTRY_ID;
    mIndexTable.FindOne(mDB, type, id);
    return id;
}
AssetCacheDB::DatabaseID AssetCacheDB::GetAsset(const String& type, PathHash hash)
{
    DatabaseID id = MemDBTypes::INVALID_ENTRY_ID;
    mIndexTable.FindOne(mDB, hash, type, id);
    return id;
}

bool AssetCacheDB::GetAssetInfo(DatabaseID id, IndexTableEntry& outInfo)
{
    return mDB.SelectRead<IndexTableEntry>(mIndexTable.mID, id,
        [&outInfo](const IndexTableEntry* entry)
        {
            outInfo = *entry;
        });
}

AssetCacheDB::DatabaseID AssetCacheDB::GetRuntimeID(DatabaseID id)
{
    DatabaseID runtimeID = MemDBTypes::INVALID_ENTRY_ID;
    mDB.SelectRead<IndexTableEntry>(mIndexTable.mID, id,
        [&runtimeID](const IndexTableEntry* entry)
        {
            runtimeID = entry->mRuntimeID;
        });
    return runtimeID;
}

AssetCacheDB::DatabaseID AssetCacheDB::GetMetaID(DatabaseID id)
{
    DatabaseID metaID = MemDBTypes::INVALID_ENTRY_ID;
    mDB.SelectRead<IndexTableEntry>(mIndexTable.mID, id,
        [&metaID](const IndexTableEntry* entry)
        {
            metaID = entry->mMetaID;
        });
    return metaID;
}

TVector<AssetCacheDB::DatabaseID> AssetCacheDB::GetStrongReferences(DatabaseID id)
{
    TVector<DatabaseID> referenceIds;
    mStrongReferenceTable.FindAll(mDB, id, referenceIds);

    TVector<DatabaseID> ids;
    for (DatabaseID referenceId : referenceIds)
    {
        mDB.SelectRead<StrongReferenceTableEntry>(mStrongReferenceTable.mID, referenceId,
            [&ids](const StrongReferenceTableEntry* entry)
            {
                ids.push_back(entry->mReferenceID);
            });
    }
    return ids;
}

bool AssetCacheDB::DeleteStrongReferences(DatabaseID id)
{
    TVector<DatabaseID> ids;
    mStrongReferenceTable.FindAll(mDB, id, ids);

    bool fail = false;
    for (DatabaseID tableID : ids)
    {
        if (!mDB.Delete(mStrongReferenceTable.mID, tableID))
        {
            fail = true;
        }
    }
    return !fail;
}

bool AssetCacheDB::UpdateStrongReferences(DatabaseID id, const TVector<DatabaseID>& ids)
{
    if (!DeleteStrongReferences(id))
    {
        return false;
    }

    bool fail = false;
    for (DatabaseID referenceID : ids)
    {

        StrongReferenceTableEntry entry;
        entry.mIndexID = id;
        entry.mReferenceID = referenceID;
        DatabaseID dummyId;
        if (!mDB.Insert(mStrongReferenceTable.mID, entry, dummyId))
        {
            fail = true;
        }
    }
    return !fail;
}

TVector<AssetCacheDB::DatabaseID> AssetCacheDB::GetWeakReferences(DatabaseID id)
{
    TVector<DatabaseID> referenceIds;
    mWeakReferenceTable.FindAll(mDB, id, referenceIds);

    TVector<DatabaseID> ids;
    for (DatabaseID referenceId : referenceIds)
    {
        mDB.SelectRead<StrongReferenceTableEntry>(mWeakReferenceTable.mID, referenceId,
            [&ids](const StrongReferenceTableEntry* entry)
            {
                ids.push_back(entry->mReferenceID);
            });
    }
    return ids;
}

bool AssetCacheDB::DeleteWeakReferences(DatabaseID id)
{
    TVector<DatabaseID> ids;
    mWeakReferenceTable.FindAll(mDB, id, ids);

    bool fail = false;
    for (DatabaseID tableID : ids)
    {
        if (!mDB.Delete(mWeakReferenceTable.mID, tableID))
        {
            fail = true;
        }
    }
    return !fail;
}

bool AssetCacheDB::UpdateWeakReferences(DatabaseID id, const TVector<DatabaseID>& ids)
{
    if (!DeleteStrongReferences(id))
    {
        return false;
    }

    bool fail = false;
    for (DatabaseID referenceID : ids)
    {

        StrongReferenceTableEntry entry;
        entry.mIndexID = id;
        entry.mReferenceID = referenceID;
        DatabaseID dummyId;
        if (!mDB.Insert(mWeakReferenceTable.mID, entry, dummyId))
        {
            fail = true;
        }
    }
    return !fail;
}

bool AssetCacheDB::GetRuntimeInfo(DatabaseID id, RuntimeTableEntry& outEntry)
{
    return mDB.SelectRead<RuntimeTableEntry>(mRuntimeTable.mID, id,
        [&outEntry](const RuntimeTableEntry* entry)
        {
            outEntry = *entry;
        });
}

bool AssetCacheDB::UpdateRuntimeInfo(DatabaseID id, const RuntimeTableEntry& entry)
{
    return mDB.SelectWrite<RuntimeTableEntry>(mRuntimeTable.mID, id,
        [&entry](RuntimeTableEntry* record)
        {
            *record = entry;
        });
}

bool AssetCacheDB::GetMetaInfo(DatabaseID id, MetaTableEntry& outEntry)
{
    return mDB.SelectRead<MetaTableEntry>(mMetaTable.mID, id,
        [&outEntry](const MetaTableEntry* entry)
        {
            outEntry = *entry;
        });
}

bool AssetCacheDB::UpdateMetaInfo(DatabaseID id, const MetaTableEntry& entry)
{
    return mDB.SelectWrite<MetaTableEntry>(mMetaTable.mID, id,
        [&entry](MetaTableEntry* record)
        {
            *record = entry;
        });
}

bool AssetCacheDB::CreateAsset(const String& type, const String& parent)
{
    DatabaseID superID = MemDBTypes::INVALID_ENTRY_ID;
    PathHash superHash = 0;
    if (!parent.Empty())
    {
        superHash = ComputeHash(parent);
        superID = GetAsset(parent, superHash);;
        if (Invalid(superID))
        {
            return false;
        }
    }

    IndexTableEntry indexEntry;
    indexEntry.mCacheHash = ComputeHash(type);
    indexEntry.mCachePath.Assign(type.CStr());
    indexEntry.mMetaID = MemDBTypes::INVALID_ENTRY_ID;
    indexEntry.mRuntimeID = MemDBTypes::INVALID_ENTRY_ID;
    
    DatabaseID indexID = MemDBTypes::INVALID_ENTRY_ID;
    if (!mDB.Insert(mIndexTable.mID, indexEntry, indexID))
    {
        return false;
    }

    RuntimeTableEntry runtimeEntry;
    runtimeEntry.mSuperID = superID;
    runtimeEntry.mSuperHash = superHash;
    runtimeEntry.mCacheSize = 0;
    runtimeEntry.mCacheOffset = 0;
    runtimeEntry.mCacheLocation = INVALID64;
    runtimeEntry.mCacheMagicFooter = INVALID32;
    runtimeEntry.mCacheMagicHeader = INVALID32;

    MetaTableEntry metaEntry;
    metaEntry.mSizeRaw = 0;
    metaEntry.mSizeSource = 0;
    
    DatabaseID runtimeID = MemDBTypes::INVALID_ENTRY_ID;
    DatabaseID metaID = MemDBTypes::INVALID_ENTRY_ID;
    if (!mDB.Insert(mRuntimeTable.mID, runtimeEntry, runtimeID)
     || !mDB.Insert(mMetaTable.mID, metaEntry, metaID))
    {
        if (Valid(runtimeID))
        {
            mDB.Delete(mRuntimeTable.mID, runtimeID);
        }
        if (Valid(metaID))
        {
            mDB.Delete(mMetaTable.mID, metaID);
        }
        mDB.Delete(mIndexTable.mID, indexID);
        return false;
    }

    Assert(mDB.SelectWrite<IndexTableEntry>(mIndexTable.mID, indexID,
        [runtimeID, metaID](IndexTableEntry* entry)
        {
            entry->mRuntimeID = runtimeID;
            entry->mMetaID = metaID;
        }));
    return true;
}

bool AssetCacheDB::CreateAssets(const TVector<std::pair<String, String>>& assets)
{
    if (assets.empty())
    {
        return true;
    }

    TVector<PathHash> superHashes; superHashes.reserve(assets.size());
    TVector<DatabaseID> superIDs; superIDs.reserve(assets.size());
    TVector<IndexTableEntry> indexEntries; indexEntries.reserve(assets.size());
    for (const auto& pair : assets)
    {
        const String& type = pair.first;
        const String& parent = pair.second;

        DatabaseID superID = MemDBTypes::INVALID_ENTRY_ID;
        PathHash superHash = 0;
        if (!parent.Empty())
        {
            superHash = ComputeHash(parent);
            superID = GetAsset(parent, superHash);;
            if (Invalid(superID))
            {
                return false;
            }
        }

        superHashes.push_back(superHash);
        superIDs.push_back(superID);

        IndexTableEntry indexEntry;
        indexEntry.mCacheHash = ComputeHash(type);
        indexEntry.mCachePath.Assign(type.CStr());
        indexEntry.mMetaID = MemDBTypes::INVALID_ENTRY_ID;
        indexEntry.mRuntimeID = MemDBTypes::INVALID_ENTRY_ID;
        indexEntries.push_back(indexEntry);
    }


    TVector<DatabaseID> indexIDs;
    if (!mDB.BulkInsert(mIndexTable.mID, indexEntries, indexIDs))
    {
        return false;
    }

    Assert(indexIDs.size() == indexEntries.size());
    Assert(superHashes.size() == indexEntries.size());
    Assert(superIDs.size() == indexEntries.size());

    TVector<RuntimeTableEntry> runtimeEntries; runtimeEntries.reserve(assets.size());
    TVector<MetaTableEntry> metaEntries; metaEntries.reserve(assets.size());

    for (SizeT i = 0; i < indexIDs.size(); ++i)
    {
        DatabaseID superID = superIDs[i];
        PathHash superHash = superHashes[i];

        RuntimeTableEntry runtimeEntry;
        runtimeEntry.mSuperID = superID;
        runtimeEntry.mSuperHash = superHash;
        runtimeEntry.mCacheSize = 0;
        runtimeEntry.mCacheOffset = 0;
        runtimeEntry.mCacheLocation = INVALID64;
        runtimeEntry.mCacheMagicFooter = INVALID32;
        runtimeEntry.mCacheMagicHeader = INVALID32;

        MetaTableEntry metaEntry;
        metaEntry.mSizeRaw = 0;
        metaEntry.mSizeSource = 0;

        runtimeEntries.push_back(runtimeEntry);
        metaEntries.push_back(metaEntry);
    }

    TVector<DatabaseID> runtimeIDs;
    TVector<DatabaseID> metaIDs;
    if (!mDB.BulkInsert(mRuntimeTable.mID, runtimeEntries, runtimeIDs)
        || !mDB.BulkInsert(mMetaTable.mID, metaEntries, metaIDs))
    {
        for (DatabaseID id : runtimeIDs)
        {
            mDB.Delete(mRuntimeTable.mID, id);
        }

        for (DatabaseID id : metaIDs)
        {
            mDB.Delete(mMetaTable.mID, id);
        }

        for (DatabaseID id : indexIDs)
        {
            mDB.Delete(mIndexTable.mID, id);
        }
        return false;
    }
    

    for (SizeT i = 0; i < indexIDs.size(); ++i)
    {
        DatabaseID runtimeID = runtimeIDs[i];
        DatabaseID metaID = metaIDs[i];

        Assert(mDB.SelectWrite<IndexTableEntry>(mIndexTable.mID, indexIDs[i],
            [runtimeID, metaID](IndexTableEntry* entry)
            {
                entry->mRuntimeID = runtimeID;
                entry->mMetaID = metaID;
            }));
    }

    return true;
}

void LogStatsCommon(Log& log, const MemDBStats& stats, const String& table, SizeT entrySize)
{
    log.Info(LogMessage("Displaying stats for table ") << table);
    log.Info(LogMessage("  Runtime Bytes Reserved: ") << stats.mRuntimeBytesReserved);
    log.Info(LogMessage("  Runtime Bytes Used:     ") << stats.mRuntimeBytesUsed);
    log.Info(LogMessage("  Data Bytes Reserved:    ") << stats.mDataBytesReserved);
    log.Info(LogMessage("  Data Bytes Used:        ") << stats.mDataBytesUsed);
    log.Info(LogMessage("  Resize Count:           ") << stats.mResizeCount);
    log.Info(LogMessage("  Entry Size:             ") << entrySize);
}

void AssetCacheDB::LogStats(Log& log)
{
    LogStatsCommon(log, mDB.GetTableStats(mIndexTable.mID), "IndexTable", sizeof(IndexTableEntry));
    LogStatsCommon(log, mDB.GetTableStats(mRuntimeTable.mID), "RuntimeTable", sizeof(RuntimeTableEntry));
    LogStatsCommon(log, mDB.GetTableStats(mMetaTable.mID), "MetaTable", sizeof(MetaTableEntry));
    LogStatsCommon(log, mDB.GetTableStats(mStrongReferenceTable.mID), "StrongReferenceTable", sizeof(StrongReferenceTableEntry));
    LogStatsCommon(log, mDB.GetTableStats(mWeakReferenceTable.mID), "WeakReferenceTable", sizeof(WeakReferenceTableEntry));
    log.Sync();
}

bool AssetCacheDB::ValidateSettings(const AssetCacheSettings& settings)
{
    return mIndexTable.ValidateSettings(settings)
        && mRuntimeTable.ValidateSettings(settings)
        && mMetaTable.ValidateSettings(settings)
        && mStrongReferenceTable.ValidateSettings(settings)
        && mWeakReferenceTable.ValidateSettings(settings);
}

AssetCacheSettings AssetCacheDB::GetDefaultSettings()
{
    AssetCacheSettings settings;
    settings.mMultiFile = false;
    settings.mCompressed = false;

    AssetCacheSettings::TableSettings tableSettings;
    mIndexTable.InitializeSettings(tableSettings);
    settings.mTables.push_back(tableSettings);

    mRuntimeTable.InitializeSettings(tableSettings);
    settings.mTables.push_back(tableSettings);

    mMetaTable.InitializeSettings(tableSettings);
    settings.mTables.push_back(tableSettings);

    mStrongReferenceTable.InitializeSettings(tableSettings);
    settings.mTables.push_back(tableSettings);

    mWeakReferenceTable.InitializeSettings(tableSettings);
    settings.mTables.push_back(tableSettings);

    return settings;
}

bool AssetCacheDB::LoadTableData(const String& tableName, MemDB::TableID tableID)
{
    const String filename = mFilename + "_" + tableName + ".db";
    File file;
    if (!file.Open(filename, FF_READ | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_EXISTING))
    {
        return !FileSystem::FileExists(filename);
    }

    const SizeT size = file.Size();
    if (size == 0)
    {
        return true;
    }

    TVector<ByteT> bytes;
    bytes.resize(size);
    if (file.Read(bytes.data(), size) != size)
    {
        return false;
    }
    file.Close();

    return mDB.LoadTableData(tableID, bytes.data(), size);
}

struct SimpleAssetTypeInfo
{
    AssetCacheDB::DatabaseID mID;
    AssetCacheDB::DatabaseID mRuntimeID;
    AssetCacheDB::DatabaseID mMetaID;

    AssetCacheDB::DatabaseID mSuperID;
    AssetCacheDB::PathHash   mSuperHash;

    String mAssetName;
    String mSuperName;

    UInt64 mCacheLocation;


    // AssetTypeInfo& assetType = context->mStaticTable.mTypes[index];
    // assetType.mPath.SetPath(data.mPath.CStr());
    // if (assetType.mPath.Empty())
    // {
    //     gSysLog.Warning(LogMessage("Failed to initialize AssetTypeInfo 'bad name'. Name=") << data.mPath << ", ConcreteType=" << data.mConcreteType);
    //     continue;
    // }
    // assetType.mConcreteType = GetReflectionMgr().FindType(data.mConcreteType);
    // assetType.mParent = nullptr;
    // assetType.mCacheIndex.mBlobID = data.mCacheBlobID;
    // assetType.mCacheIndex.mObjectID = data.mCacheObjectID;
    // assetType.mCacheIndex.mUID = data.mCacheUID;
    // assetType.mWeakReferences = data.mWeakReferences;
    // assetType.mStrongReferences = data.mStrongReferences;


};

class AssetCacheEntry
{
public:

    void Serialize(Stream& s)
    {
        SERIALIZE(s, mCacheName, "");
        SERIALIZE(s, mSuperName, "");
        SERIALIZE(s, mCacheLocation, "");
        SERIALIZE(s, mCacheMagicHeader, "");
        SERIALIZE(s, mCacheMagicFooter, "");
        SERIALIZE(s, mSizeRaw, "");
        SERIALIZE(s, mSizeSource, "");
        SERIALIZE_ARRAY(s, mStrongReferences, "");
        SERIALIZE_ARRAY(s, mWeakReferences, "");

        mCacheHash = FNV::Hash(mCacheName.CStr(), mCacheName.Size());
        mSuperHash = FNV::Hash(mSuperName.CStr(), mSuperName.Size());
    }

    String mCacheName;
    String mSuperName;
    UInt64 mCacheLocation;
    UInt32 mCacheMagicHeader;
    UInt32 mCacheMagicFooter;
    UInt32 mSizeRaw;
    UInt32 mSizeSource;
    TVector<String> mStrongReferences;
    TVector<String> mWeakReferences;

    FNV::HashT mCacheHash;
    FNV::HashT mSuperHash;
};

Stream& operator<<(Stream& s, AssetCacheEntry& d)
{
    d.Serialize(s);
    return s;
}

class AssetCacheRegistry
{
public:
    using ArrayType = TVector<AssetCacheEntry>;
    using HashMap = std::unordered_map<FNV::HashT, TVector<AssetCacheEntry>::iterator>;

    static const bool BINARY = true;

    bool BuildFromContent(const TVector<std::pair<String, String>>& pairs)
    {
        for (const auto& pair : pairs)
        {
            if (GetAsset(pair.first) != mEntries.end())
            {
                return false; // no duplicates!
            }
        }

        mEntries.resize(mEntries.size() + pairs.size());
        for (const auto& pair : pairs)
        {
            const String& name = pair.first;
            const String& super = pair.second;

            mEntries.push_back(AssetCacheEntry());
            AssetCacheEntry& entry = mEntries.back();
            entry.mCacheName = name;
            entry.mSuperName = super;
            entry.mCacheLocation = INVALID64;
            entry.mCacheMagicFooter = 0;
            entry.mCacheMagicHeader = 0;
            entry.mSizeRaw = 0;
            entry.mSizeSource = 0;

            entry.mCacheHash = FNV::Hash(name.CStr(), name.Size());
            entry.mSuperHash = 0;
        }

        mHashIndex.clear();
        for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
        {
            Assert(mHashIndex.find(it->mCacheHash) == mHashIndex.end()); // duplicate hash! 
            mHashIndex.emplace(it->mCacheHash, it);
        }
        return true;
    }

    void Serialize(Stream& s)
    {
        if (s.BeginObject("AssetCacheRegistry", "Native"))
        {
            SERIALIZE_STRUCT_ARRAY(s, mEntries, "");
            s.EndObject();
        }
    }

    bool OpenFiles(const String& filename)
    {
        if (mFile.IsOpen())
        {
            mFile.Close();
        }

        const String fullname = BINARY ? (filename + ".bin") : (filename + ".json");
        if (!mFile.Open(fullname, FF_READ | FF_WRITE | FF_SHARE_READ | FF_SHARE_WRITE, FILE_OPEN_ALWAYS))
        {
            return false;
        }
       
        const SizeT fileSize = mFile.Size();
        if(BINARY)
        {
            if (fileSize > 0)
            {
                TVector<ByteT> bytes;
                bytes.resize(mFile.Size());
                Assert(mFile.Read(bytes.data(), bytes.size()) == bytes.size());

                MemoryBuffer buffer(bytes.data(), bytes.size(), MemoryBuffer::STATIC);
                buffer.SetSize(buffer.GetCapacity());

                BinaryStream s;
                s.Open(Stream::MEMORY, &buffer, Stream::SM_READ);
                Serialize(s);
                s.Close();
            }
        }
        else
        {
            if (fileSize > 0)
            {
                String json;
                json.Resize(fileSize);
                Assert(mFile.Read(const_cast<char*>(json.CStr()), json.Size()) == json.Size());

                JsonStream s;
                s.Open(Stream::TEXT, &json, Stream::SM_READ);
                Serialize(s);
                s.Close();
            }
        }
        

        mHashIndex.clear();
        for (auto it = mEntries.begin(); it != mEntries.end(); ++it)
        {
            mHashIndex.emplace(it->mCacheHash, it);
        }

        return true;
    }

    void CloseFiles()
    {
        if (mFile.IsOpen())
        {
            mFile.SetCursor(0, FILE_CURSOR_BEGIN);

            if (BINARY)
            {
                MemoryBuffer buffer;
                BinaryStream s;
                s.Open(Stream::MEMORY, &buffer, Stream::SM_WRITE);
                Serialize(s);
                s.Close();

                if (buffer.GetSize() > 0)
                {
                    Assert(mFile.Write(buffer.GetData(), buffer.GetSize()) == buffer.GetSize());
                }
            }
            else
            {
                String json;
                JsonStream s;
                s.Open(Stream::TEXT, &json, Stream::SM_WRITE);
                Serialize(s);
                s.Close();

                if (json.Size() > 0)
                {
                    Assert(mFile.Write(json.CStr(), json.Size()) == json.Size());
                }
            }
        }
    }

    ArrayType::iterator GetAsset(const String& name)
    {
        if (name.Empty())
        {
            return mEntries.end();
        }
        
        auto it = mHashIndex.find(FNV::Hash(name.CStr(), name.Size()));
        if (it == mHashIndex.end())
        {
            return mEntries.end();
        }
        return it->second;
    }
    
    

    ArrayType mEntries;
    HashMap mHashIndex;
    File    mFile;
};

void BuildDB(const TVector<String>& content)
{

// IndexTable:
// | ID | CacheHash | CacheName (Unique) | RuntimeID | MetaID
//
// RuntimeTable:
// | ID | SuperID | SuperHash (Verifies user didnt delete asset) | CacheLocation ( BlobID | ObjectID ) | CacheMagicHeader | CacheMagicFooter
//
// MetaTable:
// | ID | Weak References (Int32) | Strong References (Int32) | Size (Raw), Size (Cache), Size (Source)
//
// StrongReferenceTable:
// | ID | IndexID (Who we are) | ReferenceID (Who we reference)
// 
// WeakReferenceTable:
// | ID | IndexID (Who we are) | ReferenceID (Who we reference)

    String cacheDir = TestFramework::GetConfig().mEngineConfig->GetCacheDirectory();

    AssetCacheDB db;
    Timer t;

    t.Start();
    TEST(db.Initialize());
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to initialize db.");
    
    t.Start();
    TEST(db.OpenFiles(cacheDir + "AssetCacheDB"));
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to open files for db.");

    TVector<std::pair<String, String>> pairs;
    for (const String& asset : content)
    {
        if (Valid(db.GetAsset(asset)))
        {
            continue;
        }
        pairs.push_back(std::make_pair(asset, String()));
    }

    t.Start();
    TEST(db.CreateAssets(pairs));
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to build db.");
  
    TMap<AssetCacheDB::DatabaseID, SimpleAssetTypeInfo> assets;

    t.Start();
    // Under the assumption we had the database info.. how can we build our asset type and how long will it take?
    for (const String& asset : content)
    {
        SimpleAssetTypeInfo type;
        // This will effectively be a no-op
        type.mID = db.GetAsset(asset);
        TEST_CRITICAL(Valid(type.mID));

        AssetCacheDB::IndexTableEntry assetInfo;
        TEST_CRITICAL(db.GetAssetInfo(type.mID, assetInfo));
        type.mRuntimeID = assetInfo.mRuntimeID;
        type.mMetaID = assetInfo.mMetaID;
        type.mAssetName = assetInfo.mCachePath.CStr();
        TEST_CRITICAL(Valid(type.mRuntimeID));
        TEST_CRITICAL(Valid(type.mMetaID));

        AssetCacheDB::RuntimeTableEntry runtimeInfo;
        TEST_CRITICAL(db.GetRuntimeInfo(type.mRuntimeID, runtimeInfo));

        type.mCacheLocation = runtimeInfo.mCacheLocation;
        type.mSuperID = runtimeInfo.mSuperID;
        type.mSuperHash = runtimeInfo.mSuperHash;

        assets[type.mID] = type;
    }

    for (auto& pair : assets)
    {
        auto it = assets.find(pair.second.mSuperID);
        if (it != assets.end())
        {
            pair.second.mSuperName = it->second.mAssetName;
        }
    }
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to initialize domain.");

    t.Start();
    db.CloseFiles();
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to close files.");    

    db.LogStats(gTestLog);
}

void BuildRegistryDB(const TVector<String>& content)
{
    AssetCacheRegistry db;
    Timer t;
    String cacheDir = TestFramework::GetConfig().mEngineConfig->GetCacheDirectory();

    t.Start();
    TEST_CRITICAL(db.OpenFiles(cacheDir + "AssetCacheRegistry"));
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to open registry files.");

    TVector<std::pair<String, String>> pairs;
    for (const String& asset : content)
    {
        if (db.GetAsset(asset) != db.mEntries.end())
        {
            continue;
        }
        pairs.push_back(std::make_pair(asset, String()));
    }

    t.Start();
    TEST(db.BuildFromContent(pairs));
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to build registry content.");


    t.Start();
    for (const String& asset : content)
    {
        SimpleAssetTypeInfo type;
        // This will effectively be a no-op
        auto it = db.GetAsset(asset);
        TEST_CRITICAL(it != db.mEntries.end());

        type.mAssetName = it->mCacheName;

        type.mCacheLocation = it->mCacheLocation;
        // type.mSuperID = runtimeInfo.mSuperID;
        type.mSuperHash = it->mSuperHash;
    }
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to build registry domain.");

    t.Start();
    db.CloseFiles();
    t.Stop();
    gTestLog.Info(LogMessage("Took ") << t.GetDelta() << " seconds to save registry domain.");

}

REGISTER_TEST(InitializeDomain, "Runtime.Asset")
{
    // 1. Asset names have to be case-insensitive and ascii only.
    const bool loadData = true;
    const bool verboseLoad = true;
    if (loadData)
    {
        TVector<String> assetNames;
        TVector<String> assetScopes;
        TVector<String> domains;

        GenerateItem("arrow", assetNames, ItemSoundSet::Weapon);
        GenerateItem("bolt", assetNames, ItemSoundSet::Weapon);
        GenerateItem("gun", assetNames, ItemSoundSet::Weapon);
        GenerateItemOverride("machine_gun", assetNames, ItemSoundSet::Weapon);
        GenerateItemOverride("rifle", assetNames, ItemSoundSet::Weapon);
        GenerateItemOverride("pistol", assetNames, ItemSoundSet::Weapon);
        GenerateItem("sword", assetNames, ItemSoundSet::Weapon);
        GenerateItem("great_sword", assetNames, ItemSoundSet::Weapon);
        GenerateItem("halberd", assetNames, ItemSoundSet::Weapon);
        GenerateItem("spear", assetNames, ItemSoundSet::Weapon);
        GenerateItem("axe", assetNames, ItemSoundSet::Weapon);
        GenerateItem("great_axe", assetNames, ItemSoundSet::Weapon);
        GenerateItem("mace", assetNames, ItemSoundSet::Weapon);
        GenerateItem("great_mace", assetNames, ItemSoundSet::Weapon);
        GenerateItem("flail", assetNames, ItemSoundSet::Weapon);
        assetNames.push_back("texture_set_low.png");
        assetNames.push_back("texture_set_low.json");
        assetNames.push_back("texture_set_medium.png");
        assetNames.push_back("texture_set_medium.json");
        assetNames.push_back("texture_set_high.png");
        assetNames.push_back("texture_set_high.json");
        assetNames.push_back("lang_us.json");
        assetNames.push_back("lang_fr.json");
        assetNames.push_back("lang_it.json");
        assetNames.push_back("lang_ru.json");
        assetNames.push_back("lang_es.json");
        assetNames.push_back("lang_tw.json");
        assetNames.push_back("loot_table.json");

        GenerateItem("chair", assetNames, ItemSoundSet::Interact);
        GenerateItem("bench", assetNames, ItemSoundSet::Interact);
        GenerateItem("desk", assetNames, ItemSoundSet::Interact);
        GenerateItem("table", assetNames, ItemSoundSet::Interact);

        GenerateItem("iron_ore", assetNames);
        GenerateItem("copper_ore", assetNames);
        GenerateItem("tin_ore", assetNames);
        GenerateItem("cobalt_ore", assetNames);
        GenerateItem("coal_ore", assetNames);
        GenerateItem("lead_ore", assetNames);
        GenerateItem("nickel_ore", assetNames);
        GenerateItem("platinum_ore", assetNames);
        GenerateItem("aluminum_ore", assetNames);

        GenerateItem("iron_bar", assetNames);
        GenerateItem("copper_bar", assetNames);
        GenerateItem("tin_bar", assetNames);
        GenerateItem("bronze_bar", assetNames);
        GenerateItem("steel_bar", assetNames);

        GenerateSpell("fireball", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("frostbolt", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("arcane_explosion", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("hammer_of_justice", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("hummer_of_wrath", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("vengeful_throw", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("throw_dagger", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("charging_blast", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("echoing_nightmares", assetNames, ItemSoundSet::Ambient);
        GenerateSpell("retreating_dissident", assetNames, ItemSoundSet::Ambient);

        GenerateNpc("skeleton", assetNames);
        GenerateNpc("dog", assetNames);
        GenerateNpc("behemoth", assetNames);
        GenerateNpc("zombie", assetNames);
        GenerateNpc("man", assetNames);
        GenerateNpc("barbarian", assetNames);
        GenerateNpc("assassin", assetNames);
        GenerateNpc("troll", assetNames);
        GenerateNpc("guard0", assetNames);
        GenerateNpc("guard1", assetNames);
        GenerateNpc("hero", assetNames);
        GenerateNpc("banker", assetNames);
        GenerateNpc("farmer", assetNames);
        GenerateNpc("swordsman", assetNames);
        GenerateNpc("samurai", assetNames);
        GenerateNpc("king", assetNames);
        GenerateNpc("jarl", assetNames);
        GenerateNpc("chief", assetNames);
        GenerateNpc("sorcerer", assetNames);
        GenerateNpc("priest", assetNames);

        assetScopes.push_back("biomes/forest/");
        if (verboseLoad)
        {
            assetScopes.push_back("biomes/desert/");
            assetScopes.push_back("biomes/ocean/");
            assetScopes.push_back("biomes/fairy_forest/");
            assetScopes.push_back("biomes/plains/");
            assetScopes.push_back("biomes/savanaah/");
            assetScopes.push_back("biomes/undeath/");
            assetScopes.push_back("biomes/ruins/");
            assetScopes.push_back("biomes/dungeon/");
            assetScopes.push_back("biomes/space/");
            assetScopes.push_back("biomes/planet_zero/");
            assetScopes.push_back("biomes/alps/");
            assetScopes.push_back("biomes/tropics/");
            assetScopes.push_back("biomes/islands/");
            assetScopes.push_back("biomes/overgrown/");
            assetScopes.push_back("biomes/grove/");
            assetScopes.push_back("biomes/valley/");
            assetScopes.push_back("biomes/peaks/");
            assetScopes.push_back("biomes/bayou/");
            assetScopes.push_back("biomes/boreal/");
        }


        domains.push_back("engine//");
        if (verboseLoad)
        {
            domains.push_back("artherion//");
            domains.push_back("better_weapons//");
            domains.push_back("better_skills//");
            domains.push_back("better_blocks//");
            domains.push_back("better_textures//");
            domains.push_back("sky_world//");
            domains.push_back("dungeons_plus//");
            domains.push_back("dungeons_extreme//");
            domains.push_back("better_boss_fights//");
            domains.push_back("legendary_boss_fights//");
            domains.push_back("dooms_boss_modes//");
            domains.push_back("better_sounds//");
            domains.push_back("more_weapons//");
            domains.push_back("more_armors//");
            domains.push_back("more_spells//");
            domains.push_back("more_skills//");
            domains.push_back("adventure_realm_plus//");
            domains.push_back("technical_innovation//");
            domains.push_back("better_fps//");
            domains.push_back("more_bosses//");
            domains.push_back("firecamp//");
            domains.push_back("factions//");
            domains.push_back("better_factions//");
            domains.push_back("better_crafting//");
            domains.push_back("better_army//");
            domains.push_back("disco//");
            domains.push_back("dragons//");
            domains.push_back("war//");
            domains.push_back("mage_quest//");
            domains.push_back("more_quests//");
            domains.push_back("better_quests//");
            domains.push_back("pikes//");
        }

        TVector<String> paths;
        for (const String& domain : domains)
        {
            for (const String& scope : assetScopes)
            {
                for (const String& name : assetNames)
                {
                    paths.push_back(domain + scope + name);
                }
            }
        }
        BuildDB(paths);
        // BuildRegistryDB(paths);
    }
    else
    {
        BuildDB(TVector<String>());
        BuildRegistryDB(TVector<String>());
    }


    //
    // AssetCallbacks:
    // 
    //
    // Now that we have our paths...We just need to generate the rest of the data...
    // 
    // BaseCheckpoint:
    // Checkpoint:
    // Journal:
    // 
    //
    
    // 
    // DB FileFormat:
    //
    // { L } { F } { D } { B } | 
    // = MAGIC = 4 bytes + json desc
    // = JSON = includes binary desc
    // = BINARY =
    // 
    // TABLE_INFO: { Name, EntrySize, EntryCapacity, EntryAlignment }
    // INDEX_INFO: { Table, Name, VariantType, Offset }
    // MISC: { Compressed, UseAuxFile (This just means, use files named by table to commit temporary writes to), AuxFilePath, Unique }
    // 
    // PENDING CACHE OPERATIONS:
    // WRITE |ID| TO |BLOB_ID & OBJECT_ID|

    // Checkpoint = Apply(BaseCheckpoint, Checkpoint, Journal)
    // Checkpoint.Find(foo)

    // Operations:
    //
    //  During startup we'll need to read the RuntimeTable w/ JOIN on IndexTable to get the following data.
    //      CacheName, SuperName, CacheLocation
    //  After we initialize our data we'll need to create the links and assign the proper concrete type.
    //
    // GetRuntimeID( Type ) => DatabaseID : We'll run this to translate from Type to Runtime info (Simply find on CacheHash then confirm with CacheName compare)
    // GetMetaID( Type ) => DatabaseID : 
    // GetStrongIDs( Type ) => DatabaseID[] :
    // GetWeakIDs( Type ) => DatabaseID[] :
    //
    // FindAsset( Type ) => AssetTypeInfo : This is a runtime operation, database is unaffected
    // QueryAssetMeta( Type ) => AssetTypeMetaInfo :
    // UpdateAssetMeta( Type ) :
    // QueryStrongReferences( Type ) => AssetTypeInfo[] : We'll use this for asset loading
    // QueryWeakReferences( Type ) => AssetTypeInfo[] : We'll use this for asset loading
    // UpdateStrongReferences( Type ) => AssetTypeInfo[] : Updates only in editor
    // UpdateWeakReferences( Type ) => AssetTypeInfo[] : Updates only in editor
    // UpdateCacheLocation ( Type, Location ) : Could happen in game if downloading new content and we place the content in a different spot.
    //
    // CreateAsset/ImportAsset( Name, SuperName ) : Creating an asset will not put it in the cache
    // CacheContent( Type ) : Writes to CacheBlob, Updates CacheLocation
    // DeleteAsset( Name, SuperName ) : {
    //      In order to delete an asset properly, you must delete all references to it. 
    //          CRITICAL: CacheDB ( they reference ids )
    //          Warning:  Asset files (they reference text, so they'll be able to determine if it fails to load)
    // 
    //      1. Update StrongReferenceTable where ReferenceID = this (Update MetaTables too)
    //      2. Update WeakReferenceTable where ReferenceID = this (Update MetaTables too)
    //      3. Collapse parent so types that have SuperID = this, now have SuperID = this.SuperID
    //      4. Delete the cache object
    //      5. Erase from all tables
    // LoadAsset 
    // UpdateAsset
    //
    //
    // Loading and Unloading Domains:
    //
    // CppMod : ScriptMod
    // JsMod : ScriptMod
    // LuaMod : ScriptMod
    //
    // ScriptMod: 
    //      virtual OnLoadDomain();
    //      virtual OnUnloadDomain();
    //      
    //  
    // Game: 
    //      [Scene] Splash
    //      [Scene] Main Menu
    //      [Scene] Demo World

    // CONST DATA: Object & LESS 4KB
    // SMALL DATA: 8KB or LESS
    // SMALL+ DATA: 16KB
    // SMALL++ : 32 KB
    // SMALL+++ : 64 KB
    // MEDIUM : 512KB
    // MEDIUM++: 1024KB
    // LARGE 4MB
    // LARGE+ 8MB
    // LARGE++ 16MB
    // EPIC : 1GB
    // EPIC_TEXTURES:
    // EPIC_SOUND:
    // 
}

// How can we fix up caches programmatically..
//
// | typemap | cache | source |
// 
// If we have source but no cache/typemap we can import
// 
// If we have typemap but no source we might be able to recover from cache but should delete it
//
// AssetMgr::QueryMissingSource() => { InCache=? }
// AssetMgr::RecoverFromCache() => { }
// AssetMgr::CleanCache(); // Zero out unused cache memory in a block
// 

// todo: test we can't create 2 of the same type.

} // namespace lf 



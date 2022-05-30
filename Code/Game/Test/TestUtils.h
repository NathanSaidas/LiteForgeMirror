#pragma once
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
#include "Core/IO/Stream.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/World/ComponentSystem.h"

namespace lf {

class ServiceContainer;

struct DummyInnerStruct
{
    void Serialize(Stream& s)
    {
        SERIALIZE(s, mSimpleValue, "");
    }
    bool operator==(const DummyInnerStruct& other) const
    {
        return mSimpleValue == other.mSimpleValue;
    }
    bool operator!=(const DummyInnerStruct& other) const
    {
        return mSimpleValue != other.mSimpleValue;
    }
    Int32 mSimpleValue;
};
LF_INLINE Stream& operator<<(Stream& s, DummyInnerStruct& self)
{
    self.Serialize(s);
    return s;
}


struct DummyStruct
{
    void Serialize(Stream& s)
    {
        SERIALIZE_STRUCT(s, mStruct, "");
        SERIALIZE_STRUCT_ARRAY(s, mStructArray, "");
        SERIALIZE_ARRAY(s, mValueArray, "");
        SERIALIZE(s, mValue, "");
    }

    bool operator==(const DummyStruct& other) const
    {
        return mStruct == other.mStruct && mStructArray == other.mStructArray && mValueArray == other.mValueArray && mValue == other.mValue;
    }

    DummyInnerStruct            mStruct;
    TVector<DummyInnerStruct>    mStructArray;
    TVector<Int32>               mValueArray;
    Int32                       mValue;
};
LF_INLINE Stream& operator<<(Stream& s, DummyStruct& self)
{
    self.Serialize(s);
    return s;
}

struct TestAssetMgrProvider
{
    static AssetMgr* sInstance;
    static AssetMgr& GetManager();
};
template<typename T>
using TestAsset = TAsset<T, TestAssetMgrProvider>;
template<typename T>
using TestAssetType = TAssetType<T, TestAssetMgrProvider>;

class TestData : public AssetObject
{
    DECLARE_CLASS(TestData, AssetObject);
public:
    TestData();
    explicit TestData(Int32 value);

    void Serialize(Stream& s) override;

    Int32 mValue;
};
using TestDataAsset = TestAsset<TestData>;
using TestDataAssetType = TestAssetType<TestData>;
DECLARE_ATOMIC_PTR(TestData);


class TestDataOwner : public AssetObject
{
    DECLARE_CLASS(TestDataOwner, AssetObject);
public:
    TestDataOwner();
    explicit TestDataOwner(const TestDataAsset& obj, const TestDataAssetType& type);

    void Serialize(Stream& s) override;

    TestDataAsset mReferencedObject;
    TestDataAssetType mReferencedType;
};
using TestDataOwnerAsset = TestAsset<TestDataOwner>;
using TestDataOwnerAssetType = TestAssetType<TestDataOwner>;
DECLARE_ATOMIC_PTR(TestDataOwner);

namespace TestUtils
{
    bool CreateDataAsset(AssetMgr& mgr, const char* path, const TestData& value);
    bool CreateDataOwnerAssetType(AssetMgr& mgr, const char* path, const TestDataOwner& value);

    bool DeleteAsset(AssetMgr& mgr, const AssetTypeInfo* asset);
    bool Flush(AssetMgr& mgr, const char* domain = "engine");

    void RegisterDefaultServices(ServiceContainer& container);
}


template<typename T>
class TSystemTestAttributes
{
public:
    static bool sEnable;
    static ECSUtil::UpdateType sUpdateType;
};

template<typename T>
bool TSystemTestAttributes<T>::sEnable = false;

template<typename T>
ECSUtil::UpdateType TSystemTestAttributes<T>::sUpdateType = ECSUtil::UpdateType::SERIAL;


class TestDynamicStreamDataA : public Object
{
    DECLARE_CLASS(TestDynamicStreamDataA, Object);
public:
    void Serialize(Stream& s);

    String mValueString;
    Int32  mValueInt;
    UInt32 mValueUInt;
};
DECLARE_PTR(TestDynamicStreamDataA);

class TestDynamicStreamDataB : public Object
{
    DECLARE_CLASS(TestDynamicStreamDataB, Object);
public:
    void Serialize(Stream& s);

    Int32 mValueString; // fake
    Int32 mValueInt;
};
DECLARE_PTR(TestDynamicStreamDataB);

class TestDynamicStreamDataC : public Object
{
    DECLARE_CLASS(TestDynamicStreamDataC, Object);
public:
    void Serialize(Stream& s);

    Int32 mFoo;
    String mBar;
};
DECLARE_PTR(TestDynamicStreamDataC);

struct TestDynamicStreamDataInfo
{
    void Serialize(Stream& s);
    
    const Type* mType;
    ObjectPtr  mObject;
};
LF_INLINE Stream& operator<<(Stream& s, TestDynamicStreamDataInfo& o)
{
    o.Serialize(s);
    return s;
}

class TestDynamicStreamDataType
{
public:
    void Serialize(Stream& s);
    void Add(const ObjectWPtr& object)
    {
        TestDynamicStreamDataInfo info;
        info.mType = object->GetType();
        info.mObject = object;
        mObjects.push_back(info);
    }

    TVector<TestDynamicStreamDataInfo> mObjects;
};


} // namespace lf
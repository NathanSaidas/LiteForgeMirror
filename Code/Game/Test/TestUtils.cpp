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
#include "TestUtils.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "AbstractEngine/App/AppService.h"

namespace lf {

DEFINE_CLASS(lf::TestData) { NO_REFLECTION; }
DEFINE_CLASS(lf::TestDataOwner) { NO_REFLECTION; }
DEFINE_CLASS(lf::TestDynamicStreamDataA) { NO_REFLECTION; }
DEFINE_CLASS(lf::TestDynamicStreamDataB) { NO_REFLECTION; }
DEFINE_CLASS(lf::TestDynamicStreamDataC) { NO_REFLECTION; }


AssetMgr* TestAssetMgrProvider::sInstance = nullptr;
AssetMgr& TestAssetMgrProvider::GetManager() { CriticalAssert(sInstance); return *sInstance; }

TestData::TestData()
: Super()
, mValue(0)
{}
TestData::TestData(Int32 value)
: Super()
, mValue(value)
{}
void TestData::Serialize(Stream& s)
{
    SERIALIZE(s, mValue, "");
}

TestDataOwner::TestDataOwner()
: Super()
{}
TestDataOwner::TestDataOwner(const TestDataAsset& obj, const TestDataAssetType& type)
: Super()
, mReferencedObject(obj)
, mReferencedType(type)
{}
void TestDataOwner::Serialize(Stream& s)
{
    SERIALIZE(s, mReferencedObject, "");
    SERIALIZE(s, mReferencedType, "");
}

bool TestUtils::CreateDataAsset(AssetMgr& mgr, const char* path, const TestData& value)
{
    TestDataAtomicPtr asset = MakeConvertibleAtomicPtr<TestData>();
    asset->SetType(typeof(TestData));
    asset->mValue = value.mValue;

    return mgr.Wait(mgr.Create(AssetPath(path), asset, nullptr));
}
bool TestUtils::CreateDataOwnerAssetType(AssetMgr& mgr, const char* path, const TestDataOwner& value)
{
    TestDataOwnerAtomicPtr asset = MakeConvertibleAtomicPtr<TestDataOwner>();
    asset->SetType(typeof(TestDataOwner));
    asset->mReferencedObject = value.mReferencedObject;
    asset->mReferencedType = value.mReferencedType;

    return mgr.Wait(mgr.Create(AssetPath(path), asset, nullptr));
}

bool TestUtils::DeleteAsset(AssetMgr& mgr, const AssetTypeInfo* asset)
{
    return mgr.Wait(mgr.Delete(asset));
}

bool TestUtils::Flush(AssetMgr& mgr, const char* domain)
{
    return mgr.Wait(mgr.SaveDomain(domain)) && mgr.Wait(mgr.SaveDomainCache(domain));
}

void TestUtils::RegisterDefaultServices(ServiceContainer& container)
{
    (container);
    // TODO: Include this somehow.. maybe inherit 'parent' service container?
    // TStrongPointer<AppService> service(LFNew<AppService>());
    // service->SetType(typeof(AppService));
    // container.Register(service);
}



void TestDynamicStreamDataA::Serialize(Stream& s)
{
    Super::Serialize(s);
    SERIALIZE(s, mValueString, "");
    SERIALIZE(s, mValueInt, "");
    SERIALIZE(s, mValueUInt, "");
}

void TestDynamicStreamDataB::Serialize(Stream& s)
{
    Super::Serialize(s);
    SERIALIZE(s, mValueString, "");
    SERIALIZE(s, mValueInt, "");
}

void TestDynamicStreamDataC::Serialize(Stream& s)
{
    Super::Serialize(s);
    SERIALIZE(s, mFoo, "");
    SERIALIZE(s, mBar, "");
}

void TestDynamicStreamDataInfo::Serialize(Stream& s)
{
    SERIALIZE(s, mType, "");
    if (s.IsReading())
    {
        mObject = mType ? GetReflectionMgr().CreateObject(mType) : NULL_PTR;
        if (mObject)
        {
            if ((s << StreamPropertyInfo("mData")).BeginStruct())
            {
                mObject->Serialize(s);
                s.EndStruct();
            }
        }
    }
    else
    {
        if (mObject)
        {
            if ((s << StreamPropertyInfo("mData")).BeginStruct())
            {
                mObject->Serialize(s);
                s.EndStruct();
            }
        }
        else
        {
            if ((s << StreamPropertyInfo("mData")).BeginStruct())
            {
                s.EndStruct();
            }
        }
    }
}

void TestDynamicStreamDataType::Serialize(Stream& s)
{
    SERIALIZE_STRUCT_ARRAY(s, mObjects, "");
}

}
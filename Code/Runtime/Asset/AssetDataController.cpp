// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#include "AssetDataController.h"
#include "Runtime/Asset/AssetObject.h"
#include "Runtime/Asset/AssetCommon.h"
#include "Runtime/Reflection/ReflectionMgr.h"

#include <algorithm>

namespace lf {

static void PopulateConcreteTypes(TArray<const Type*>& outConcreteTypes)
{
    ReflectionMgr& reflection = GetReflectionMgr();
    for (TypeIterator it = reflection.GetTypesBegin(), end = reflection.GetTypesEnd(); it != end; ++it)
    {
        const Type* type = &(*it);
        if (type->IsNative() || type->IsAbstract() || type->IsEnum())
        {
            continue;
        }

        if (!type->IsA(typeof(AssetObject)))
        {
            continue;
        }
        outConcreteTypes.Add(type);
    }
}
static void ReserveStaticTable(SizeT size, AssetDataController::StaticTable& table)
{
    table.mSize = size;
    table.mTypes.Reserve(size);
    table.mTypes.Resize(size);
    table.mHandles.Reserve(size);
    table.mHandles.Resize(size);
    table.mInstances.Reserve(size);
    table.mInstances.Resize(size);
}

void AssetDataController::Initialize(const TArray<AssetTypeData>& dataDrivenTypes, const Type* categoryTypes[AssetCategory::MAX_VALUE])
{
    TArray<const Type*> concreteTypes;
    // Get all the concrete AssetObject types
    PopulateConcreteTypes(concreteTypes);
    // Reserve memory for asset types
    ReserveStaticTable(concreteTypes.Size() + dataDrivenTypes.Size(), mStaticTable);

    StaticNameIndex::TraitsType::BuilderType nameBuilder;
    StaticUIDIndex::TraitsType::BuilderType UIDBuilder;

    // Build Concrete AssetTypes
    SizeT index = 0;
    for (const Type* type : concreteTypes)
    {
        Token fullname(AssetUtil::GetConcreteAssetName(type));
        Assert(!fullname.Empty());
        AssetType& assetType = mStaticTable.mTypes[index];
        assetType.mFullName = fullname;
        assetType.mConcreteType = type;
        assetType.mParentType = nullptr;
        assetType.mUID = INVALID32;
        assetType.mVersion = 0;
        assetType.mCacheFileID = INVALID16;
        assetType.mCacheObjectIndex = INVALID16;
        assetType.mAttributes = 0;
        assetType.mFlags.Set(AssetFlags::AF_CONCRETE);
        assetType.mFlags.Set(AssetFlags::AF_ROOT_CONTAINER);
        assetType.mCategory = AssetCategory::INVALID_ENUM;
        for (SizeT i = 0; i < AssetCategory::MAX_VALUE; ++i)
        {
            if (categoryTypes[i] == type)
            {
                assetType.mCategory = static_cast<UInt8>(i);
                break;
            }
        }
        if (InvalidEnum(static_cast<AssetCategory::Value>(assetType.mCategory)))
        {
            assetType.mCategory = AssetCategory::AC_SERIALIZED_OBJECT;
        }
        assetType.mLoadState = 0; // todo:
        assetType.mHash.SetZero();
        nameBuilder.Add(std::make_pair(assetType.mFullName.CStr(), static_cast<UInt32>(index)));
        nameBuilder.Add(std::make_pair(type->GetFullName().CStr(), static_cast<UInt32>(index)));
        // no UID since its always invalid and can change
        ++index;
    }

    // Build DataDriven AssetTypes
    
    for (const AssetTypeData& data : dataDrivenTypes)
    {
        AssetType& assetType = mStaticTable.mTypes[index];
        assetType.mFullName = data.mFullName;
        assetType.mConcreteType = GetReflectionMgr().FindType(data.mConcreteType);
        assetType.mParentType = nullptr;
        assetType.mUID = data.mUID;
        assetType.mVersion = data.mVersion;
        assetType.mCacheFileID = INVALID16; // todo: fetch cache info in future
        assetType.mCacheObjectIndex = INVALID16; // todo fetch cache info in future
        assetType.mAttributes = data.mAttributes;
        assetType.mFlags.SetMask(data.mFlags);
        assetType.mCategory = data.mCategory;
        assetType.mLoadState = 0; // todo:
        assetType.mHash = data.mHash;
        Assert(!assetType.mFlags.Has(AssetFlags::AF_CONCRETE));

        nameBuilder.Add(std::make_pair(assetType.mFullName.CStr(), static_cast<UInt32>(index)));
        UIDBuilder.Add(std::make_pair(assetType.mUID, static_cast<UInt32>(index)));

        ++index;
    }

    // Build Indices
    // todo: We can use promises or a task to build indices concurrently
    std::sort(nameBuilder.begin(), nameBuilder.end());
    std::sort(UIDBuilder.begin(), UIDBuilder.end());
    mStaticTable.mNameIndex.Build(nameBuilder);
    mStaticTable.mUIDIndex.Build(UIDBuilder);

    // After Indices are built we can begin linking all the parent types.
    // todo: We can use promises or a task to link concurrently
    for (const AssetTypeData& data : dataDrivenTypes)
    {
        UInt32 typeIndex = mStaticTable.mUIDIndex.Find(data.mUID);
        if (Invalid(data.mParentUID))
        {
            UInt32 parentIndex = mStaticTable.mNameIndex.Find(data.mConcreteType.CStr());
            Assert(Valid(typeIndex) && Valid(parentIndex));
            mStaticTable.mTypes[typeIndex].mParentType = &mStaticTable.mTypes[parentIndex];
        }
        else
        {
            UInt32 parentIndex = mStaticTable.mUIDIndex.Find(data.mParentUID);
            Assert(Valid(typeIndex) && Valid(parentIndex));
            mStaticTable.mTypes[typeIndex].mParentType = &mStaticTable.mTypes[parentIndex];
        }

        // CacheLookup(UID)
    }


}

const AssetType* AssetDataController::FindByName(const char* assetName) const
{
    UInt32 index = mStaticTable.mNameIndex.Find(assetName);
    if (Valid(index))
    {
        return &mStaticTable.mTypes[index];
    }
    return nullptr;
}
const AssetType* AssetDataController::FindByUID(UInt32 uid) const
{
    UInt32 index = mStaticTable.mUIDIndex.Find(uid);
    if (Valid(index))
    {
        return &mStaticTable.mTypes[index];
    }
    return nullptr;
}

static SizeT GetNameFootprint(const char* const& key) { return StrLen(key); }
static SizeT GetUIDFootprint(const UInt32& ) { return SizeT(0); }
static SizeT GetIndexFootprint(const UInt32& ) { return SizeT(0); }
SizeT AssetDataController::GetStaticFootprint() const
{
    SizeT footprint = 0;
    footprint += mStaticTable.mNameIndex.QueryFootprint(GetNameFootprint, GetIndexFootprint);
    footprint += mStaticTable.mUIDIndex.QueryFootprint(GetUIDFootprint, GetIndexFootprint);
    footprint += mStaticTable.mTypes.Size() * sizeof(AssetType);
    footprint += mStaticTable.mHandles.Size() * sizeof(AssetHandle);
    footprint += mStaticTable.mInstances.Size() * sizeof(WeakAssetInstanceArray);
    return footprint;
}


} // namespace lf
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
#ifndef LF_RUNTIME_ASSET_DATABASE_H
#define LF_RUNTIME_ASSET_DATABASE_H

#include "Core/Common/API.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Utility/StdMap.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/AssetIndex.h"

#include <algorithm>

namespace lf {

DECLARE_WPTR(AssetObject);

class LF_RUNTIME_API AssetDataController
{
public:
    // todo: In the future we might support 'Atomic' asset instance
    using WeakAssetInstanceArray = TArray<AssetObjectWPtr>;

    using StaticNameIndex = AssetUtil::DefaultNameIndex;
    using StaticUIDIndex = AssetUtil::DefaultUIDIndex;
    using StaticTypeInfos = TArray<AssetType>;
    using StaticHandles   = TArray<AssetHandle>;
    using StaticInstances = TArray<WeakAssetInstanceArray>;
    LF_INLINE static bool StaticSort(const char* a, const char* b) { return a < b; }
    struct StaticTable
    {
        StaticNameIndex mNameIndex;
        StaticUIDIndex  mUIDIndex;

        StaticTypeInfos mTypes;
        StaticHandles   mHandles;
        StaticInstances mInstances;
        SizeT           mSize;
    };

    struct DynamicSort
    {
        bool operator()(const char* a, const char* b) const { return a < b; }
    };
    struct DynamicTuple
    {
        AssetType              mType;
        AssetHandle            mHandle;
        WeakAssetInstanceArray mInstances;
    };
    using DynamicTable = TMap<const char*, DynamicTuple, DynamicSort>;
    using DynamicIterator = DynamicTable::iterator;

    // Initialize( types ) 
    void Initialize(const TArray<AssetTypeData>& types, const Type* categoryTypes[AssetCategory::MAX_VALUE]);
    // Shutdown()
    //
    //
    // 
    // CreatePrototype : AssetHandle
    // CreateInstance  : AssetObject
    // FindByName      : QueryResult
    // FindByID        : QueryResult
    // 
    // GetInstances

    const AssetType* FindByName(const char* assetName) const;
    const AssetType* FindByUID(UInt32 uid) const;

    SizeT StaticSize() const { return mStaticTable.mSize; }
    SizeT GetStaticFootprint() const;
private:

    StaticTable  mStaticTable;
    DynamicTable mDynamicTable;
};

} // namespace lf

#endif // LF_RUNTIME_ASSET_DATABASE_H
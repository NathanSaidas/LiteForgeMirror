#pragma once
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
#include "Core/Memory/AtomicSmartPointer.h"
#include "Runtime/Asset/AssetOp.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/AssetTypes.h"
#include "Runtime/Asset/CacheBlockType.h"

namespace lf {

DECLARE_ATOMIC_WPTR(AssetObject);
DECLARE_MANAGED_CPTR(AssetTypeInfo);

// This operation is started on the 'main thread' and is used
// to create an asset such that the asset can be used in other operations.
// 
// Threads: [ Main Thread ]
// Supported Modes: [ Developer Mode, Modder Mode ]
//
// 1. Allocate and initailize the 'AssetTypeInfo' in DataController
// 2. Write the instance data to the Content location
// 3. Write the cache data to the Cache (if caching is enabled)
//
// This operation is expected to be complete after 'Execute' is called.
class LF_RUNTIME_API AssetCreateOp : public AssetOp
{
public:
    using Super = AssetOp;

    AssetCreateOp(const AssetPath& assetPath, const AssetObject* object, const AssetTypeInfoCPtr& parent, const AssetOpDependencyContext& context);
    ~AssetCreateOp() override = default;

    AssetOpThread::Value GetExecutionThread() const override;
protected:
    void OnUpdate() override;
    void OnCancelled() override;
    void OnComplete() override;
    void OnFailure() override;
private:

    enum CreateOpState
    {
        Validate,
        AllocateInitialize,
        WriteContent,
        WriteCache,
        WriteDependencies,
        Done
    };

    CreateOpState           mCreateState;
    AssetPath               mAssetPath;
    AssetTypeInfoCPtr       mParentAsset;
    AssetObjectAtomicWPtr   mObject;

    AssetTypeInfoCPtr       mAssetType;
    CacheBlockType::Value   mCacheBlockType;
};

} // namespace lf
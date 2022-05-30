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
#include "Core/Utility/Array.h"
#include "Runtime/Asset/AssetOp.h"
#include "Runtime/Asset/AssetPath.h"
#include "Runtime/Asset/CacheBlockType.h"

namespace lf {

DECLARE_ATOMIC_PTR(AssetObject);
DECLARE_MANAGED_CPTR(AssetTypeInfo);

// This asset operation is started on the main thread and has the job of 
// creating an AssetType info such that it is usable with other AssetOperations
// 
// Threads: [ Main Thread ]
// Supported Modes: [ Developer Mode, Modder Mode, Game Mode ]
//
// 1. Create Runtime Type
// 2. Write Asset Data to Cache
//
// This operation is not expected to be complete after 'Execute' is called
class LF_RUNTIME_API AssetImportOp : public AssetOp
{
public:
    using Super = AssetOp;

    AssetImportOp(const AssetPath& assetPath, bool allowRawData, const AssetOpDependencyContext& context);
    ~AssetImportOp() override = default;

    AssetOpThread::Value GetExecutionThread() const override;
protected:
    void OnUpdate() override;
    void OnFailure() override;
private:
    enum ImportOpState
    {
        Validate,
        AllocateInitialize,
        WriteCache,
        Done
    };

    const AssetPath& GetCurrentAssetPath() const;

    ImportOpState           mImportState;
    AssetPath               mAssetPath;
    bool                    mAllowRawData;

    AssetObjectAtomicPtr    mCurrentAsset;
    AssetTypeInfoCPtr       mCurrentAssetType;
    
    TVector<AssetPath>       mDependencies;
    TVector<AssetTypeInfoCPtr> mLockedAssets;
};

} // namespace lf
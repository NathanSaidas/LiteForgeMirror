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
#include "Runtime/Asset/AssetOp.h"

namespace lf {

DECLARE_MANAGED_CPTR(AssetTypeInfo);

// This asset operation is started on the main thread and has the job of 
// making it impossible to start other asset operations on target 'AssetTypeInfo'
// 
// Threads: [ Main Thread ]
// Supported Modes: [ Developer Mode, Modder Mode ]
//
// 1. Acquire the 'write' lock of the asset type
// 2. Mark deleted
// 3. Remove from data controller (Find=null)
// 4. Delete Source
// 5. Delete Cache
// 6. Update Cache Map
// 
// 1. Mark the 'AssetTypeInfo' as deleted,
// 2. Remove the 'AssetTypeInfo' from the DataController
// 3. Unload the asset runtime from the AssetPrototype
// 4. Remove the asset data from the cache
// 5. Optionally move asset source to 'tmp' or delete 
//
// This operation is not expected to be complete after 'Execute' is called
//
// Creators of 'Asset Instances' should gracefully cleanup the instances and destroy them.
class LF_RUNTIME_API AssetDeleteOp : public AssetOp
{
public:
    using Super = AssetOp;
    
    AssetDeleteOp(const AssetTypeInfoCPtr& type, const AssetOpDependencyContext& context);
    ~AssetDeleteOp() override = default;

    AssetOpThread::Value GetExecutionThread() const override;
protected:
    void OnUpdate() override;

    void OnCancelled() override;
    void OnFailure() override;
    void OnComplete() override;

private:
    void Unlock();

    enum DeleteOpState
    {
        Validate,
        AcquireWriteLock,
        UpdateDataController,
        UpdateSourceController,
        UpdateCacheController,
        ReleaseWriteLock,
        Done
    };

    DeleteOpState mDeleteState;
    AssetTypeInfoCPtr mType;
    bool mLocked;

};

} // namespace lf
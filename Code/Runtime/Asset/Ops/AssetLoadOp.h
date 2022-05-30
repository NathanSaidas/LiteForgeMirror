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
#include "Runtime/Asset/AssetTypes.h"

namespace lf {

DECLARE_MANAGED_CPTR(AssetTypeInfo);

class LF_RUNTIME_API AssetLoadOp : public AssetOp
{
public:
    using Super = AssetOp;

    AssetLoadOp(const AssetTypeInfoCPtr& assetType, AssetLoadFlags::Value flags, bool loadCache, const AssetOpDependencyContext& context);
    ~AssetLoadOp() override = default;

    AssetOpThread::Value GetExecutionThread() const override;
private:
    void OnUpdate() override;

    void OnCancelled() override;
    void OnFailure() override;
    void OnComplete() override;
    void OnWaitComplete(AssetOp* op) override;
    
private:
    bool ReadCache(MemoryBuffer& buffer);
    bool ReadSource(MemoryBuffer& buffer);
    void Unlock();
    enum LoadState
    {
        Validate,
        AcquireLock,
        CreatePrototype,
        LoadBinary,
        WaitingForDependencies,
        LoadComplete,
        ReleaseLock,
        Done,

        Count
    };

    LoadState mState;
    const AssetTypeInfo* mType;
    AssetLoadFlags::Value mFlags;
    bool mLoadCache;

    bool mLocked;
    AssetHandle* mHandle;

    TVector<AssetTypeInfoCPtr> mDependencies;

    void LogStats();
    LoadState mLatencyState;
    Timer mLatencyTimer;
    Float64 mLatencyTimings[LoadState::Count];

    Float64 mLoadTime;
    Float64 mLoadSizeTime;
    Float64 mLoadDataTime;

};

} // namespace lf
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
#include "Runtime/PCH.h"
#include "AssetOp.h"
#include "Core/Common/Assert.h"
#include "Core/String/SStream.h"
#include "Core/Utility/StackTrace.h"
#include "Runtime/Asset/AssetCommon.h"
#include "Runtime/Asset/Controllers/AssetOpController.h"

#define LF_RUNTIME_CAPTURE_ASSET_OP_STACK

namespace lf {

AssetOp::AssetOp(const AssetOpDependencyContext& context)
: mWaitCount(0)
, mAsyncUpdatePending(0)
, mLock()
, mState(AOS_NONE)
, mFailReason()
, mContext(context)
, mWaitLock()
, mWaitingOps()
, mExecutionTimer()
#if defined(LF_TEST) || defined(LF_DEBUG)
, mDebugStack()
#endif 
{

}

AssetOp::~AssetOp()
{

}

void AssetOp::Start()
{
#if (defined(LF_TEST) || defined(LF_DEBUG)) && defined(LF_RUNTIME_CAPTURE_ASSET_OP_STACK)
    {
        CaptureStackTrace(mDebugStack, 64);
    }
#endif 

    Assert(mState == AOS_NONE);
    GetOpController().Register(this);
    mState = AOS_RUNNING;
    mExecutionTimer.Start();
    GetOpController().Call(AssetOpThread::MAIN_THREAD, [this, self=Pin()](void*) { OnStart(); });
}

void AssetOp::Cancel()
{
    Assert(mState == AOS_WAITING || mState == AOS_RUNNING);
    {
        ScopeLock lock(GetLock());
        mState = AOS_CANCELLED;
        DispatchCompletion();
    }
    OnCancelled();
}

void AssetOp::Update()
{
    ReportBug(mState == AOS_RUNNING);
    if (TimedOut())
    {
        SetFailed("Timed out");
    }
    else if (mState == AOS_RUNNING)
    {
        OnUpdate();
    }
    AtomicStore(&mAsyncUpdatePending, 0);
}

AssetDataController& AssetOp::GetDataController()
{
    CriticalAssert(mContext.mDataController);
    return *mContext.mDataController;
}
AssetCacheController& AssetOp::GetCacheController()
{
    CriticalAssert(mContext.mCacheController);
    return *mContext.mCacheController;
}
AssetSourceController& AssetOp::GetSourceController()
{
    CriticalAssert(mContext.mSourceController);
    return *mContext.mSourceController;
}
AssetOpController& AssetOp::GetOpController()
{
    CriticalAssert(mContext.mOpController);
    return *mContext.mOpController;
}

float AssetOp::GetTimeoutSeconds() const
{
    return 5.0f * 60.0f;
}

AssetOpThread::Value AssetOp::GetExecutionThread() const
{
    return AssetOpThread::WORKER_THREAD;
}

bool AssetOp::QueueAsyncUpdate()
{
    bool updatePending = AtomicLoad(&mAsyncUpdatePending) == 1;
    if (updatePending)
    {
        return false;
    }
    AtomicStore(&mAsyncUpdatePending, 1);
    return true;
}

void AssetOp::WaitFor(AssetOp* op)
{
    CriticalAssert(op != nullptr);
    ReportBug(!op->IsComplete());
    ReportBug(op->IsRunning() || op->IsWaiting());
    if (op->IsComplete())
    {
        OnWaitComplete(op);
        return;
    }

    Assert(mState == AOS_RUNNING || mState == AOS_WAITING);
    {
        ScopeLock lock(GetLock());
        mState = AOS_WAITING;
    }
    
    {
        ScopeLock lock(op->mWaitLock);
        op->mWaitingOps.push_back(GetWeakPointer());
    }
    AtomicIncrement32(&mWaitCount);

    OnWait();
}

void AssetOp::DispatchCompletion()
{
    ScopeLock lock(mWaitLock);
    for (auto& op : mWaitingOps)
    {
        op->Resume();
        op->OnWaitComplete(this);
    }
    mWaitingOps.clear();
}

void AssetOp::Resume()
{
    Atomic32 count = AtomicDecrement32(&mWaitCount);
    if (count == 0)
    {
        ScopeLock lock(GetLock());
        mState = AOS_RUNNING;
    }
;}

void AssetOp::SetComplete()
{
    ReportBug(mState == AOS_RUNNING);
    {
        ScopeLock lock(GetLock());
        mState = AOS_COMPLETE;
        DispatchCompletion();
    }
    OnComplete();
}
void AssetOp::SetFailed(const String& reason)
{
    ReportBug(mState == AOS_RUNNING);
    {
        ScopeLock lock(GetLock());
        mState = AOS_FAILED;
        mFailReason = reason;
        DispatchCompletion();
    }
    OnFailure();
}

void AssetOp::ForceComplete()
{
    mState = AOS_COMPLETE;
}

void AssetOp::OnStart() {}
void AssetOp::OnCancelled() {}
void AssetOp::OnWait() {}
void AssetOp::OnComplete() {}
void AssetOp::OnFailure() {}

void AssetOp::OnWaitComplete(AssetOp* ) {}
void AssetOp::OnUpdate() {}

} // namespace lf
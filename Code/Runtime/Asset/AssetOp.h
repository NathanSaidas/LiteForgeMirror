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

#include "Core/Common/API.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/SpinLock.h"
#include "Core/String/String.h"
#include "Core/Utility/StdVector.h"
#include "Core/Utility/Time.h"
#include "Runtime/Asset/AssetCommon.h"

namespace lf {

class AssetDataController;
class AssetCacheController;
class AssetSourceController;
class AssetOpController;

DECLARE_ATOMIC_PTR(AssetOp);
DECLARE_ATOMIC_WPTR(AssetOp);

// NOTE: The data types are pointers here but their lifetime is guaranteed to exist for the duration of the Op.
struct AssetOpDependencyContext
{
    AssetOpDependencyContext()
        : mDataController(nullptr)
        , mCacheController(nullptr)
        , mSourceController(nullptr)
        , mOpController(nullptr)
    {
    }

    AssetDataController* mDataController;
    AssetCacheController* mCacheController;
    AssetSourceController* mSourceController;
    AssetOpController* mOpController;
};

// 
// AssetOp is a class designed to be derived from that provides client code the ability
// to schedule asynchronous operations within the AssetMgr. 
//
// It's intended for client code to allocate an AssetOp and provide a 'context'
// which will contain all the necessary controllers for an AssetOp to function.
// 
// Only 2 public methods are intended to be called to manipulate the state of the AssetOp
// to begin with, one of those is the 'Start' method and the other is the Cancel method.
// 
// If you don't explicitly start the asset op it will fade away and do nothing.
//
// Usage:
//   auto op = MakeConvertibleAtomicPtr<AssetOp>(context, ...);
//   op->Start();
//
//
// Key Client Code Functions:
//
//   OnUpdate()     
//   SetComplete()
//   SetFailed();
//
// In order to have the AssetOp do something, override the OnUpdate method, but keep in mind depending
// on the return value of GetExecutionThread the asset op will be executed on the main thread or any worker thread.
// 
// In order to avoid timing out an asset op, SetComplete should be called at some point to signal its successful completion.
// 
// If an asset op we're to do something that would be considered a failure, then you should call SetFailed(...) with a reason.
//
// Note: A cancellelation is not necessarily a failure
// 
class LF_RUNTIME_API AssetOp : public TAtomicWeakPointerConvertible<AssetOp>
{
public:
    //                     [AOS_WAITING] <=== IsWaiting()
    //                          ^
    //                          |
    //                          v            +---> [AOS_COMPLETE]    <=== IsSuccess()
    // [ AOS_NONE ] ----> [ AOS_RUNNING ] ---| 
    //                                       +---> [AOS_CANCELLED]   <=== IsCancelled()
    //                    ^^ IsRunning() ^^  |  
    //                                       +---> [AOS_FAILED]      <=== IsFailed()
    //
    //                                       ^^^ IsComplete() == true ^^^
    enum State
    {
        AOS_NONE,

        AOS_RUNNING,
        AOS_WAITING,
        AOS_COMPLETE,
        AOS_CANCELLED,
        AOS_FAILED
    };

    AssetOp(const AssetOpDependencyContext& context);
    virtual ~AssetOp();

    //** Call this method to kick off the operation (register with OpController for updates)
    void Start();
    //** Call this method to cancel the operation.
    void Cancel();
    //** Called by OpController to 'update' the op. (Ill-advised to call this manually)
    void Update(); 

    SpinLock& GetLock() { return mLock; }
    SpinLock& GetLock() const { return mLock; }

    //** Returns true for completed, failed, cancelled operations
    bool IsComplete() const { return mState >= AOS_COMPLETE; }
    //** Returns true for succesfully completed operations
    bool IsSuccess() const { return mState == AOS_COMPLETE; }
    //** Returns true for failed operations
    bool IsFailed() const { return mState == AOS_FAILED; }
    //** Returns true for cancelled operations.
    bool IsCancelled() const { return mState == AOS_CANCELLED; }
    //** Returns true for running operations (not true for waiting operations)
    bool IsRunning() const { return mState == AOS_RUNNING; }
    //** Returns true for waiting operations
    bool IsWaiting() const { return mState == AOS_WAITING; }
    //** Returns the actual state of the asset op.
    State GetState() const { return mState; }

    //** Returns true if the operation should time-out.
    bool TimedOut() const { return mExecutionTimer.PeekDelta() > GetTimeoutSeconds(); }
    //** Returns the number of seconds the operation can run before being 'timed-out' (5min default)
    virtual float GetTimeoutSeconds() const;
    //** Returns the thread the operation will update on.
    virtual AssetOpThread::Value GetExecutionThread() const;
    //** Returns the reason why the operation failed. ( OPT: Can become token. )
    const String& GetFailReason() const { return mFailReason; }
    //** Attempts to queue an async update, if this returns true, you can go ahead and call update on another thread.
    bool QueueAsyncUpdate();
protected:
    //** Should be called by 'this' to wait on another op
    void WaitFor(AssetOp* op);
    void DispatchCompletion();
    void Resume();

    //** Call from an derived AssetOp to 'complete' the operation, completing the operation
    //** will suspend further updates.
    void SetComplete();
    //** Call from a derived AssetOp to 'fail' the operation, failing the operation will
    //** suspend further updates.
    void SetFailed(const String& reason);
    //** Special case for when we want dummy 'completed' ops.
    void ForceComplete();

    //** (MT) Gets called on the main thread to start an asset op.
    virtual void OnStart();
    //** (MT/WT) Gets called on any thread when the op is cancelled.
    virtual void OnCancelled();
    //** (MT/WT) Gets called on any thread when the op is put on wait.
    virtual void OnWait();
    //** (MT/WT) Gets called on any thread when the op is completed (succesfully).
    virtual void OnComplete();
    //** (MT/WT) Gets called on any thread when the op is completed (failed).
    virtual void OnFailure();
    //** (MT/WT) Gets called on any thread when a dependency is completed for any reason.
    virtual void OnWaitComplete(AssetOp* op);
    //** Gets called on 'execution thread' to update the asset op.
    virtual void OnUpdate();

    AssetDataController& GetDataController();
    AssetCacheController& GetCacheController();
    AssetSourceController& GetSourceController();
    AssetOpController& GetOpController();
    AssetOpDependencyContext& GetContext() { return mContext; }
    const AssetOpDependencyContext& GetContext() const { return mContext; }
private:
    TAtomicStrongPointer<AssetOp> Pin() { return GetWeakPointer(); }

    // ** External state of the asset operation. (Controls updates)
    volatile Atomic32        mWaitCount;
    volatile Atomic32        mAsyncUpdatePending;
    mutable SpinLock         mLock;
    State                    mState;
    String                   mFailReason;
    AssetOpDependencyContext mContext;

    SpinLock                  mWaitLock;
    TVector<AssetOpAtomicWPtr> mWaitingOps;

    Timer                     mExecutionTimer;

#if defined(LF_TEST) || defined(LF_DEBUG)
    UnresolvedStackTrace      mDebugStack;
#endif
    
};

} // namespace lf 
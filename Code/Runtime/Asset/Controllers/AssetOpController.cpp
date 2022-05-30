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
#include "AssetOpController.h"
#include "Runtime/Asset/AssetOp.h"
namespace lf {

    class CompletedOp : public AssetOp
    {
    public:
        CompletedOp(const AssetOpDependencyContext& context) : AssetOp(context) {}

        void Complete()
        {
            ForceComplete();
        }

    };


    AssetOpController::AssetOpController()
    : mAsyncCallLock()
    , mScheduler()
    , mMainThreadAsyncCalls()
    , mInitializeOpsLock()
    , mInitializeOps()
    , mAsyncOps(0)
    , mOps()
    {}

    AssetOpController::~AssetOpController()
    {
        if (mScheduler.IsRunning())
        {
            mScheduler.Shutdown();
        }
    }

    void AssetOpController::Register(AssetOp* op)
    {
        CriticalAssert(op != nullptr);
        ScopeLock lock(mInitializeOpsLock);
        mInitializeOps.push_back(op->GetWeakPointer());
    }

    void AssetOpController::Call(AssetOpThread::Value thread, const TCallback<void, void*>& function, void* param)
    {
        switch (thread)
        {
            case AssetOpThread::MAIN_THREAD:
            {
                if (IsMainThread())
                {
                    function.Invoke(param);
                }
                else
                {
                    ScopeLock lock(mAsyncCallLock);
                    mMainThreadAsyncCalls.push_back({ function, param });
                }
            } break;
            case AssetOpThread::WORKER_THREAD:
            {
                mScheduler.RunTask(function, param);
            } break;
            default:
                CriticalAssertMsg("Invalid case for AssetOpThread");
                break;
        }
    }

    void AssetOpController::DispatchAsyncCalls()
    {
        Assert(IsMainThread());
        TVector<AsyncCall> callbacks;
        {
            ScopeLock lock(mAsyncCallLock);
            callbacks.swap(mMainThreadAsyncCalls);
        }

        for (AsyncCall& call : callbacks)
        {
            call.mFunction.Invoke(call.mParam);
        }
    }

    void AssetOpController::Initialize()
    {
        TaskScheduler::OptionsType options;
#if defined(LF_DEBUG) || defined(LF_TEST) 
        options.mWorkerName = "AssetOpController_Worker";
#endif
        mScheduler.Initialize(options, true);

        AssetOpDependencyContext context;
        context.mOpController = this;

        mCompletedOp = MakeConvertibleAtomicPtr<CompletedOp>(context);
        StaticCast<TStrongPointer<CompletedOp>>(mCompletedOp)->Complete();
    }

    void AssetOpController::Shutdown()
    {
        mScheduler.Shutdown();
        // TODO: We should cancel all the current operations.
        for (AssetOp* op : mOps)
        {
            if (op->IsWaiting() || op->IsRunning())
            {
                op->Cancel();
            }
        }
        mOps.clear();

        mCompletedOp.Release();
    }

    void AssetOpController::Update()
    {
        Assert(IsMainThread());
        
        // Wait for async ops to update.
        // 
        // If this becomes a hot spot, we can individually check if a op needs an update to be kicked on
        // a worker thread.
        while (AtomicLoad(&mAsyncOps) != 0) {}

        {
            ScopeLock lock(mInitializeOpsLock);
            mOps.insert(mOps.begin(), mInitializeOps.begin(), mInitializeOps.end());
            mInitializeOps.resize(0);
        }

        DispatchAsyncCalls();

        // Remove the complete'd ops.
        for (auto iter = mOps.begin(); iter != mOps.end();)
        {
            if ((*iter)->IsComplete())
            {
                iter = mOps.swap_erase(iter);
            }
            else
            {
                ++iter;
            }
        }

        // Update all WORKER_THREAD ops.
        for (AssetOp* op : mOps)
        {
            if (
                op->GetExecutionThread() == AssetOpThread::WORKER_THREAD // Check if op desires to be run on worker thread
             && !op->IsWaiting() // Check if the op is waiting, waiting ops don't require updates.
             && op->QueueAsyncUpdate() // Check if the op is already updating asynchronously.
            )
            {
                AtomicIncrement32(&mAsyncOps);
                AssetOpAtomicPtr pinned = GetAtomicPointer(op);
                Call(AssetOpThread::WORKER_THREAD, [this, pinned](void*) 
                { 
                    pinned->Update(); 
                    AtomicDecrement32(&mAsyncOps);
                });
            }
        }

        // Update all MAIN_THREAD ops.
        for (AssetOp* op : mOps)
        {
            if (op->GetExecutionThread() == AssetOpThread::MAIN_THREAD && !op->IsWaiting())
            {
                op->Update();
            }
        }


    }
} // namespace lf
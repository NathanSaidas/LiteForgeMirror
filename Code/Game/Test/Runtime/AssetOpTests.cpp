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
#include "Core/Test/Test.h"
#include "Runtime/Asset/AssetOp.h"
#include "Runtime/Asset/Controllers/AssetOpController.h"

namespace lf {

// Simple-Operation: An operation that has a single step to execute.
// Multi-Step-Operation: An operation that has multiple steps to execute.

// Tests:
// 1. Test that ensures we can start/run/complete a simple operation, and multi-step operation.
// 2. Test that ensures we can start/cancel a simple operation, and multi-step operation.
// 3. Test that ensures we can start/run/cancel a simple operation, and multi-step operation.
// 4. Test that ensures we can start/wait/complete a multi-step operation.
// 5. Test that ensures we can start/wait/cancel a multi-step-operation
//      Op A ==[ depends on ]==> Op B
//      Op B is cancelled
//      Op A failed because Op B is cancelled
// 6. 
// 

// Operation purposefully times out
class TimeoutOp : public AssetOp
{
public:
    TimeoutOp(const AssetOpDependencyContext& context) : AssetOp(context) {}
    virtual float GetTimeoutSeconds() const override { return 2.0f; }
    virtual AssetOpThread::Value GetExecutionThread() const override { return AssetOpThread::MAIN_THREAD; }
};

class WorkOp : public AssetOp
{
public:
    using WaitCallback = TCallback<void, bool>;

    WorkOp(const AssetOpDependencyContext& context, int steps, bool async) : AssetOp(context), mStep(steps), mAsync(async) {}
    virtual AssetOpThread::Value GetExecutionThread() const override { return mAsync ? AssetOpThread::WORKER_THREAD : AssetOpThread::MAIN_THREAD; }
    virtual void OnUpdate() override
    {
        if (mExplicitDependency)
        {
            WaitFor(mExplicitDependency);
            return;
        }

        --mStep;
        if (mStep == 0)
        {
            SetComplete();
        }
    }

    virtual void OnWaitComplete(AssetOp* op) override
    {
        TEST(op == mExplicitDependency);
        if (mExplicitWaitCallback.IsValid())
        {
            mExplicitWaitCallback.Invoke(op->IsSuccess());
        }
        mExplicitDependency = NULL_PTR;
        mExplicitWaitCallback = WaitCallback();
    }

    void WaitOn(AssetOp* op, WaitCallback callback = WaitCallback())
    {
        TEST_CRITICAL(op);
        TEST_CRITICAL(mExplicitDependency == NULL_PTR);
        mExplicitDependency = GetAtomicPointer(op);
        mExplicitWaitCallback = callback;
    }

    int mStep;
    bool mAsync;
    AssetOpAtomicPtr mExplicitDependency;
    WaitCallback     mExplicitWaitCallback;
};

// Operation 
class WaitOp : public AssetOp
{
public:
    WaitOp(const AssetOpDependencyContext& context, bool async) 
        : AssetOp(context)
        , mAsync(async)
        , mState(LS_NONE)
        , mWaitingOps(1) 
    {}
    WaitOp(const AssetOpDependencyContext& context, bool async, Atomic32 waitingOps)
        : AssetOp(context)
        , mAsync(async)
        , mState(LS_NONE)
        , mWaitingOps(waitingOps) 
    {}
    virtual AssetOpThread::Value GetExecutionThread() const override { return mAsync ? AssetOpThread::WORKER_THREAD : AssetOpThread::MAIN_THREAD; }
    
    enum LocalState 
    {
        LS_NONE,
        LS_STARTED,
        LS_COMPLETED
    };

    virtual void OnUpdate() override
    {
        switch (mState)
        {
            case LS_NONE:
            {
                for (Atomic32 i = 0; i < mWaitingOps; ++i)
                {
                    auto op = MakeConvertibleAtomicPtr<WorkOp>(GetContext(), 3, mAsync);
                    op->Start();
                    WaitFor(op);
                }
                mState = LS_STARTED;
            } break;
            case LS_STARTED:
            {
                TEST(false); // We should never get this condition because we go from LS_STARTED => COMPLETED from the WaitComplete callback
            } break;
            case LS_COMPLETED:
            {
                SetComplete();
            } break;
        }
    }

    virtual void OnWaitComplete(AssetOp* ) override
    {
        if (AtomicDecrement32(&mWaitingOps) == 0)
        {
            TEST(IsRunning());
            mState = LS_COMPLETED;
        }
    }

    LocalState mState;
    bool mAsync;
    volatile Atomic32 mWaitingOps;
};

// Demonstrate how an isolated AssetOp can be used
REGISTER_TEST(AssetOp_Usage, "Runtime.AssetOp")
{
    AssetOpController controller;
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    // Create an operation with 3 steps
    // ExampleTestAssetOp op(context, 3);
    auto op = MakeConvertibleAtomicPtr<WorkOp>(context, 3, false);
    TEST(op->GetState() == AssetOp::AOS_NONE);

    // We first call 'Execute' to start the operation.
    op->Start();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    // The operation must be updated until completion.
    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_COMPLETE);
}

REGISTER_TEST(AssetOp_Timeout, "Runtime.AssetOp")
{
    AssetOpController controller;
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    Timer timer;

    auto op = MakeConvertibleAtomicPtr<TimeoutOp>(context);
    TEST(op->GetState() == AssetOp::AOS_NONE);

    op->Start();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    timer.Start();
    while (timer.PeekDelta() < 2.0f)
    {
        controller.Update();
    }

    TEST(op->GetState() == AssetOp::AOS_FAILED);
}

REGISTER_TEST(AssetOp_Wait, "Runtime.AssetOp")
{
    AssetOpController controller;
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    auto op = MakeConvertibleAtomicPtr<WaitOp>(context, false);
    TEST(op->GetState() == AssetOp::AOS_NONE);

    op->Start();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    // First update will update the 'WaitOp' and the
    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_WAITING);

    // The next 3 will complete the 'WorkOp' and then complete the 'WaitOp'
    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_WAITING);

    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_WAITING);

    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_COMPLETE);
}

REGISTER_TEST(AssetOp_WaitAsync, "Runtime.AssetOp")
{
    AssetOpController controller;
    controller.Initialize();
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    auto op = MakeConvertibleAtomicPtr<WaitOp>(context, true);
    TEST(op->GetState() == AssetOp::AOS_NONE);

    op->Start();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    Timer t;
    t.Start();
    while (t.PeekDelta() < op->GetTimeoutSeconds())
    {
        controller.Update();
        if (op->GetState() == AssetOp::AOS_COMPLETE)
        {
            break;
        }
    }

    TEST(op->GetState() == AssetOp::AOS_COMPLETE);
    controller.Shutdown();
}

REGISTER_TEST(AssetOp_MultiWait, "Runtime.AssetOp")
{
    AssetOpController controller;
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    auto op = MakeConvertibleAtomicPtr<WaitOp>(context, false, 10);
    TEST(op->GetState() == AssetOp::AOS_NONE);

    op->Start();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);

    // First update will update the 'WaitOp' and the
    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_WAITING);

    // The next 3 will complete the 'WorkOp' and then complete the 'WaitOp'
    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_WAITING);

    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_WAITING);

    controller.Update();
    TEST(op->GetState() == AssetOp::AOS_COMPLETE);
}

REGISTER_TEST(AssetOp_MultiWaitAsync, "Runtime.AssetOp")
{
    AssetOpController controller;
    controller.Initialize();
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    auto op = MakeConvertibleAtomicPtr<WaitOp>(context, false, 10);
    TEST(op->GetState() == AssetOp::AOS_NONE);

    op->Start();
    TEST(op->GetState() == AssetOp::AOS_RUNNING);


    Timer t;
    t.Start();
    while (t.PeekDelta() < op->GetTimeoutSeconds())
    {
        controller.Update();
        if (op->GetState() == AssetOp::AOS_COMPLETE)
        {
            break;
        }
    }

    TEST(op->GetState() == AssetOp::AOS_COMPLETE);
    controller.Shutdown();

}

REGISTER_TEST(AssetOp_Cancel, "Runtime.AssetOp")
{
    AssetOpController controller;
    controller.Initialize();
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    // Test we can cancel a normal operation.
    {
        auto op = MakeConvertibleAtomicPtr<WorkOp>(context, 5, false);
        TEST(op->GetState() == AssetOp::AOS_NONE);

        op->Start();
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        controller.Update();
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        op->Cancel();
        TEST(op->GetState() == AssetOp::AOS_CANCELLED);

        controller.Update();
    }
    
    // Test we can cancel a dependency and that the parent op will fail.
    {
        auto op = MakeConvertibleAtomicPtr<WorkOp>(context, 5, false);
        auto dependency = MakeConvertibleAtomicPtr<WorkOp>(context, 5, false);
        TEST(op->GetState() == AssetOp::AOS_NONE);
        TEST(dependency->GetState() == AssetOp::AOS_NONE);

        op->Start();
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        dependency->Start();
        TEST(dependency->GetState() == AssetOp::AOS_RUNNING);

        // we call WaitOn but it should only really be waiting after the first update.
        op->WaitOn(dependency, WorkOp::WaitCallback::Make([op](bool value) { if (!value) { op->Cancel(); } }));
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        controller.Update();
        TEST(op->GetState() == AssetOp::AOS_WAITING);
        TEST(dependency->GetState() == AssetOp::AOS_RUNNING);

        dependency->Cancel();
        TEST(dependency->GetState() == AssetOp::AOS_CANCELLED);
        TEST(op->GetState() == AssetOp::AOS_CANCELLED);

        controller.Update();

    }

    // Test we can cancel an op but also continue another to completion.
    {
        auto op = MakeConvertibleAtomicPtr<WorkOp>(context, 3, false);
        auto dependency = MakeConvertibleAtomicPtr<WorkOp>(context, 5, false);
        TEST(op->GetState() == AssetOp::AOS_NONE);
        TEST(dependency->GetState() == AssetOp::AOS_NONE);

        op->Start();
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        dependency->Start();
        TEST(dependency->GetState() == AssetOp::AOS_RUNNING);

        // we call WaitOn but it should only really be waiting after the first update.
        op->WaitOn(dependency);
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        controller.Update();
        TEST(op->GetState() == AssetOp::AOS_WAITING);
        TEST(dependency->GetState() == AssetOp::AOS_RUNNING);

        dependency->Cancel();
        TEST(dependency->GetState() == AssetOp::AOS_CANCELLED);
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        controller.Update();
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        controller.Update();
        TEST(op->GetState() == AssetOp::AOS_RUNNING);

        controller.Update();
        TEST(op->GetState() == AssetOp::AOS_COMPLETE);


    }

    controller.Shutdown();
}

// TODO: Changing an operation to cancel that is currently updating should prevent it from 'uncancelling'

/*

REGISTER_TEST(AssetOp_ControllerUsage, "Runtime.AssetOp")
{
    AssetOpController controller;
    AssetOpDependencyContext context;
    context.mOpController = &controller;
    
    auto op = MakeConvertibleAtomicPtr<ExampleTestAssetOp>(context, 3);

    TEST(op->IsIdle() == true);
    TEST(op->IsRunning() == false);
    TEST(op->IsComplete() == false);
    TEST(op->IsFailed() == false);

    op->Start();
    controller.Update();

    TEST(op->IsIdle() == false);
    TEST(op->IsRunning() == true);
    TEST(op->IsComplete() == false);
    TEST(op->IsFailed() == false);

    controller.Update();
    TEST(op->IsIdle() == false);
    TEST(op->IsRunning() == true);
    TEST(op->IsComplete() == false);
    TEST(op->IsFailed() == false);

    controller.Update();
    TEST(op->IsIdle() == false);
    TEST(op->IsRunning() == false);
    TEST(op->IsComplete() == true);
    TEST(op->IsFailed() == false);
}



REGISTER_TEST(AssetOp_ExternalUpdate, "Runtime.AssetOp")
{
    AssetOpController controller;
    AssetOpDependencyContext context;
    context.mOpController = &controller;

    auto op = MakeConvertibleAtomicPtr<ExampleTestAssetOp>(context, 6);

    op->Start();
    op->Update();
    op->Update();

    controller.Update();
    
}

*/
} // namespace lf
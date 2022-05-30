// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#include "Core/Concurrent/Task.h"
#include "Core/Utility/APIResult.h"
#include "Core/Utility/Error.h"
#include "Runtime/Async/PromiseImpl.h"

namespace lf {

DECLARE_HASHED_CALLBACK(TestPromiseCallback, void);
using TestPromise = PromiseImpl<TestPromiseCallback, TestPromiseCallback>;

// Tests that we can queue tasks on a promise and have them executed
// 'next-frame'
REGISTER_TEST(PromiseTest, "Runtime.Async")
{
    GetAsync().WaitForSync(); // Synchronize with the async drain queue thread, without this we may have drift and fail sometimes.

    PromiseWrapper promise = TestPromise([](Promise* self)
    {
        static_cast<TestPromise*>(self)->Resolve();
    })
    .Queue();


    volatile Atomic32 testValue = 0;
    TEST(!promise->IsDone());
    TEST(promise->IsQueued());
    // SleepCallingThread(95);
    SleepCallingThread(50); // We want to exercise promise registration to
    TEST(promise->IsQueued());

    TestPromise* p = static_cast<TestPromise*>(promise.AsPtr());

    auto lambda = [&testValue]()
    {
        AtomicIncrement32(&testValue);
    };

    p->Then(TestPromiseCallback::Make(lambda));

    TEST(!promise->IsDone());
    TEST(promise->IsQueued());


    promise->LazyWait();
    TEST(promise->IsDone());
    TEST(AtomicLoad(&testValue) == 1);

    // StaticCast<TestPromise*>(promise)->Then([&testValue]() { AtomicIncrement32(&testValue); });
    

}

DECLARE_HASHED_CALLBACK(APIFailed, void, ErrorBase*);
DECLARE_HASHED_CALLBACK(APISuccess, void);
using APIResultPromise = PromiseImpl<APISuccess, APIFailed>;

APIResultPromise GetFailPromise()
{
    return APIResultPromise([](Promise* promise)
        {
            APIResult<bool> failed = ReportError(false, OperationFailureError, "Test operation failed! (Ignore)", "<NONE>");
            static_cast<APIResultPromise*>(promise)->Reject(failed.GetError());
        });
}

APIResultPromise GetSuccessPromise()
{
    return APIResultPromise([](Promise* promise)
        {
            static_cast<APIResultPromise*>(promise)->Resolve();
        });
}

APIResultPromise GetEmptyPromise()
{
    return APIResultPromise();
}

REGISTER_TEST(APIResultPromiseTest, "Runtime.Async")
{
    ThreadFence fence;
    TEST_CRITICAL(fence.Initialize());
    volatile Atomic32 status = 0;

    AtomicStore(&status, 0);
    fence.Set(true);
    {
        GetSuccessPromise()
            .Then(APISuccess::Make([&status, &fence]()
                {
                    AtomicStore(&status, 1);
                    fence.Set(false);
                }))
            .Catch(APIFailed::Make([&status, &fence](ErrorBase* error)
                {
                    if (error)
                    {
                        error->Ignore();
                    }
                    AtomicStore(&status, -1);
                    fence.Set(false);
                }));
                fence.Wait(5000);
                TEST(AtomicLoad(&status) == 1);
    }

    AtomicStore(&status, 0);
    fence.Set(true);
    {
        GetFailPromise()
            .Then(APISuccess::Make([&status, &fence]()
                {
                    AtomicStore(&status, 1);
                    fence.Set(false);
                }))
            .Catch(APIFailed::Make([&status, &fence](ErrorBase* error)
                {
                    if (error)
                    {
                        error->Ignore();
                    }
                    AtomicStore(&status, -1);
                    fence.Set(false);
                }));
                fence.Wait(5000);
                TEST(AtomicLoad(&status) == -1);
    }

    AtomicStore(&status, 0);
    fence.Set(true);
    {
        GetEmptyPromise()
            .Then(APISuccess::Make([&status, &fence]()
                {
                    AtomicStore(&status, 1);
                    fence.Set(false);
                }))
            .Catch(APIFailed::Make([&status, &fence](ErrorBase* error)
                {
                    if (error)
                    {
                        error->Ignore();
                    }
                    AtomicStore(&status, -1);
                    fence.Set(false);
                }));
                fence.Wait(5000);
                TEST(AtomicLoad(&status) == 0);
    }
}

REGISTER_TEST(SimpleTaskTest, "Runtime.Async")
{
    Task<int> task = GetAsync().Run<int>([]() -> int 
        {
            return 5;
        });
    
    TEST(task.IsRunning() || task.IsComplete());
    TEST_CRITICAL(task.Wait());
    TEST(task.ResultValue() == 5);
}


REGISTER_TEST(StructTaskTest, "Runtime.Async")
{
    struct CustomData
    {
        TStrongPointer<int> mData;
        float mFoo;
        String mBar;
    };

    Task<CustomData> task = GetAsync().Run<CustomData>([]()->CustomData
        {
            CustomData d;
            d.mData = TStrongPointer<int>(LFNew<int>());
            *d.mData = 10;
            d.mFoo = 32.0f;
            d.mBar = "Test String";
            return d;
        });

    TEST(task.IsRunning() || task.IsComplete());
    TEST_CRITICAL(task.Wait());
    TEST(task.ResultValue().mData != NULL_PTR);
    TEST(*task.ResultValue().mData == 10);
    TEST(task.ResultValue().mFoo == 32.0f);
    TEST(task.ResultValue().mBar == "Test String");
}

REGISTER_TEST(SynchronousTaskTest, "Runtime.Async")
{
    TaskScheduler scheduler;
    scheduler.Initialize(false);
    TEST_CRITICAL(scheduler.IsRunning());

    Task<int> task(Task<int>::TaskCallback::Make([]() { return 5; }), scheduler);
    TEST(task.IsRunning() && !task.IsComplete());
    scheduler.UpdateSync(1.0);
    TEST(task.IsComplete());
    TEST(task.ResultValue() == 5);
    scheduler.Shutdown();
}

} // namespace lf 
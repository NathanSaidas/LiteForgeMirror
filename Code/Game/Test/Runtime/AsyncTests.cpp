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
#include "Core/Test/Test.h"
#include "Runtime/Async/PromiseImpl.h"

namespace lf {

using TestPromise = PromiseImpl<TCallback<void>, TCallback<void>>;

// Tests that we can queue tasks on a promise and have them executed
// 'next-frame'
REGISTER_TEST(PromiseTest)
{
    GetAsync().WaitForSync(); // Synchronize with the async drain queue thread, without this we may have drift and fail sometimes.

    PromiseWrapper promise = TestPromise([](Promise* self)
    {
        self->Resolve();
    })
    .Queue();


    volatile Atomic32 testValue = 0;
    TEST(!promise->IsDone());
    TEST(promise->IsQueued());
    // SleepCallingThread(95);
    SleepCallingThread(50); // We want to exercise promise registration to
    TEST(promise->IsQueued());

    TestPromise* p = static_cast<TestPromise*>(promise.AsPtr());
    p->Then([&testValue]()
    {
        AtomicIncrement32(&testValue);
    });

    TEST(!promise->IsDone());
    TEST(promise->IsQueued());


    promise->LazyWait();
    TEST(promise->IsDone());
    TEST(AtomicLoad(&testValue) == 1);

    // StaticCast<TestPromise*>(promise)->Then([&testValue]() { AtomicIncrement32(&testValue); });
    

}

} // namespace lf 
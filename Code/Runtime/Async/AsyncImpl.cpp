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
#include "AsyncImpl.h"
#include "Runtime/Async/Promise.h"

namespace lf {

AsyncImpl::AsyncImpl()
: mScheduler()
, mDrainQueueThread()
, mFence()
, mBuffer()
, mWork()
, mLock()
, mIsRunning()
{
}
AsyncImpl::~AsyncImpl()
{

}

void AsyncImpl::Initialize()
{
    AtomicStore(&mIsRunning, 1);
    mWork.Reserve(64);
    mBuffer.Reserve(64);

    Assert(mFence.Initialize());
    mFence.Set(true);

    mScheduler.Initialize(true);
    mDrainQueueThread.Fork([](void* context)
    {
        reinterpret_cast<AsyncImpl*>(context)->DrainQueue();
    }, this);
}
void AsyncImpl::Shutdown()
{
    AtomicStore(&mIsRunning, 0);
    mDrainQueueThread.Join();
    mFence.Destroy();

    // Guarantee we execute our promises.
    for (PromiseWrapper& wrapper : mBuffer)
    {
        RunPromise(wrapper);
    }

    for (PromiseWrapper& wrapper : mWork)
    {
        RunPromise(wrapper);
    }
    mBuffer.Clear();
    mWork.Clear();

    mScheduler.Shutdown();
}

void AsyncImpl::RunPromise(PromiseWrapper promise)
{
    if (!promise->SetState(Promise::PROMISE_PENDING))
    {
        return;
    }
    if (!IsRunning() || !mScheduler.IsRunning() || !mScheduler.IsAsync())
    {
        promise->Run();
        return;
    }
    TaskHandle task;
    {
        auto lamb = [promise](void*) { promise->Run(); };
        TaskCallback callback = TaskCallback::CreateLambda(lamb);
        task = mScheduler.RunTask(callback);
    }
    promise->SetTask(task);
}

void AsyncImpl::QueuePromise(PromiseWrapper promise)
{
    if (!promise->SetState(Promise::PROMISE_QUEUED))
    {
        return;
    }

    ScopeLock lock(mLock);
    mBuffer.Add(promise);
}

TaskHandle AsyncImpl::RunTask(const TaskCallback& callback, void* param)
{
    return mScheduler.RunTask(callback, param);
}

void AsyncImpl::WaitForSync()
{
    mFence.Wait();
}

void AsyncImpl::Signal()
{
    mFence.Signal();
}

bool AsyncImpl::IsRunning() const
{
    return AtomicLoad(&mIsRunning) != 0;
}

void AsyncImpl::DrainQueue()
{
    static const SizeT MAX_FRAME_MS = 100;
    while (IsRunning())
    {
        // Wait
        mFence.Wait(100);
        mFence.Signal();

        // Swap
        {
            ScopeLock lock(mLock);
            mBuffer.swap(mWork);
        }

        // Execute
        for (PromiseWrapper& wrapper : mWork)
        {
            if (wrapper->IsQueued())
            {
                RunPromise(wrapper);
            }
        }
        mWork.Resize(0);
    }
}

}
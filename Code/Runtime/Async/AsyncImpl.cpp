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
#include "Runtime/PCH.h"
#include "AsyncImpl.h"
#include "Runtime/Async/Promise.h"
#include "Runtime/Async/ThreadDispatcher.h"

namespace lf {

AsyncImpl::AsyncImpl()
: mScheduler()
, mDrainQueueThread()
, mFence()
, mBuffer()
, mWork()
, mLock()
, mIsRunning(0)
, mAllowAppThreadExecution(0)
, mAppThreads()
{
}
AsyncImpl::~AsyncImpl()
{

}

void AsyncImpl::Initialize()
{
    AtomicStore(&mIsRunning, 1);
    mWork.reserve(64);
    mBuffer.reserve(64);

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
    Assert(!AppThreadRunning());
    for (AppThreadId threadID = 0; threadID < APP_THREAD_ID_MAX; ++threadID)
    {
        StopThread(threadID);
    }
    for (AppThreadId threadID = 0; threadID < APP_THREAD_ID_MAX; ++threadID)
    {
        if (mAppThreads[threadID] && threadID != APP_THREAD_ID_MAIN)
        {
            mAppThreads[threadID]->mPlatformThread.Join();
            mAppThreads[threadID] = NULL_PTR;
        }
    }

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
    mBuffer.clear();
    mWork.clear();

    mScheduler.Shutdown();
}

void AsyncImpl::EnableAppThread()
{
    Assert(IsMainThread());
    ScopeRWSpinLockWrite lock(mAppThreadLock);
    Assert(AtomicCompareExchange(&mAllowAppThreadExecution, 1, 0) == 0);
}

void AsyncImpl::DisableAppThread()
{
    Assert(IsMainThread());
    ScopeRWSpinLockWrite lock(mAppThreadLock);
    Assert(AtomicCompareExchange(&mAllowAppThreadExecution, 0, 1) == 1);

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
        TaskCallback callback = TaskCallback::Make(lamb);
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
    mBuffer.push_back(promise);
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
        mWork.resize(0);
    }
}

bool AsyncImpl::AppThreadRunning() const
{
    return AtomicLoad(&mAllowAppThreadExecution) == 1;
}

bool AsyncImpl::StartThread(AppThreadId threadID, const AppThreadCallback& callback, const AppThreadAttributes& threadAttributes)
{
    if (!IsMainThread()) // Must be main thread!
    {
        return false;
    }

    ScopeRWSpinLockWrite lock(mAppThreadLock);
    if (AppThreadRunning()) // Cannot create threads while running
    {
        return false;
    }

    if (threadID >= APP_THREAD_ID_MAX)
    {
        return false;
    }

    if (!callback.IsValid() && threadID != APP_THREAD_ID_MAIN)
    {
        return false;
    }

    if (mAppThreads[threadID])
    {
        return false;
    }

    if (Valid(threadAttributes.mWorkerID))
    {
        ReportBugMsg("TODO: Implement worker app thread support.");
    }

    mAppThreads[threadID] = AppThreadContextPtr(LFNew<AppThreadContext>());
    AppThreadContext* context = mAppThreads[threadID];
    context->mAppThreadId = threadID;
    context->mAppThreadCallback = callback;
    context->mAsync = this;
    context->mDispatcher = threadAttributes.mDispatcher;
    context->mPlatformThreadProc = AsyncImpl::PlatformThreadProc;

    if (threadID == APP_THREAD_ID_MAIN)
    {
        SetThreadLocalData(threadID, INVALID_APP_WORKER_THREAD_ID);
        context->SetState(AppThreadContext::USER_EXECUTE);
    }
    else
    {
        context->SetState(AppThreadContext::PLATFORM_INITIALIZE);
        context->mPlatformFence.Set(true); // Block until started.
        context->mPlatformThread.Fork(
            [](void* data)
            {
                AppThreadContext* context = reinterpret_cast<AppThreadContext*>(data);
                context->mPlatformThreadProc(context);
            }, context);
        context->mPlatformFence.Wait();
    }

    return true;
}
bool AsyncImpl::StopThread(AppThreadId threadID)
{
    if (!IsMainThread())
    {
        return false;
    }

    ScopeRWSpinLockWrite lock(mAppThreadLock);
    if (AppThreadRunning()) // Cannot stop threads while running
    {
        return false;
    }

    if (threadID >= APP_THREAD_ID_MAX)
    {
        return false;
    }

    AppThreadContext* context = mAppThreads[threadID];
    if (!context || context->GetState() != AppThreadContext::USER_EXECUTE)
    {
        return false;
    }
    context->SetState(AppThreadContext::USER_SHUTDOWN);
    return true;
}
bool AsyncImpl::ExecuteOn(AppThreadId threadID, const AppThreadDispatchCallback& callback)
{
    ScopeRWSpinLockRead lock(mAppThreadLock);
    if (!AppThreadRunning())
    {
        return false;
    }

    if (threadID >= APP_THREAD_ID_MAX)
    {
        return false;
    }

    if (!callback.IsValid())
    {
        return true;
    }

    if (threadID == GetAppThreadId())
    {
        callback.Invoke();
        return true;
    }

    AppThreadContext* context = mAppThreads[threadID];
    if (!context || !context->mDispatcher)
    {
        return false;
    }

    context->mDispatcher->Queue(callback);
    return true;
}

TaskScheduler& AsyncImpl::GetScheduler()
{
    return mScheduler;
}

void AsyncImpl::PlatformInitAppThread(AppThreadContext* context)
{
    // TODO: Worker Thread Implementation
    SetThreadLocalData(context->mAppThreadId, INVALID_APP_WORKER_THREAD_ID);
    context->SetState(AppThreadContext::USER_EXECUTE);
    context->mPlatformFence.Set(false);
}
void AsyncImpl::PlatformShutdownAppThread(AppThreadContext* context)
{
    SetThreadLocalData(INVALID_APP_THREAD_ID, INVALID_APP_WORKER_THREAD_ID);
    context->SetState(AppThreadContext::STOPPED);
}
void AsyncImpl::PlatformThreadProc(AppThreadContext* context)
{
    PlatformInitAppThread(context);

    AppThread appThread(context);
    Assert(context->mAppThreadCallback.IsValid());
    context->mAppThreadCallback.Invoke(&appThread);

    context->SetState(AppThreadContext::PLATFORM_RELEASE);
    PlatformShutdownAppThread(context);
}

}
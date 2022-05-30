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
#ifndef LF_RUNTIME_ASYNC_IMPL_H
#define LF_RUNTIME_ASYNC_IMPL_H

#include "Runtime/Async/Async.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadFence.h"
#include "Runtime/Async/AppThread.h"

namespace lf {

DECLARE_PTR(AppThreadContext);

class LF_RUNTIME_API AsyncImpl : public Async
{
public:
    AsyncImpl();
    ~AsyncImpl();

    void Initialize();
    void Shutdown();

    void EnableAppThread();
    void DisableAppThread();

    void RunPromise(PromiseWrapper promise) final;
    void QueuePromise(PromiseWrapper promise) final;
    TaskHandle RunTask(const TaskCallback& callback, void* param = nullptr) final;
    void WaitForSync() final;
    void Signal() final;

    bool IsRunning() const;
    void DrainQueue();

    bool AppThreadRunning() const;
    bool StartThread(AppThreadId threadID, const AppThreadCallback& callback, const AppThreadAttributes& threadAttributes);
    bool StopThread(AppThreadId threadID);
    bool ExecuteOn(AppThreadId threadID, const AppThreadDispatchCallback& callback);
protected:
    TaskScheduler& GetScheduler() final;
private:
    static void PlatformInitAppThread(AppThreadContext* context);
    static void PlatformShutdownAppThread(AppThreadContext* context);
    static void PlatformThreadProc(AppThreadContext* context);


    TaskScheduler mScheduler;

    Thread                 mDrainQueueThread;
    ThreadFence            mFence;
    TVector<PromiseWrapper> mBuffer;
    TVector<PromiseWrapper> mWork;
    SpinLock               mLock;
    volatile Atomic32      mIsRunning;

    volatile Atomic32      mAllowAppThreadExecution;
    RWSpinLock             mAppThreadLock;
    AppThreadContextPtr    mAppThreads[APP_THREAD_ID_MAX];
};

}

#endif // LF_RUNTIME_ASYNC_IMPL_H
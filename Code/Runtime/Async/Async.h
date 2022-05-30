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
#ifndef LF_RUNTIME_ASYNC_H
#define LF_RUNTIME_ASYNC_H

#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Concurrent/TaskTypes.h"
#include "Core/Concurrent/Task.h"


namespace lf {

class Promise;
using PromiseWrapper = TAtomicStrongPointer<Promise>;
class TaskHandle;

using AppThreadId = SizeT;
using AppWorkerThreadId = SizeT;

enum : AppThreadId
{
    INVALID_APP_THREAD_ID = INVALID
};
enum : AppWorkerThreadId
{
    INVALID_APP_WORKER_THREAD_ID = INVALID
};

enum EngineAppThreadId : AppThreadId
{
    // Reserved Thread IDs for the engine
    APP_THREAD_ID_MAIN,
    APP_THREAD_ID_ASYNC,
    APP_THREAD_ID_ASSET_OP,
    APP_THREAD_ID_RENDER,
    APP_THREAD_ID_RENDER_WORKER,
    APP_THREAD_ID_RESERVED_2,
    APP_THREAD_ID_RESERVED_3,
    APP_THREAD_ID_RESERVED_4,
    APP_THREAD_ID_RESERVED_5,
    APP_THREAD_ID_RESERVED_6,
    APP_THREAD_ID_RESERVED_7,
    APP_THREAD_ID_RESERVED_8,
    APP_THREAD_ID_RESERVED_9,
    APP_THREAD_ID_RESERVED_10,
    APP_THREAD_ID_RESERVED_11,
    APP_THREAD_ID_RESERVED_12,

    // Available user thread ids.
    APP_THREAD_ID_USER_BEGIN = 16,
    APP_THREAD_ID_USER_MAX = 32,
    APP_THREAD_ID_MAX = 32
};

class AppThread;
DECLARE_HASHED_CALLBACK(AppThreadDispatchCallback, void);
DECLARE_HASHED_CALLBACK(AppThreadCallback, void, AppThread*);
DECLARE_WPTR(ThreadDispatcher);
struct AppThreadAttributes
{
    LF_INLINE AppThreadAttributes()
    : mWorkerID(INVALID_APP_WORKER_THREAD_ID)
    , mDispatcher()
    {}
    // ** Whether or not the thread is executed as a 'worker', Invalid(mWorkerID) == NotWorker
    AppWorkerThreadId    mWorkerID;
    // ** Dispatcher used to execute work items on other thread.
    ThreadDispatcherWPtr mDispatcher;
};

class LF_RUNTIME_API Async
{
public:
    // ********************************************************************
    // Disable 'StartThread/StopThread' calls and enable 'ExecuteOn'
    // Main thread only!
    // ********************************************************************
    virtual void EnableAppThread() = 0;
    // ********************************************************************
    // Disable 'ExecuteOn' and enable 'StartThread/StopThread'
    // Main thread only!
    // ********************************************************************
    virtual void DisableAppThread() = 0;
    // **********************************
    // Pushes a promise into the task scheduler immediately for execution.
    // 
    // Chained tasks (Then/Error) are not guaranteed to be executed.
    // **********************************
    virtual void RunPromise(PromiseWrapper promise) = 0;
    // **********************************
    // Pushes a promise into the 'next-frame' queue. (note: If a frame takes an 
    // excessively long time to execute (more than 100ms) then the promise is
    // pushed into the task scheduler for execution.
    // **********************************
    virtual void QueuePromise(PromiseWrapper promise) = 0;
    // **********************************
    // Pushes a simple task into the thread scheduler
    // **********************************
    virtual TaskHandle RunTask(const TaskCallback& callback, void* param = nullptr) = 0;
    // **********************************
    // Use this to yield your thread execution until the 'next-frame'. This is not to be used
    // on the main thread, except for testing.
    // **********************************
    virtual void WaitForSync() = 0;
    // **********************************
    // Signals to dispatch queued promises.
    // **********************************
    virtual void Signal() = 0;

    template<typename LambdaT>
    TaskHandle RunTask(const LambdaT& lambda, void* param = nullptr)
    {
        return RunTask(TaskCallback::Make(lambda), param);
    }

    template<typename ResultTypeT, typename LambdaT>
    Task<ResultTypeT> Run(const LambdaT& callback)
    {
        return Task<ResultTypeT>(typename Task<ResultTypeT>::TaskCallback::Make(callback), GetScheduler());
    }

    // **********************************
    // Function function designed for the use of waiting on a collection
    // of promises using iterators.
    // **********************************
    template<typename T, typename P>
    static LF_INLINE void WaitAll(T first, T last, P pred)
    {
        SizeT required = (last - first);
        SizeT done = 0;
        while (done != required)
        {
            done = 0;
            for (T it = first; it != last; ++it)
            {
                if (pred(*it))
                {
                    ++done;
                }
            }
        }
    }

    // ********************************************************************
    // Checks to see if the app threading is running.
    // If true then StartThread/StopThread cannot be called, ExecuteOn can be called
    // If false then vice versa (StartThread/StopThread can be called, ExecuteOn cannot be called)
    // ********************************************************************
    virtual bool AppThreadRunning() const = 0;
    // ********************************************************************
    // Starts up a App Thread
    // ********************************************************************
    virtual bool StartThread(AppThreadId threadID, const AppThreadCallback& callback, const AppThreadAttributes& threadAttributes) = 0;
    // ********************************************************************
    // Stops the App Thread
    // ********************************************************************
    virtual bool StopThread(AppThreadId threadID) = 0;
    // ********************************************************************
    // Executes code on another thread
    // ********************************************************************
    virtual bool ExecuteOn(AppThreadId threadID, const AppThreadDispatchCallback& callback) = 0;

    static AppThreadId GetAppThreadId();
    static AppWorkerThreadId GetAppWorkerThreadId();
protected:
    virtual TaskScheduler& GetScheduler() = 0;
    static void SetThreadLocalData(AppThreadId appThreadID, AppWorkerThreadId appWorkerThreadID);
};

LF_RUNTIME_API Async& GetAsync();

}

#endif // LF_RUNTIME_ASYNC_H
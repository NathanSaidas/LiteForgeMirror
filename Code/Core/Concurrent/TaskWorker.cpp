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
#include "Core/PCH.h"
#include "TaskWorker.h"
#include "Core/Platform/ThreadFence.h"

namespace lf {

TaskWorker::TaskWorker()
: mThread()
, mRunning(0)
, mDispatcherQueue(nullptr)
, mDispatcherFence(nullptr)
, mAsync(false)
{}
TaskWorker::TaskWorker(const TaskWorker&)
{
    // Stubbed just for compliation since we use TArray
    CriticalAssertMsgEx("Copying TaskWorker is not allowed!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}
TaskWorker::~TaskWorker() 
{
    // If this trips, we're destroying the TaskWorker without calling Shutdown! Background thread could still be running!
    CriticalAssertEx(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    // If this trips, we haven't stopped the background thread, use Join!
    CriticalAssertEx(!mThread.IsRunning(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

TaskWorker& TaskWorker::operator=(const TaskWorker&)
{
    // Stubbed just for compliation since we use TArray
    CriticalAssertMsgEx("Copying TaskWorker is not allowed!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    return *this;
}

// void TaskWorker::Initialize(RingBufferType* dispatcherQueue, ThreadFence* dispatcherFence, bool async)
void TaskWorker::Initialize(RingBufferType* dispatcherQueue
    , ThreadFence* dispatcherFence
    , bool async
#if defined(LF_DEBUG) || defined(LF_TEST)
    , const char* workerName
#endif
)
{
    // If either of these trip, you're likely calling Initialize twice!
    AssertEx(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(!mThread.IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(!mDispatcherQueue && dispatcherQueue, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    if (IsRunning() || mThread.IsRunning())
    {
        return;
    }
    mDispatcherQueue = dispatcherQueue;
    mDispatcherFence = dispatcherFence;
    mAsync = async;
    SetRunning(true);
    if (async)
    {
        Fork();
#if defined(LF_DEBUG) || defined(LF_TEST)
        if (!workerName)
        {
            workerName = "WorkerThread";
        }
        mThread.SetDebugName(workerName);
#endif
    }
}
void TaskWorker::Shutdown()
{
    // If either of these trip, you're likely calling Shutdown twice!
    AssertEx(IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(!mAsync || mThread.IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    SetRunning(false);
}

void TaskWorker::Join()
{
    while (mThread.IsRunning())
    {
        SleepCallingThread(0); // todo: YieldThread()
    }
    mDispatcherQueue = nullptr;
    mDispatcherFence = nullptr;
    mAsync = false;
}

void TaskWorker::UpdateSync()
{
    if (IsAsync())
    {
        ReportBugMsgEx("TaskWorker::UpdateSync cannot be called on an asynchronous worker!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    Update();
}

void TaskWorker::Update()
{
    TaskTypes::TaskRingBufferResult result = mDispatcherQueue->TryPop();
    if (result)
    {
        TaskTypes::TaskRingBufferResultWrapper task = result.mData;
        // It's possible for a TaskHandle to 'Wait' and complete this task, in that case just skip
        if (task.mCallback)
        {
            task.mCallback.Invoke(task.mParam);
        }
    }
}

void TaskWorker::BackgroundUpdate()
{
    while (IsRunning())
    {
        Update();
        if (mDispatcherQueue->Size() == 0)
        {
            mDispatcherFence->Wait();
        }
    }
}

}
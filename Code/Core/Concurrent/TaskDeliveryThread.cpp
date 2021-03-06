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
#if defined(LF_MPMC_BOUNDLESS_EXP)
#include "TaskDeliveryThread.h"
#endif

namespace lf {

#if defined(LF_MPMC_BOUNDLESS_EXP)
TaskDeliveryThread::TaskDeliveryThread() :
mRingBuffer(),
mFatBuffer(),
mInternalFatBuffer(),
mRunning(0),
mBufferLock(),
mAsync(false),
mRingBufferDrain(16),
mFatBufferDrain(128),
mDispatcherQueue(nullptr),
mThread()
{}
TaskDeliveryThread::TaskDeliveryThread(const TaskDeliveryThread& )
{
    // Stubbed just for compliation since we use TArray
    CriticalAssertMsgEx("Copying TaskDeliveryThread is not allowed!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}
TaskDeliveryThread::~TaskDeliveryThread()
{
    // If this trips, we're destroying the TaskDeliveryThread without calling Shutdown! Background thread could still be running!
    AssertEx(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    // If this trips, we haven't stopped the background thread, use Join!
    AssertEx(!mThread.IsRunning(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

TaskDeliveryThread& TaskDeliveryThread::operator=(const TaskDeliveryThread&)
{
    // Stubbed just for compliation since we use TArray
    CriticalAssertMsgEx("Copying TaskDeliveryThread is not allowed!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    return *this;
}

void TaskDeliveryThread::Initialize(RingBufferType* dispatcherQueue, bool async)
{
    Initialize(OptionsType(), dispatcherQueue, async);
}

void TaskDeliveryThread::Initialize(const OptionsType& options, RingBufferType* dispatcherQueue, bool async)
{
    // If either of these trip, you're likely calling Initialize twice!
    AssertEx(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(!mThread.IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(!mDispatcherQueue && dispatcherQueue, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    // Perhaps you didn't PopPendingItems()
    AssertEx(mFatBuffer.Empty() && mInternalFatBuffer.Empty(), LF_ERROR_BAD_STATE, ERROR_API_CORE);

    mRingBuffer.Resize(options.mFastBufferCapacity);
    mDispatcherQueue = dispatcherQueue;
    mAsync = async;
    mRingBufferDrain = options.mRingBufferDrain;
    mFatBufferDrain = options.mFatBufferDrain;
    SetRunning(true);
    if (async)
    {
        Fork();
    }
}
void TaskDeliveryThread::Shutdown()
{
    SetRunning(false);
    Join();
    mDispatcherQueue = nullptr;
    mAsync = false;
}

TaskTypes::TaskEnqueueStatus TaskDeliveryThread::Enqueue(const TaskItemType& task)
{
    if (!IsRunning() || !IsAsync())
    {
        // Ensure all tasks queued while shutdown run
        task->mCallback.Invoke(task->mParam);
        return TaskTypes::TES_SYNCHRONOUS;
    }

    // Try fast-buffer
    SizeT attempts = 10;
    while (attempts > 0)
    {
        if (mRingBuffer.TryPush(task))
        {
            return TaskTypes::TES_LOCK_FREE;
        }
        --attempts;
    }

    // Oops, congested try slow lock
    ScopeLock lock(mBufferLock);
    mFatBuffer.Add(task);
    return TaskTypes::TES_LOCK;
}

void TaskDeliveryThread::UpdateSync()
{
    if (IsAsync())
    {
        ReportBugMsgEx("TaskDeliveryThread::UpdateSync cannot be called on an asynchronous worker!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    Update();
}

TArray<TaskDeliveryThread::TaskItemType> TaskDeliveryThread::PopPendingItems()
{
    TArray<TaskDeliveryThread::TaskItemType> result;
    if (IsRunning())
    {
        ReportBugMsgEx("TaskWorker::PopPendingItems cannot be called while running!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return result;
    }

    while (mRingBuffer.Size() > 0)
    {
        auto taskResult = mRingBuffer.TryPop();
        if (taskResult)
        {
            result.Add(taskResult.mData);
        }
    }

    ScopeLock lock(mBufferLock);
    result.Insert(result.end(), mInternalFatBuffer.begin(), mInternalFatBuffer.end());
    result.Insert(result.end(), mFatBuffer.begin(), mFatBuffer.end());
    mInternalFatBuffer.Clear();
    mFatBuffer.Clear();
    return result;
}

void TaskDeliveryThread::Update()
{
    const SizeT RING_BUFFER_DRAIN = mRingBufferDrain;
    const SizeT FAT_BUFFER_DRAIN = mFatBufferDrain;
    // Drain Ring Buffer:
    for (SizeT i = 0; i < RING_BUFFER_DRAIN && mRingBuffer.Size() > 0; ++i)
    {
        auto result = mRingBuffer.TryPop();
        if (result)
        {
            bool pushed = false;
            for (SizeT attempt = 0; attempt < 3; ++attempt)
            {
                if (mDispatcherQueue->TryPush(result.mData))
                {
                    pushed = true;
                    break;
                }
            }

            // Dispatch Stall
            if (!pushed)
            {
                mInternalFatBuffer.Add(result.mData);
            }
        }
    }

    // todo: there could possibly be contention here, so we should profile for it.
    // Lock&Swap
    mBufferLock.Acquire();
    mInternalFatBuffer.Insert(mInternalFatBuffer.end(), mFatBuffer.begin(), mFatBuffer.end());
    mBufferLock.Release();

    // Drain fat buffer
    SizeT attempts = FAT_BUFFER_DRAIN;
    for (SizeT i = 0; i < mInternalFatBuffer.Size(); ++i)
    {
        while (attempts > 0)
        {
            --attempts;
            if (mDispatcherQueue->TryPush(mInternalFatBuffer.GetFirst()))
            {
                mInternalFatBuffer.Remove(mInternalFatBuffer.begin());
                break;
            }
        }

        if (attempts == 0)
        {
            break;
        }
    }
}

void TaskDeliveryThread::BackgroundUpdate()
{
    while (IsRunning())
    {
        Update();
    }
}
#endif

} // namespace lf
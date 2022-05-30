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
#pragma once

#include "Core/Concurrent/TaskTypes.h"
#include "Core/Platform/SpinLock.h"

namespace lf {

#if defined(LF_MPMC_BOUNDLESS_EXP)

// todo: To protect against huge stalls we should have a hard limit on the number of items that can possibly
//       fit in the TaskDeliveryThread and wait until all current work is done before proceeding.
class TaskDeliveryThread
{
public:
    using TaskItemType = TaskTypes::TaskItemType;
    using RingBufferType = TaskTypes::RingBufferType;
    using OptionsType = TaskTypes::TaskDeliveryThreadOptions;
    // **********************************
    // Constructs a task delivery thread, initializing internal data with default values,
    // If you want to run the thread you must call Initialize
    // **********************************
    TaskDeliveryThread();
    // **********************************
    // Forbidden-Action, we don't allow copying but it's required for TArray implementation (but we don't use it)
    // **********************************
    TaskDeliveryThread(const TaskDeliveryThread&);
    // **********************************
    // Deconstructs the thread object ensuring all resources have been released!
    // **********************************
    ~TaskDeliveryThread();
    // **********************************
    // Forbidden-Action, we don't allow copying but it's required for TArray implementation (but we don't use it)
    // **********************************
    TaskDeliveryThread& operator=(const TaskDeliveryThread&);

    // **********************************
    // Initializes the TaskDelivery thread starting a background worker thread if async.
    // @param options [Optional] -- Configuration options for the TaskDeliveryThread
    // @param dispatcherQueue    -- The queue the delivery thread will produce work for
    // @param async              -- A flag signalling whether or not a background worker should
    //                              be used or not, if you pass in 'false' make sure you call
    //                              UpdateSync
    // **********************************
    void Initialize(RingBufferType* dispatcherQueue, bool async);
    void Initialize(const OptionsType& options, RingBufferType* dispatcherQueue, bool async);
    // **********************************
    // Stops the delivery background-thread (if async) finishing it's current work item and then
    // releasing any acquired resources
    // note: After calling this function you should call PopPendingItems to get any items that are held within
    //       the internal buffers that could not fit in the dispatcher queue and complete the tasks returned
    // **********************************
    void Shutdown();
    // **********************************
    // Enqueues a task within the delivery thread, the task is guaranteed to be completed
    // 
    // note: Enqueuing an item while the delivery thread is not running is not recommended
    //       however it will run synchronously to keep the 'task is guaranteed to complete'
    // @returns A status of how the task was queued/executed
    // **********************************
    TaskTypes::TaskEnqueueStatus Enqueue(const TaskItemType& task);
    // **********************************
    // Updates task delivery synchronously, cannot be called if the TaskDeliveryThread was initialized
    // as an async thread.
    // **********************************
    void UpdateSync();
    // **********************************
    // Pops all remaining items off all internal buffers and returns an array of tasks to be completed.
    // **********************************
    TArray<TaskItemType> PopPendingItems();
    // **********************************
    // Check if the delivery thread is currently running
    // **********************************
    bool IsRunning() const { return AtomicLoad(&mRunning) > 0; }
    // **********************************
    // Check if the delivery thread was initialized as an asynchronous worker.
    // **********************************
    bool IsAsync() const { return mAsync; }
private:
    void SetRunning(bool value) { AtomicStore(&mRunning, value ? 1 : 0); }
    void Fork() { mThread.Fork(BackgroundUpdateEntry, this); }
    void Join() { if (mThread.IsRunning()) { mThread.Join(); } }
    void Update();

    // Background updated thread, only runs if async
    void BackgroundUpdate();
    static void BackgroundUpdateEntry(void* param) { static_cast<TaskDeliveryThread*>(param)->BackgroundUpdate(); }

    // MPMC collection used to pass items quickly between threads
    RingBufferType       mRingBuffer;
    // Protected buffer for keeping overflowing items
    TArray<TaskItemType> mFatBuffer;
    // Synchronized buffer used only by the worker thread to push items onto the dispatcher queue.
    TArray<TaskItemType> mInternalFatBuffer;
    // Atomic state of running
    volatile Atomic32    mRunning;
    // Lock protected mFatBuffer access
    SpinLock             mBufferLock;
    // Initialization flag mAsync
    bool                 mAsync;
    // Initialization option for how many iterations of trying to pop something off the ring buffer should be done
    SizeT                mRingBufferDrain;
    // Initialization option for how many iterations of trying to pop something off the fat buffer drain
    SizeT                mFatBufferDrain;
    // Dispatcher MPMC collection we 'produce' for.
    RingBufferType*      mDispatcherQueue;
    // Thread executing the background worker.
    Thread               mThread;
};

#endif

} // namespace lf
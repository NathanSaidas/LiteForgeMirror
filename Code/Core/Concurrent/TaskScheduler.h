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
#ifndef LF_CORE_TASK_SCHEDULER_H
#define LF_CORE_TASK_SCHEDULER_H

#include "Core/Platform/ThreadSignal.h"
#include "Core/Concurrent/TaskTypes.h"
#include "Core/Concurrent/TaskHandle.h"

namespace lf {

class TaskWorker;
#if defined(LF_MPMC_BOUNDLESS_EXP)
class TaskDeliveryThread;
#endif



class LF_CORE_API TaskScheduler
{
public:
    using TaskItemType = TaskTypes::TaskItemType;
    using RingBufferType = TaskTypes::RingBufferType;
    using OptionsType = TaskTypes::TaskSchedulerOptions;

    // **********************************
    // Constructs the task scheduler with default values,
    // Must call Initialize to actually run the scheduler
    // **********************************
    TaskScheduler();
    TaskScheduler(const TaskScheduler&) = delete;
    TaskScheduler(TaskScheduler&&) = delete;
    
    // **********************************
    // Deconstructs the scheduler verfiying allocated resources are
    // in fact deallocated and the scheduler state is no longer running
    // **********************************
    ~TaskScheduler();

    // **********************************
    // Initializes the TaskScheduler spinning up the TaskWorkers/TaskDeliveryThreads
    // @param options [Optional] -- Configuration options for the scheduler and child objects
    // @param async              -- A flag for signalling whether the algorithm runs in a async
    //                              Synchronous mode. (Synchronous is for debugging)
    // **********************************
    void Initialize(bool async = false);
    void Initialize(const OptionsType& options, bool async = false);
    // **********************************
    // Shutsdown all the worker/delivery threads then pops off all pending tasks and executes them
    // **********************************
    void Shutdown();

    // **********************************
    // Task runner functions, tasks run are guaranteed to be completed but running on a seperate 
    // thread or even async is not guaranteed.
    // **********************************
    TaskHandle RunTask(TaskLambdaCallback func, void* param = nullptr);
    // void RunTask(TaskItemAtomicPtr& task, TaskLambdaCallback func, void* param = nullptr);
    TaskHandle RunTask(TaskCallback func, void* param = nullptr);

    template<typename LambdaT>
    TaskHandle RunTask(const LambdaT& lambda, void* param = nullptr)
    {
        return RunTask(TaskCallback::CreateLambda(lambda), param);
    }

    // void RunTask(TaskItemAtomicPtr& task, TaskCallback func, void* param = nullptr);
    // **********************************
    // Check if the scheduler is currently running
    // **********************************
    bool IsRunning() const { return AtomicLoad(&mRunning) != 0; }
    // **********************************
    // Check if the scheduler is running as a asynchronous task scheduler
    // **********************************
    bool IsAsync() const { return mAsync; }
private:
    void SetRunning(bool value) { AtomicStore(&mRunning, value ? 1 : 0); }

    RingBufferType mDispatcherQueue;
    ThreadSignal   mDispatcherSignal;
    // Workers:
    TArray<TaskWorker> mWorkerThreads;

    volatile Atomic32 mRunning;
    bool mAsync;
};

// Below is some code that was written as test to build a wait-free/lock-free/boundless MPMC
// the current result is something that is horribly slow, especially under massive stress
// thus we are not currently using it
#if defined(LF_MPMC_BOUNDLESS_EXP)

// **********************************
// The inspiration behind the scheduler is to create a wait-free/lock-free/unbounded MPMC
// to distribute work quickly with low-latency to other threads.
// 
// The latency here refers to how much time it takes from the moment you call 'RunTask' to the moment
// a worker wakes up and executes it, this can be a little offset if workers are busy executing other
// work
//
//
//
// **********************************
class TaskScheduler
{
public:
    using TaskItemType = TaskTypes::TaskItemType;
    using RingBufferType = TaskTypes::RingBufferType;
    using OptionsType = TaskTypes::TaskSchedulerOptions;
    // **********************************
    // Constructs the task scheduler with default values,
    // Must call Initialize to actually run the scheduler
    // **********************************
    TaskScheduler();
    // **********************************
    // Deconstructs the scheduler verfiying allocated resources are
    // in fact deallocated and the scheduler state is no longer running
    // **********************************
    ~TaskScheduler();

    // **********************************
    // Initializes the TaskScheduler spinning up the TaskWorkers/TaskDeliveryThreads
    // @param options [Optional] -- Configuration options for the scheduler and child objects
    // @param async              -- A flag for signalling whether the algorithm runs in a async
    //                              Synchronous mode. (Synchronous is for debugging)
    // **********************************
    void Initialize(bool async = false);
    void Initialize(const OptionsType& options, bool async = false);
    // **********************************
    // Shutsdown all the worker/delivery threads then pops off all pending tasks and executes them
    // **********************************
    void Shutdown();

    // **********************************
    // Task runner functions, tasks run are guaranteed to be completed but running on a seperate 
    // thread or even async is not guaranteed.
    // **********************************
    TaskItemAtomicPtr RunTask(TaskLambdaCallback func, void* param = nullptr);
    void RunTask(TaskItemAtomicPtr& task, TaskLambdaCallback func, void* param = nullptr);
    TaskItemAtomicPtr RunTask(TaskCallback func, void* param = nullptr);
    void RunTask(TaskItemAtomicPtr& task, TaskCallback func, void* param = nullptr);
    // **********************************
    // Updates the delivery threads and workers synchronously, this can only be used if 
    // initialized as non-async
    // **********************************
    void UpdateSync();
    // **********************************
    // Check if the scheduler is currently running
    // **********************************
    bool IsRunning() const { return AtomicLoad(&mRunning) != 0; }
    // **********************************
    // Check if the scheduler is running as a asynchronous task scheduler
    // **********************************
    bool IsAsync() const { return mAsync; }
private:
    void SetRunning(bool value) { AtomicStore(&mRunning, value ? 1 : 0); }
    // Round-Robin selection on delivery
    TaskDeliveryThread& Select();

    // Delivery:
    TArray<TaskDeliveryThread> mDeliveryThreads;
    Atomic32 mId;

    // Dispatcher:
    RingBufferType mDispatcherQueue;

    // Workers:
    TArray<TaskWorker> mWorkerThreads;

    volatile Atomic32 mRunning;
    bool mAsync;
};

#endif

} // namespace lf

#endif // LF_CORE_TASK_SCHEDULER_H
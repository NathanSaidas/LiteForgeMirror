// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_TASK_WORKER_H
#define LF_CORE_TASK_WORKER_H

#include "Core/Concurrent/TaskTypes.h"
#include "Core/Platform/Thread.h"

namespace lf {

class ThreadSignal;

class TaskWorker
{
public:
    using TaskItemType = TaskTypes::TaskItemType;
    using RingBufferType = TaskTypes::RingBufferType;
    // **********************************
    // Initializes the default values of the TaskWorker object, to actually
    // run the worker you must call Initialize
    // **********************************
    TaskWorker();
    // **********************************
    // Forbidden-Action, we don't allow copying but it's required for TArray implementation (but we don't use it)
    // **********************************
    TaskWorker(const TaskWorker&);
    // **********************************
    // Ensures the object correctly released resources (threads)
    // **********************************
    ~TaskWorker();
    // **********************************
    // Forbidden-Action, we don't allow copying but it's required for TArray implementation (but we don't use it)
    // **********************************
    TaskWorker& operator=(const TaskWorker&);
    // **********************************
    // Initializes the TaskWorker, marking it as 'running'
    // @param dispatcherQueue -- The dispatcher queue the worker will 'pop' items from.
    // @param async -- If true then a background thread will be spun up to process items, otherwise
    //                 the caller must call UpdateSync to process items.
    // **********************************
    void Initialize(RingBufferType* dispatcherQueue, ThreadSignal* dispatcherSignal, bool async);
    // **********************************
    // Marks the worker as not running then waits for it to complete it's current work item
    // and releases resources used. (Thread handles)
    // **********************************
    void Shutdown();

    void Join();
    // **********************************
    // Updates the worker in synchronous fashion, calling this method on a async worker is a invalid
    // operation.
    // **********************************
    void UpdateSync();
    // **********************************
    // Check if the worker is currently running
    // **********************************
    bool IsRunning() const { return AtomicLoad(&mRunning) > 0; }
    // **********************************
    // Check if the worker was initialized as an asynchronous worker.
    // **********************************
    bool IsAsync() const { return mAsync; }
private:
    void SetRunning(bool value) { AtomicStore(&mRunning, value ? 1 : 0); }
    void Fork() { mThread.Fork(BackgroundUpdateEntry, this); }
    void Update();

    // Background thread updating function, runs until the worker is no longer 'running'
    void BackgroundUpdate();
    static void BackgroundUpdateEntry(void* param) { static_cast<TaskWorker*>(param)->BackgroundUpdate(); }
    // Background update thread
    Thread              mThread;
    // Atomic running state
    volatile Atomic32   mRunning;
    // The MPMC collection we 'consume' from.
    RingBufferType*     mDispatcherQueue;
    // A signal we can wait on if there is no work todo (pauses thread execution)
    ThreadSignal*       mDispatcherSignal;
    // Property to contain the async or not state
    bool                mAsync;
};

} // namespace lf

#endif // LF_CORE_TASK_WORKER_H
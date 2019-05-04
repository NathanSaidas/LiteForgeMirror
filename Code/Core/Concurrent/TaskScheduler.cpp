// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "TaskScheduler.h"
#include "Core/Concurrent/TaskDeliveryThread.h"
#include "Core/Concurrent/TaskWorker.h"

namespace lf {

TaskScheduler::TaskScheduler() :
mDispatcherQueue(),
mWorkerThreads(),
mRunning(0),
mAsync(false)
{

}

TaskScheduler::~TaskScheduler()
{
    // If these trip you're forgetting a call to Shutdown
    AssertError(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(mWorkerThreads.Empty(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    // Oh no some tasks are not going to be run even though we have a guarantee to run them
    AssertError(mDispatcherQueue.Size() == 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}

void TaskScheduler::Initialize(bool async)
{
    Initialize(OptionsType(), async);
}

void TaskScheduler::Initialize(const OptionsType& options, bool async)
{
    AssertError(options.mNumDeliveryThreads > 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(options.mNumWorkerThreads > 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);

    // If these trip you haven't called Shutdown, thus the scheduler is still running!
    AssertError(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(mWorkerThreads.Empty(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(mDispatcherQueue.Size() == 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    mAsync = async;
    mWorkerThreads.Resize(options.mNumWorkerThreads);
    mDispatcherQueue.Resize(options.mDispatcherSize);

    // Spin up workers first to process work ASAP
    for (TaskWorker& worker : mWorkerThreads)
    {
        worker.Initialize(&mDispatcherQueue, &mDispatcherSignal, async);
    }

    SetRunning(true);
}

void TaskScheduler::Shutdown()
{
    AssertError(IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    SetRunning(false);
    // Tasks that were not completed by workers that must be completed now!
    TArray<TaskItemType> spilledTasks;

    for (TaskWorker& worker : mWorkerThreads)
    {
        worker.Shutdown();
    }
    mDispatcherSignal.WakeAll();
    for (TaskWorker& worker : mWorkerThreads)
    {
        worker.Join();
    }

    mWorkerThreads.Clear();

    while (mDispatcherQueue.Size() > 0)
    {
        auto result = mDispatcherQueue.TryPop();
        if (result)
        {
            TaskItemType task;
            task.mCallback = result.mData.mCallback;
            task.mParam = result.mData.mParam;
            if (task.mCallback)
            {
                spilledTasks.Add(task);
            }
        }
    }

    for (TaskItemType& task : spilledTasks)
    {
        task.mCallback.Invoke(task.mParam);
    }

    // If this trips, perhaps someone pushed onto the queue while we were executing pending tasks.
    AssertError(mDispatcherQueue.Size() == 0, LF_ERROR_BAD_STATE, ERROR_API_CORE);
}

TaskHandle TaskScheduler::RunTask(TaskLambdaCallback func, void* param)
{
    return RunTask(TaskCallback(func), param);
}
TaskHandle TaskScheduler::RunTask(TaskCallback func, void* param)
{
    TaskItem taskItem;
    taskItem.mCallback = func;
    taskItem.mParam = param;
    TaskHandle taskHandle;
    do {
        taskHandle = mDispatcherQueue.TryPush(taskItem);
    } while (!taskHandle);
    mDispatcherSignal.WakeOne();
    return taskHandle;
}

#if defined(LF_MPMC_BOUNDLESS_EXP)

TaskScheduler::TaskScheduler() : 
mDeliveryThreads(),
mId(0),
mDispatcherQueue(),
mWorkerThreads(),
mRunning(0),
mAsync(false)
{

}
TaskScheduler::~TaskScheduler()
{
    // If these trip you're forgetting a call to Shutdown
    AssertError(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(mDeliveryThreads.Empty(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    AssertError(mWorkerThreads.Empty(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    // Oh no some tasks are not going to be run even though we have a guarantee to run them
    AssertError(mDispatcherQueue.Size() == 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}

void TaskScheduler::Initialize(bool async)
{
    Initialize(OptionsType(), async);
}
void TaskScheduler::Initialize(const OptionsType& options, bool async)
{
    AssertError(options.mNumDeliveryThreads > 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(options.mNumWorkerThreads > 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);

    // If these trip you haven't called Shutdown, thus the scheduler is still running!
    AssertError(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(mDeliveryThreads.Empty(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(mWorkerThreads.Empty(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(mDispatcherQueue.Size() == 0, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    AtomicStore(&mId, 0);
    mAsync = async;
    mDeliveryThreads.Resize(options.mNumDeliveryThreads);
    mWorkerThreads.Resize(options.mNumWorkerThreads);
    mDispatcherQueue.Resize(options.mDispatcherSize);

    // Spin up workers first to process work ASAP
    for (TaskWorker& worker : mWorkerThreads)
    {
        worker.Initialize(&mDispatcherQueue, async);
    }

    for (TaskDeliveryThread& thread : mDeliveryThreads)
    {
        thread.Initialize(options.mDeliveryOptions, &mDispatcherQueue, async);
    }

    SetRunning(true);
}

void TaskScheduler::Shutdown()
{
    AssertError(IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    SetRunning(false);
    // Tasks that were not completed by workers that must be completed now!
    TArray<TaskItemType> spilledTasks;

    // Reverse init order, stop generating tasks
    for (TaskDeliveryThread& thread : mDeliveryThreads)
    {
        thread.Shutdown();
        TArray<TaskItemType> tasks = thread.PopPendingItems();
        spilledTasks.Insert(spilledTasks.end(), tasks.begin(), tasks.end());
    }

    for (TaskWorker& worker : mWorkerThreads)
    {
        worker.Shutdown();
    }

    mDeliveryThreads.Clear();
    mWorkerThreads.Clear();

    while (mDispatcherQueue.Size() > 0)
    {
        auto result = mDispatcherQueue.TryPop();
        if (result)
        {
            spilledTasks.Add(result.mData);
        }
    }

    for (TaskItemType& task : spilledTasks)
    {
        task->mCallback.Invoke(task->mParam);
    }

    // If this trips, perhaps someone pushed onto the queue while we were executing pending tasks.
    AssertError(mDispatcherQueue.Size() == 0, LF_ERROR_BAD_STATE, ERROR_API_CORE);
}

TaskItemAtomicPtr TaskScheduler::RunTask(TaskLambdaCallback func, void* param)
{
    return RunTask(TaskCallback(func), param);
}
void TaskScheduler::RunTask(TaskItemAtomicPtr& task, TaskLambdaCallback func, void* param)
{
    RunTask(task, TaskCallback(func), param);
}

TaskItemAtomicPtr TaskScheduler::RunTask(TaskCallback func, void* param)
{
    TaskItemAtomicPtr task(LFNew<TaskItem>());
    RunTask(task, func, param);
    return task;
}
void TaskScheduler::RunTask(TaskItemAtomicPtr& task, TaskCallback func, void* param)
{
    task->mCallback = func;
    task->mParam = param;
    // todo: if not async and not debugging we should just execute the task and go ahead
    //       for now were exercising the algorithm, but it sucks for singlethreaded execution
    if (!IsRunning())
    {
        func.Invoke(param);
        return;
    }
    Assert(!mDeliveryThreads.Empty());
    TaskDeliveryThread& thread = Select();
    thread.Enqueue(task);
}

void TaskScheduler::UpdateSync()
{
    if (IsAsync())
    {
        ReportBug("TaskScheduler::UpdateSync cannot be called while running asynchronous workers!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    // Deliver some tasks
    for (TaskDeliveryThread& thread : mDeliveryThreads)
    {
        thread.UpdateSync();
    }
    // Then work on them
    for (TaskWorker& worker : mWorkerThreads)
    {
        worker.UpdateSync();
    }
}


TaskDeliveryThread& TaskScheduler::Select() { return mDeliveryThreads[(AtomicIncrement32(&mId) - 1) % mDeliveryThreads.Size()]; }
#endif

} // namespace lf
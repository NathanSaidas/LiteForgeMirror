// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "TaskWorker.h"
#include "Core/Platform/ThreadSignal.h"

namespace lf {

TaskWorker::TaskWorker() :
mThread(),
mRunning(0),
mDispatcherQueue(nullptr),
mAsync(false)
{}
TaskWorker::TaskWorker(const TaskWorker&)
{
    // Stubbed just for compliation since we use TArray
    Crash("Copying TaskWorker is not allowed!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
}
TaskWorker::~TaskWorker() 
{
    // If this trips, we're destroying the TaskWorker without calling Shutdown! Background thread could still be running!
    AssertError(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    // If this trips, we haven't stopped the background thread, use Join!
    AssertError(!mThread.IsRunning(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

TaskWorker& TaskWorker::operator=(const TaskWorker&)
{
    // Stubbed just for compliation since we use TArray
    Crash("Copying TaskWorker is not allowed!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    return *this;
}

void TaskWorker::Initialize(RingBufferType* dispatcherQueue, ThreadSignal* dispatcherSignal, bool async)
{
    // If either of these trip, you're likely calling Initialize twice!
    AssertError(!IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(!mThread.IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(!mDispatcherQueue && dispatcherQueue, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    if (IsRunning() || mThread.IsRunning())
    {
        return;
    }
    mDispatcherQueue = dispatcherQueue;
    mDispatcherSignal = dispatcherSignal;
    mAsync = async;
    SetRunning(true);
    if (async)
    {
        Fork();
    }
}
void TaskWorker::Shutdown()
{
    // If either of these trip, you're likely calling Shutdown twice!
    AssertError(IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertError(!mAsync || mThread.IsRunning(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    SetRunning(false);
}

void TaskWorker::Join()
{
    while (mThread.IsRunning())
    {
        mDispatcherSignal->WakeAll();
    }
    mDispatcherQueue = nullptr;
    mDispatcherSignal = nullptr;
    mAsync = false;
}

void TaskWorker::UpdateSync()
{
    if (IsAsync())
    {
        ReportBug("TaskWorker::UpdateSync cannot be called on an asynchronous worker!", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return;
    }
    Update();
}

void TaskWorker::Update()
{
    auto result = mDispatcherQueue->TryPop();
    if (result)
    {
        auto task = result.mData;
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
            mDispatcherSignal->Wait();
        }
    }
}

}
// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "AsyncImpl.h"
#include "Runtime/Async/Promise.h"

namespace lf {

AsyncImpl::AsyncImpl()
{
}
AsyncImpl::~AsyncImpl()
{

}

void AsyncImpl::Initialize()
{
    mScheduler.Initialize(true);
}
void AsyncImpl::Shutdown()
{
    mScheduler.Shutdown();
}

void AsyncImpl::RunPromise(PromiseWrapper promise)
{
    if (!promise->SetState(Promise::PROMISE_PENDING))
    {
        return;
    }
    if (!mScheduler.IsRunning() || !mScheduler.IsAsync())
    {
        promise->Run();
        return;
    }
    TaskHandle task;
    {
        auto lamb = [promise](void*) { promise->Run(); };
        TaskCallback callback = TaskCallback::CreateLambda(lamb);
        task = mScheduler.RunTask(callback);
    }
    promise->SetTask(task);
}
TaskHandle AsyncImpl::RunTask(const TaskCallback& callback, void* param)
{
    return mScheduler.RunTask(callback, param);
}

}
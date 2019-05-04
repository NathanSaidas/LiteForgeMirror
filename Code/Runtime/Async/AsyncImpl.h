// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_RUNTIME_ASYNC_IMPL_H
#define LF_RUNTIME_ASYNC_IMPL_H

#include "Runtime/Async/Async.h"
#include "Core/Concurrent/TaskScheduler.h"

namespace lf {

class AsyncImpl : public Async
{
public:
    AsyncImpl();
    ~AsyncImpl();

    void Initialize();
    void Shutdown();

    void RunPromise(PromiseWrapper promise) override final;
    TaskHandle RunTask(const TaskCallback& callback, void* param = nullptr) override final;

private:
    TaskScheduler mScheduler;
};

}

#endif // LF_RUNTIME_ASYNC_IMPL_H
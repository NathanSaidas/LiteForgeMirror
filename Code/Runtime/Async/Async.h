// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_RUNTIME_ASYNC_H
#define LF_RUNTIME_ASYNC_H

#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Concurrent/TaskTypes.h"

namespace lf {

class Promise;
using PromiseWrapper = TAtomicStrongPointer<Promise>;
class TaskHandle;

class LF_RUNTIME_API Async
{
public:
    virtual void RunPromise(PromiseWrapper promise) = 0;
    virtual TaskHandle RunTask(const TaskCallback& callback, void* param = nullptr) = 0;

    template<typename LambdaT>
    TaskHandle RunTask(const LambdaT& lambda, void* param = nullptr)
    {
        return RunTask(TaskCallback::CreateLambda(lambda), param);
    }
};

LF_RUNTIME_API Async& GetAsync();

}

#endif // LF_RUNTIME_ASYNC_H
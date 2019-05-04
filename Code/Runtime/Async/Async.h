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
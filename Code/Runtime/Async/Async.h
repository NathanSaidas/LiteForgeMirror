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
    // **********************************
    // Pushes a promise into the task scheduler immediately for execution.
    // 
    // Chained tasks (Then/Error) are not guaranteed to be executed.
    // **********************************
    virtual void RunPromise(PromiseWrapper promise) = 0;
    // **********************************
    // Pushes a promise into the 'next-frame' queue. (note: If a frame takes an 
    // excessively long time to execute (more than 100ms) then the promise is
    // pushed into the task scheduler for execution.
    // **********************************
    virtual void QueuePromise(PromiseWrapper promise) = 0;
    // **********************************
    // Pushes a simple task into the thread scheduler
    // **********************************
    virtual TaskHandle RunTask(const TaskCallback& callback, void* param = nullptr) = 0;
    // **********************************
    // Use this to yield your thread execution until the 'next-frame'. This is not to be used
    // on the main thread, except for testing.
    // **********************************
    virtual void WaitForSync() = 0;
    // **********************************
    // Signals to dispatch queued promises.
    // **********************************
    virtual void Signal() = 0;

    template<typename LambdaT>
    TaskHandle RunTask(const LambdaT& lambda, void* param = nullptr)
    {
        return RunTask(TaskCallback::CreateLambda(lambda), param);
    }

    // **********************************
    // Function function designed for the use of waiting on a collection
    // of promises using iterators.
    // **********************************
    template<typename T, typename P>
    static LF_INLINE void WaitAll(T first, T last, P pred)
    {
        SizeT required = (last - first);
        SizeT done = 0;
        while (done != required)
        {
            done = 0;
            for (T it = first; it != last; ++it)
            {
                if (pred(*it))
                {
                    ++done;
                }
            }
        }
    }
};

LF_RUNTIME_API Async& GetAsync();

}

#endif // LF_RUNTIME_ASYNC_H
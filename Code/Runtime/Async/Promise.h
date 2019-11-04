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
#ifndef LF_RUNTIME_PROMISE_H
#define LF_RUNTIME_PROMISE_H

#include "Core/Concurrent/TaskHandle.h"
#include "Core/Platform/ThreadSignal.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/SmartCallback.h"

namespace lf {

class Async;
class Promise;
using PromiseCallback = TCallback<void, Promise*>;
using PromiseWrapper = TAtomicStrongPointer<Promise>;

// **********************************
// Base class of lite_forge implementation of a promise
// 
// A promise is the concept of running a function 'executor'
// at 'some point in time' not guaranteed to run it now on
// the creating thread and then being able to react based on
// whether or not it was resolved or rejected.
//
// To implement a promise you must use a TPromiseImpl which is used to
// create a promise.
// **********************************
class LF_RUNTIME_API Promise
{
public:
    enum PromiseState
    {
        // Execute has not been called yet
        PROMISE_NULL,
        // Queued to be started 'next-frame'
        PROMISE_QUEUED,
        // Waiting to be executed
        PROMISE_PENDING,
        // Promise was resolved
        PROMISE_RESOLVED,
        // Promise was rejected
        PROMISE_REJECTED
    };

    Promise();
    Promise(const PromiseCallback& executor, Async* async = nullptr);
    // **********************************
    // Invokes the executor callback that created the promise
    // **********************************
    void Run();
    // **********************************
    // Assigns the taskHandle for async promises
    // **********************************
    void SetTask(const TaskHandle& taskHandle);
    // **********************************
    // Attempts to change the state of the promise (state transitions are checked and any invalid transitions result in an error)
    // **********************************
    bool SetState(PromiseState state);
    // **********************************
    // Waits for the promise to complete. (Attempts to 'Run' the promise earlier on the calling thread if possible)
    // **********************************
    void Wait();
    // **********************************
    // Waits for the promise to complete but does not attempt to 'Run' the promise on the calling thread.
    // **********************************
    void LazyWait();

    bool IsPending() const { return AtomicLoad(&mState) == PROMISE_PENDING; }
    bool IsQueued() const { return AtomicLoad(&mState) == PROMISE_QUEUED; }
    bool IsDone() const { return AtomicLoad(&mState) >= PROMISE_RESOLVED; }
    bool IsResolved() const { return AtomicLoad(&mState) == PROMISE_RESOLVED; }
    bool IsRejected() const { return AtomicLoad(&mState) == PROMISE_REJECTED; }
    bool IsEmpty() const { return !mExecutor.IsValid(); }
    // **********************************
    // Invokes all callbacks registered as a resolver,
    // the promise is then marked as resolved.
    // **********************************
    template<typename ... ARGS>
    void Resolve(ARGS&&... args)
    {
        if (!IsPending()) 
        {
            return;
        }

        for (CallbackHandle& callback : mResolverCallbacks)
        {
            TCallback<void, ARGS...> invoker;
            invoker.Assign(callback);
            if (invoker.IsValid())
            {
                invoker.Invoke(args...);
            }
        }
        SetState(PROMISE_RESOLVED);
    }

    // **********************************
    // Invokes all callbacks registered as an error callback
    // the promise is then marked as rejected
    // **********************************
    template<typename ... ARGS>
    void Reject(ARGS&&... args)
    {
        if (!IsPending())
        {
            return;
        }

        for (CallbackHandle& callback : mErrorCallbacks)
        {
            TCallback<void, ARGS...> invoker;
            invoker.Assign(callback);
            if (invoker.IsValid())
            {
                invoker.Invoke(args...);
            }
        }
        SetState(PROMISE_REJECTED);
    }
protected:
    TArray<CallbackHandle> mResolverCallbacks;
    TArray<CallbackHandle> mErrorCallbacks;
    PromiseCallback        mExecutor;
    TaskHandle             mTask;
    Async*                 mAsync;
    ThreadSignal           mStateSignaller;
    volatile Atomic32      mState;
};

} // namespace lf

#endif // LF_RUNTIME_PROMISE_H
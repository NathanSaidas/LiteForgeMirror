// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_RUNTIME_PROMISE_H
#define LF_RUNTIME_PROMISE_H

#include "Core/Concurrent/TaskHandle.h"
#include "Core/Platform/ThreadSignal.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/SmartCallback.h"

namespace lf {

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
class Promise
{
public:
    enum PromiseState
    {
        // Execute has not been called yet
        PROMISE_NULL,
        // Waiting to be executed
        PROMISE_PENDING,
        // Promise was resolved
        PROMISE_RESOLVED,
        // Promise was rejected
        PROMISE_REJECTED
    };

    Promise();
    Promise(const PromiseCallback& executor);
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
    bool IsDone() const { return AtomicLoad(&mState) >= PROMISE_RESOLVED; }
    bool IsResolved() const { return AtomicLoad(&mState) == PROMISE_RESOLVED; }
    bool IsRejected() const { return AtomicLoad(&mState) == PROMISE_REJECTED; }

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
    ThreadSignal           mStateSignaller;
    volatile Atomic32      mState;
};

} // namespace lf

#endif // LF_RUNTIME_PROMISE_H
// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "Promise.h"

namespace lf {

Promise::Promise() :
    mResolverCallbacks(),
    mErrorCallbacks(),
    mExecutor(),
    mTask(),
    mState(PROMISE_NULL)
{}
Promise::Promise(const PromiseCallback& executor) :
    mResolverCallbacks(),
    mErrorCallbacks(),
    mExecutor(executor),
    mTask(),
    mState(PROMISE_NULL)
{}


void Promise::Run()
{
    if (mExecutor.IsValid())
    {
        mExecutor.Invoke(this);
    }
}

void Promise::SetTask(const TaskHandle& taskHandle)
{
    mTask = taskHandle;
}
bool Promise::SetState(PromiseState state)
{
    // Esnure correct state transitions:
    switch (state)
    {
        case PROMISE_NULL:
        {
            if (IsPending())
            {
                ReportBug("Invalid promise state transition PROMISE_PENDING -> PROMISE_NULL", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
        } break;
        case PROMISE_PENDING:
        {
            if (AtomicLoad(&mState) != PROMISE_NULL)
            {
                ReportBug("Invalid promise state transition NOT PROMISE_NULL -> PROMISE_PENDING", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
        } break;
        case PROMISE_RESOLVED:
        {
            if (!IsPending())
            {
                ReportBug("Invalid promise state transition NOT PROMISE_PENDING -> PROMISE_RESOLVED", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
            mStateSignaller.WakeAll();
        } break;

        case PROMISE_REJECTED:
        {
            if (!IsPending())
            {
                ReportBug("Invalid promise state transition NOT PROMISE_PENDING -> PROMISE_REJECTED", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
            mStateSignaller.WakeAll();
        } break;
        default:
            Crash("Invalid promise state!", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
            return false;
    }

    AtomicStore(&mState, state);
    return true;
}

void Promise::Wait()
{
    if (IsPending())
    {
        if (mTask)
        {
            mTask.Wait();
            while (IsPending()) {} // User code must block until user-data says its completed
        }
        else
        {
            Run();
        }
    }
}
void Promise::LazyWait()
{
    while (IsPending())
    {
        if (!mTask)
        {
            Run();
        }
        else
        {
            mStateSignaller.Wait();
        }
    }
}

} // namespace lf
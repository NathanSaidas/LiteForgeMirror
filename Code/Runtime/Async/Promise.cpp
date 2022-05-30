// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#include "Runtime/PCH.h"
#include "Promise.h"
#include "Async.h"

namespace lf {

Promise::Promise() 
: mResolverCallbacks()
, mErrorCallbacks()
, mExecutor()
, mTask()
, mAsync(&GetAsync())
, mStateSignaller()
, mState(PROMISE_NULL)
, mExecuteOnDestroy(true)
{
    mStateSignaller.Initialize();
}
Promise::Promise(const PromiseCallback& executor, Async* async) 
: mResolverCallbacks()
, mErrorCallbacks()
, mExecutor(executor)
, mTask()
, mAsync(async ? async : &GetAsync())
, mStateSignaller()
, mState(PROMISE_NULL)
, mExecuteOnDestroy(true)
{
    mStateSignaller.Initialize();
    mStateSignaller.Set(true);
}
Promise::~Promise()
{
    mStateSignaller.Destroy();
}



void Promise::Run()
{
    mExecuteOnDestroy = false;

    if (AtomicLoad(&mState) == PROMISE_NULL || AtomicLoad(&mState) == PROMISE_QUEUED)
    {
        SetState(PROMISE_PENDING);
    }

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
                ReportBugMsgEx("Invalid promise state transition PROMISE_PENDING -> PROMISE_NULL", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
        } break;
        case PROMISE_QUEUED:
        {
            if (AtomicLoad(&mState) != PROMISE_NULL)
            {
                ReportBugMsgEx("Invalid promise state transition NOT PROMISE_NULL -> PROMISE_QUEUED", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
        } break;
        case PROMISE_PENDING:
        {
            if (AtomicLoad(&mState) != PROMISE_NULL && !IsQueued())
            {
                ReportBugMsgEx("Invalid promise state transition NOT PROMISE_NULL -> PROMISE_PENDING or NOT PROMISE_QUEUED -> PROMISE_PENDING", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
        } break;
        case PROMISE_RESOLVED:
        {
            if (!IsPending())
            {
                ReportBugMsgEx("Invalid promise state transition NOT PROMISE_PENDING -> PROMISE_RESOLVED", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
            mStateSignaller.Set(false);
        } break;

        case PROMISE_REJECTED:
        {
            if (!IsPending())
            {
                ReportBugMsgEx("Invalid promise state transition NOT PROMISE_PENDING -> PROMISE_REJECTED", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
                return false;
            }
            mStateSignaller.Set(false);
        } break;
        default:
            CriticalAssertMsgEx("Invalid promise state!", LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
            return false;
    }

    AtomicStore(&mState, state);
    return true;
}

void Promise::Wait()
{
    if (IsPending() || IsQueued())
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
    while (IsPending() || IsQueued())
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
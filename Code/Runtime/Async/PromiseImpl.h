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
#ifndef LF_RUNTIME_PROMISE_IMPL_H
#define LF_RUNTIME_PROMISE_IMPL_H

#include "Runtime/Async/Promise.h"
#include "Runtime/Async/Async.h"

namespace lf {

template<typename ResolverCallbackT, typename ErrorCallbackT>
class PromiseImpl : public Promise
{
public:
    using ResolverCallbackType = ResolverCallbackT;
    using ErrorCallbackType = ErrorCallbackT;
    using Super = Promise;

    PromiseImpl() 
    : Super()
    {}
    ~PromiseImpl()
    {
        if (mExecuteOnDestroy)
        {
            Execute();
        }
    }
    PromiseImpl(PromiseCallback executor, Async* async = nullptr) 
    : Super(executor, async)
    {}
    template<typename LambdaT>
    PromiseImpl(LambdaT executor, Async* async = nullptr) 
    : Super(PromiseCallback::Make(executor), async)
    {}

    PromiseImpl& Then(ResolverCallbackType callback)
    {
        if (AtomicLoad(&mState) <= PROMISE_QUEUED)
        {
            mResolverCallbacks.push_back(callback.DownCastAnonymous());
        }
        return *this;
    }
    // template<typename LambdaT>
    // PromiseImpl& Then(const LambdaT& callback)
    // {
    //     Then(ResolverCallbackType::Make(callback));
    //     return *this;
    // }

    PromiseImpl& Catch(ErrorCallbackType callback)
    {
        if (AtomicLoad(&mState) <= PROMISE_QUEUED)
        {
            mErrorCallbacks.push_back(callback.DownCastAnonymous());
        }
        return *this;
    }
    // template<typename LambdaT>
    // PromiseImpl& Catch(const LambdaT& callback)
    // {
    //     Catch(ErrorCallbackType::Make(callback));
    //     return *this;
    // }

    PromiseWrapper Execute()
    {
        if (!mExecuteOnDestroy)
        {
            return PromiseWrapper();
        }
        PromiseImpl* promise = LFNew<PromiseImpl>();
        promise->mErrorCallbacks.swap(mErrorCallbacks);
        promise->mResolverCallbacks.swap(mResolverCallbacks);
        promise->mExecutor = std::move(mExecutor);
        promise->mAsync = mAsync;
        promise->mExecuteOnDestroy = false;
        mExecuteOnDestroy = false;
        PromiseWrapper wrapped(promise);
        GetAsync().RunPromise(wrapped);
        return wrapped;
    }

    PromiseWrapper Queue()
    {
        if (!mExecuteOnDestroy)
        {
            return PromiseWrapper();
        }
        PromiseImpl* promise = LFNew<PromiseImpl>();
        promise->mErrorCallbacks.swap(mErrorCallbacks);
        promise->mResolverCallbacks.swap(mResolverCallbacks);
        promise->mExecutor = std::move(mExecutor);
        promise->mAsync = mAsync;
        promise->mExecuteOnDestroy = false;
        mExecuteOnDestroy = false;
        PromiseWrapper wrapped(promise);
        GetAsync().QueuePromise(wrapped);
        return wrapped;
    }

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

        for (AnonymousCallback& callback : mResolverCallbacks)
        {
            ResolverCallbackType invoker;
            if (invoker.UpCast(callback) && invoker.IsValid())
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
        for (AnonymousCallback& callback : mErrorCallbacks)
        {
            ErrorCallbackType invoker;
            if (invoker.UpCast(callback) && invoker.IsValid())
            {
                invoker.Invoke(args...);
            }
        }

        SetState(PROMISE_REJECTED);
    }
};

}

#endif // LF_RUNTIME_PROMISE_IMPL_H
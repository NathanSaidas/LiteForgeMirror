// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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

    PromiseImpl() : Super()
    {}
    PromiseImpl(PromiseCallback executor) : Super(executor)
    {}
    template<typename LambdaT>
    PromiseImpl(LambdaT executor) : Super(PromiseCallback::CreateLambda(executor))
    {}

    PromiseImpl& Then(ResolverCallbackType callback)
    {
        mResolverCallbacks.Add(callback.GetHandle());
        return *this;
    }
    template<typename LambdaT>
    PromiseImpl& Then(const LambdaT& callback)
    {
        Then(ResolverCallbackType::CreateLambda(callback));
        return *this;
    }

    PromiseImpl& Catch(ErrorCallbackType callback)
    {
        mErrorCallbacks.Add(callback.GetHandle());
        return *this;
    }
    template<typename LambdaT>
    PromiseImpl& Catch(const LambdaT& callback)
    {
        Catch(ErrorCallbackType::CreateLambda(callback));
        return *this;
    }

    PromiseWrapper Execute()
    {
        PromiseImpl* promise = LFNew<PromiseImpl>();
        promise->mErrorCallbacks.swap(mErrorCallbacks);
        promise->mResolverCallbacks.swap(mResolverCallbacks);
        promise->mExecutor = std::move(mExecutor);
        PromiseWrapper wrapped(promise);
        GetAsync().RunPromise(wrapped);
        return wrapped;
    }
};

}

#endif // LF_RUNTIME_PROMISE_IMPL_H
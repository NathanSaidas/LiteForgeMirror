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
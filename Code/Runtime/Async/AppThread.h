// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#pragma once

#include "Runtime/Async/Async.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadFence.h"
#include "Runtime/Async/ThreadDispatcher.h"

namespace lf {

DECLARE_PTR(ThreadDispatcher);
class AppThreadContext;
using AsyncAppThreadProc = void(*)(AppThreadContext*);

class LF_RUNTIME_API AppThreadContext
{
public:
    enum State
    {
        STARTED,
        PLATFORM_INITIALIZE,
        USER_EXECUTE,
        USER_SHUTDOWN,
        PLATFORM_RELEASE,
        STOPPED
    };

    AppThreadContext();
    ~AppThreadContext();

    LF_INLINE State GetState() const { return static_cast<State>(AtomicLoad(&mState)); }
    LF_INLINE void SetState(State value) { AtomicStore(&mState, value); }

    volatile Atomic32 mState;
    AppThreadId       mAppThreadId;
    AppThreadCallback mAppThreadCallback;
    ThreadFence       mPlatformFence;
    Thread            mPlatformThread;
    AsyncAppThreadProc mPlatformThreadProc;
    Async*              mAsync;
    ThreadDispatcherPtr mDispatcher;
};

class LF_RUNTIME_API AppThread
{
public:
    AppThread(AppThreadContext* context)
    : mContext(context)
    {}
    
    LF_INLINE AppThreadContext::State GetState() const { return mContext->GetState(); }
    LF_INLINE bool IsRunning() const { return GetState() == AppThreadContext::USER_EXECUTE; }
    LF_INLINE ThreadDispatcher* GetDispatcher() { return mContext->mDispatcher; }
private:
    AppThreadContext* mContext;
};

} // namespace lf
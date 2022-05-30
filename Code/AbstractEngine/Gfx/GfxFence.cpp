// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "AbstractEngine/PCH.h"
#include "GfxFence.h"

namespace lf {

DEFINE_ABSTRACT_CLASS(lf::GfxFence) { NO_REFLECTION; }

GfxFence::GfxFence()
: Super()
, mFenceValue(1)
, mSignalThread()
, mSignalThreadFence()
, mThreadRunning(0)
{
    mSignalThreadFence.Initialize();
}

GfxFence::~GfxFence()
{
    CriticalAssert(!mSignalThread.IsRunning());
    mSignalThreadFence.Destroy();
}

void GfxFence::StartThread()
{
    if (mSignalThread.IsRunning())
    {
        ReportBugMsg("GfxFence::StartThread cannot be called while the thread is running. Call StopThread.");
        return;
    }

    AtomicStore(&mThreadRunning, 1);
    mSignalThreadFence.Set(true);
    mSignalThread.Fork(SignalThread, this);
}

void GfxFence::StopThread()
{
    if (!mSignalThread.IsRunning())
    {
        return;
    }

    AtomicStore(&mThreadRunning, 0);
    mSignalThreadFence.Set(false);
    mSignalThread.Join();
}

void GfxFence::Wait()
{
    WaitImpl();
    WaitComplete();
}

void GfxFence::SetCallback(const FenceWaitCallback& callback)
{
    ScopeLock lock(mWaitLock);
    mWaitCallback = callback;
}

void GfxFence::Signal()
{
    mSignalThreadFence.Signal();
}

void GfxFence::WaitComplete()
{
    ScopeLock lock(mWaitLock);
    if (mWaitCallback.IsValid())
    {
        mWaitCallback.Invoke();
    }
    mWaitCallback = FenceWaitCallback();
}

void GfxFence::SignalThread(void* param)
{
    auto fence = static_cast<GfxFence*>(param);
    while (AtomicLoad(&fence->mThreadRunning) != 0 )
    {
        fence->mSignalThreadFence.Wait();
        fence->WaitImpl();
        fence->WaitComplete();
    }
}


} // namespace lf
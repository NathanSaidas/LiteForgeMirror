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
#include "Runtime/PCH.h"
#include "ThreadDispatcher.h"

namespace lf
{

ThreadDispatcher::ThreadDispatcher()
: mLock()
, mCurrent(0)
, mPending(1)
, mCallbacks()
{
    CriticalAssert(mWaitFence.Initialize());
    mWaitFence.Set(true);
}

ThreadDispatcher::~ThreadDispatcher()
{
    mWaitFence.Set(false);
    mWaitFence.Destroy();
}

void ThreadDispatcher::Queue(const ThreadDispatcherCallback& callback)
{
    ScopeLock lock(mLock);
    Pending().push_back(callback);
    mWaitFence.Signal();
}

void ThreadDispatcher::Wait(SizeT milliseconds)
{
    mWaitFence.Wait(milliseconds);
}
void ThreadDispatcher::Dispatch()
{
    Swap();
    for (ThreadDispatcherCallback& callback : Current())
    {
        if (callback.IsValid())
        {
            callback.Invoke();
        }
    }
    Current().resize(0);
}

void ThreadDispatcher::Swap()
{
    ScopeLock lock(mLock);
    std::swap(mCurrent, mPending);
}

ThreadDispatcher::DispatcherArray& ThreadDispatcher::Current()
{
    return mCallbacks[AtomicLoad(&mCurrent)];
}

ThreadDispatcher::DispatcherArray& ThreadDispatcher::Pending()
{
    return mCallbacks[AtomicLoad(&mPending)];
}

}
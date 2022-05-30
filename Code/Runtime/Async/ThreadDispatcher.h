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

#include "Core/Platform/ThreadFence.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Utility/Array.h"
namespace lf
{
DECLARE_HASHED_CALLBACK(ThreadDispatcherCallback, void);

class LF_RUNTIME_API ThreadDispatcher
{
public:
    // ********************************************************************
    // Constructor
    // ********************************************************************
    ThreadDispatcher();
    virtual ~ThreadDispatcher();


    // ********************************************************************
    // Queue a callback to the dispatcher on the 'pending'
    // @threadsafe
    // ********************************************************************
    void Queue(const ThreadDispatcherCallback& callback);
    // ********************************************************************
    // Swap the dispatcher queue, and execute the 'current'
    // @threadsafe
    // ********************************************************************
    void Dispatch();

    void Wait(SizeT milliseconds = INVALID);
private:
    using DispatcherArray = TVector<ThreadDispatcherCallback>;

    // Locks & Swaps the current/pending.
    // @threadsafe
    void Swap();
    // Returns the current dispatcher array
    DispatcherArray& Current();
    // Returns the pending dispatcher array
    DispatcherArray& Pending();

    // ** The synchronization primitive
    SpinLock mLock;
    // ** Index of the current list
    volatile Atomic32 mCurrent;
    // ** Index of the pending list
    volatile Atomic32 mPending;
    // ** The lists of callbacks to issue
    DispatcherArray mCallbacks[2];

    ThreadFence mWaitFence;
};

} // namespace lf
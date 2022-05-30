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
#include "Core/PCH.h"
#include "RWSpinLock.h"
#include "Core/Common/Assert.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Utility/ErrorCore.h"

namespace lf {

static const Atomic32 SPIN_LOCK_UNLOCKED = 0x7FFFFFFF;
static const SizeT DEFAULT_SPIN_COUNT = 1000;
static const Atomic32 READ_LOCK = 1;
static const Atomic32 WRITE_LOCK = 2;

static void RWAcquire(volatile Atomic32& lock, Atomic32 value)
{
    SizeT spin = DEFAULT_SPIN_COUNT;
    Atomic32 state;
    do
    {
        if (spin == 0)
        {
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
        }
        state  = AtomicCompareExchange(&lock, value, SPIN_LOCK_UNLOCKED);
        --spin;
        // AssertEx(state != value, LF_ERROR_DEADLOCK, ERROR_API_CORE); // If this trips were in deadlock
    } while (state != SPIN_LOCK_UNLOCKED);
}

static void RWRelease(volatile Atomic32& lock, Atomic32 value)
{
    Atomic32 state = AtomicCompareExchange(&lock, SPIN_LOCK_UNLOCKED, value);
    AssertEx(state == value, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips someone has corrupted the ownership of this spin lock, or you're attemping to release without acquiring.
}

RWSpinLock::RWSpinLock()
    : mGlobal(SPIN_LOCK_UNLOCKED)
    , mReaderLock()
    , mReaderCount(0)
{}
RWSpinLock::~RWSpinLock()
{
    bool readerLocked = !mReaderLock.TryAcquire();
    bool globalLocked = AtomicLoad(&mGlobal) != SPIN_LOCK_UNLOCKED;
    if (readerLocked || globalLocked)
    {
        CriticalAssertMsg("RWSpinLock is still locked while being destroyed.");
    }

    CriticalAssert(AtomicLoad(&mReaderCount) == 0);
    mReaderLock.Release();
}

void RWSpinLock::AcquireRead()
{
    mReaderLock.Acquire();
    Atomic32 readers = AtomicIncrement32(&mReaderCount);
    Assert(readers >= 0);
    if (readers == 1)
    {
        RWAcquire(mGlobal, READ_LOCK);
    }
    mReaderLock.Release();
}

void RWSpinLock::AcquireWrite()
{
    RWAcquire(mGlobal, WRITE_LOCK);
}

void RWSpinLock::ReleaseRead()
{
    mReaderLock.Acquire();
    Atomic32 readers = AtomicDecrement32(&mReaderCount);
    Assert(readers >= 0);
    if (readers == 0)
    {
        RWRelease(mGlobal, READ_LOCK);
    }
    mReaderLock.Release();
}

void RWSpinLock::ReleaseWrite()
{
    RWRelease(mGlobal, WRITE_LOCK);
}

} // namespace lf
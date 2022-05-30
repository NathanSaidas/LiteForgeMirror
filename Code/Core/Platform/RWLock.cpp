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
#include "Core/PCH.h"
#include "RWLock.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Utility/ErrorCore.h"

namespace lf {

static const Atomic32 SPIN_LOCK_UNLOCKED = 0x7FFFFFFF;
static const SizeT DEFAULT_SPIN_COUNT = 1000;
static const Atomic32 READ_LOCK = 1;
static const Atomic32 WRITE_LOCK = 2;

void RWAcquire(volatile Atomic32& lock, Atomic32 value)
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
        state = AtomicCompareExchange(&lock, value, SPIN_LOCK_UNLOCKED);
        --spin;
    } while (state != SPIN_LOCK_UNLOCKED);
}

void RWRelease(volatile Atomic32& lock, Atomic32 value)
{
    Atomic32 state = AtomicCompareExchange(&lock, SPIN_LOCK_UNLOCKED, value);
    AssertEx(state == value, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips someone has corrupted the ownership of this spin lock, or you're attemping to release without acquiring.
}

RWLock::RWLock()
: mGlobal(SPIN_LOCK_UNLOCKED)
, mReaderCount(0)
, mReaderLock()
, mWriterCount(0)
, mReaderFence()
{
    Assert(mReaderFence.Initialize());
}

RWLock::~RWLock()
{
    mReaderFence.Destroy();
}

bool RWLock::TryAcquireRead()
{
    if (AtomicLoad(&mWriterCount) > 0)
    {
        return false;
    }
    AcquireRead();
    return true;
}

bool RWLock::TryAcquireWrite()
{
    if (AtomicLoad(&mWriterCount) > 0 || AtomicLoad(&mReaderCount) > 0)
    {
        return false;
    }
    AcquireWrite();
    return true;
}

void RWLock::AcquireRead()
{
    // Wait before trying to lock
    ThreadFence::WaitStatus status;
    do
    {
        status = mReaderFence.Wait(1);
        Assert(status != ThreadFence::WS_FAILED);
    } while (status != ThreadFence::WS_SUCCESS && AtomicLoad(&mWriterCount) != 0);

    // Acquire
    mReaderLock.Acquire();
    Atomic32 readers = AtomicIncrement32(&mReaderCount);
    Assert(readers >= 0);
    if (readers == 1)
    {
        RWAcquire(mGlobal, READ_LOCK);
    }
    mReaderLock.Release();
}

void RWLock::AcquireWrite()
{
    Atomic32 writers = AtomicIncrement32(&mWriterCount);
    Assert(writers >= 0);
    if (writers == 1)
    {
        Assert(mReaderFence.Set(true));
    }
    RWAcquire(mGlobal, WRITE_LOCK);
}
void RWLock::ReleaseRead()
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
void RWLock::ReleaseWrite()
{
    RWRelease(mGlobal, WRITE_LOCK);
    Atomic32 writers = AtomicDecrement32(&mWriterCount);
    Assert(writers >= 0);
    if (writers == 0)
    {
        Assert(mReaderFence.Set(false));
    }
}

bool RWLock::IsWrite() const
{
    return AtomicLoad(&mWriterCount) > 0;
}
bool RWLock::IsRead() const
{
    return AtomicLoad(&mReaderCount) > 0;
}

}
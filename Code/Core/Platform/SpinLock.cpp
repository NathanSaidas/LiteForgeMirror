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
#include "SpinLock.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include <intrin.h>

namespace lf {

static const long long SPIN_LOCK_UNLOCKED = 0x7FFFFFFFFFFFFFFF;
static const SizeT DEFAULT_SPIN_COUNT = 1000;

SpinLock::SpinLock() : 
mOwner(SPIN_LOCK_UNLOCKED)
{
}

SpinLock::~SpinLock()
{
    // If this trips we didn't release the lock! Possible dead lock ahead.
    CriticalAssertEx(_InterlockedCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, SPIN_LOCK_UNLOCKED) == SPIN_LOCK_UNLOCKED, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

void SpinLock::Acquire()
{
    SizeT spin = DEFAULT_SPIN_COUNT;
    long long state;
    long long id = static_cast<long long>(GetCallingThreadId());
    do
    {
        if (spin == 0)
        {
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
        }
        state = _InterlockedCompareExchange64(&mOwner, id, SPIN_LOCK_UNLOCKED);
        --spin;
        if (state == id)
        {
            LF_ERROR_BREAK;
        }
        // AssertEx(state != id, LF_ERROR_DEADLOCK, ERROR_API_CORE); // If this trips were in deadlock
    } while (state != SPIN_LOCK_UNLOCKED);
}
bool SpinLock::TryAcquire()
{
    long long id = static_cast<long long>(GetCallingThreadId());
    long long state = _InterlockedCompareExchange64(&mOwner, id, SPIN_LOCK_UNLOCKED);
    return state == SPIN_LOCK_UNLOCKED;
}
bool SpinLock::TryAcquire(SizeT milliseconds)
{
    if (milliseconds == 0)
    {
        return TryAcquire();
    }

    SizeT spin = DEFAULT_SPIN_COUNT;
    long long state;
    long long id = static_cast<long long>(GetCallingThreadId());
    do
    {
        if (spin == 0)
        {
            if (milliseconds == 0)
            {
                return false;
            }
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
            --milliseconds;
        }
        state = _InterlockedCompareExchange64(&mOwner, id, SPIN_LOCK_UNLOCKED);
        --spin;
        AssertEx(state != id, LF_ERROR_DEADLOCK, ERROR_API_CORE); // If this trips were in deadlock
    } while (state != SPIN_LOCK_UNLOCKED);
    return state == SPIN_LOCK_UNLOCKED;
}
void SpinLock::Release()
{
    long long id = static_cast<long long>(GetCallingThreadId());
    long long state = _InterlockedCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, id);
    AssertEx(state == id, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips someone has corrupted the ownership of this spin lock, or you're attemping to release without acquiring.
}

MemorySpinLock::MemorySpinLock()
: mOwner(nullptr)
{

}
MemorySpinLock::~MemorySpinLock()
{
    // If this trips we didn't release the lock! Possible dead lock ahead.
    CriticalAssertEx(AtomicCompareExchangePointer<void>(&mOwner, nullptr, nullptr) == nullptr, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}
void MemorySpinLock::Acquire(void* owner)
{
    SizeT spin = DEFAULT_SPIN_COUNT;
    void* state;
    do
    {
        if (spin == 0)
        {
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
        }
        state = AtomicCompareExchangePointer<void>(&mOwner, owner, nullptr);
        --spin;
        if (state == owner)
        {
            // If this trips were in deadlock
            LF_ERROR_BREAK;
        }
    } while (state != nullptr);
}
bool MemorySpinLock::TryAcquire(void* owner)
{
    void* state = AtomicCompareExchangePointer<void>(&mOwner, owner, nullptr);
    return state == nullptr;
}
bool MemorySpinLock::TryAcquire(void* owner, SizeT milliseconds)
{
    if (milliseconds == 0)
    {
        return TryAcquire(owner);
    }

    SizeT spin = DEFAULT_SPIN_COUNT;
    void* state;
    do
    {
        if (spin == 0)
        {
            if (milliseconds == 0)
            {
                return false;
            }
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
            --milliseconds;
        }
        state = AtomicCompareExchangePointer<void>(&mOwner, owner, nullptr);
        --spin;
        AssertEx(state != owner, LF_ERROR_DEADLOCK, ERROR_API_CORE);
    } while (state != nullptr);
    return state == nullptr;
}
void MemorySpinLock::Release(void* owner)
{
    void* state = AtomicCompareExchangePointer<void>(&mOwner, nullptr, owner);
    AssertEx(state == owner, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips someone has corrupted the ownership of this spin lock, or you're attemping to release without acquiring.
}
bool MemorySpinLock::IsOwner(void* owner) const
{
    return mOwner == owner;
}
bool MemorySpinLock::IsOwned() const
{
    return mOwner != nullptr;
}

MultiSpinLock::MultiSpinLock()
: mOwner(SPIN_LOCK_UNLOCKED)
, mInternalLock(SPIN_LOCK_UNLOCKED)
, mRefs(0)
{}

MultiSpinLock::~MultiSpinLock()
{
    // If this trips we didn't release the lock! Possible dead lock ahead.
    CriticalAssertEx(_InterlockedCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, SPIN_LOCK_UNLOCKED) == SPIN_LOCK_UNLOCKED, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    CriticalAssertEx(_InterlockedCompareExchange64(&mRefs, 0, 0) == 0, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

void MultiSpinLock::Acquire()
{
    SizeT spin = DEFAULT_SPIN_COUNT;
    Atomic64 state;
    Atomic64 id = static_cast<long long>(GetCallingThreadId());
    Atomic64 internalState;
    bool owned = false;
    do
    {
        if (spin == 0)
        {
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
        }
        internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
        if (internalState == SPIN_LOCK_UNLOCKED)
        {
            state = AtomicCompareExchange64(&mOwner, id, SPIN_LOCK_UNLOCKED);
            owned = state == SPIN_LOCK_UNLOCKED || state == id;
            if (owned)
            {
                AssertEx(AtomicIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
            }
            internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
            AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips someone has corrupted the internal lock
        }
        --spin;
    } while (!owned);
}

bool MultiSpinLock::TryAcquire()
{
    Atomic64 state;
    Atomic64 id = static_cast<long long>(GetCallingThreadId());
    Atomic64 internalState;
    bool owned = false;

    internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
    if (internalState == SPIN_LOCK_UNLOCKED)
    {
        state = AtomicCompareExchange64(&mOwner, id, SPIN_LOCK_UNLOCKED);
        owned = state == SPIN_LOCK_UNLOCKED || state == id;
        if (owned)
        {
            AssertEx(AtomicIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
        }
        internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
        AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips someone has corrupted the internal lock
    }
    return true;
}

bool MultiSpinLock::TryAcquire(SizeT milliseconds)
{
    if (milliseconds == 0)
    {
        return TryAcquire();
    }

    SizeT spin = DEFAULT_SPIN_COUNT;
    Atomic64 state;
    Atomic64 id = static_cast<long long>(GetCallingThreadId());
    Atomic64 internalState;
    bool owned = false;

    do
    {
        if (spin == 0)
        {
            if (milliseconds == 0)
            {
                return false;
            }
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
            --milliseconds;
        }

        internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
        if (internalState == SPIN_LOCK_UNLOCKED)
        {
            state = AtomicCompareExchange64(&mOwner, id, SPIN_LOCK_UNLOCKED);
            owned = state == SPIN_LOCK_UNLOCKED || state == id;
            if (owned)
            {
                AssertEx(AtomicIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
            }
            internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
            AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips someone has corrupted the internal lock
        }

    } while (!owned);
    return owned;
}

void MultiSpinLock::Release()
{
    Atomic64 id = static_cast<Atomic64>(GetCallingThreadId());
    Atomic64 internalState;
    bool success = false;
    do
    {
        internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
        if (internalState == SPIN_LOCK_UNLOCKED)
        {
            Atomic64 refs = AtomicDecrement64(&mRefs);
            AssertEx(refs >= 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
            if (refs == 0)
            {
                success = AtomicCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, id) == id;
            }
            else
            {
                success = true;
            }
            AssertEx(success, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips you're trying to Release a lock not owned by you.
            internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
            AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE);
        }
    } while (!success);
}

MemoryMultiSpinLock::MemoryMultiSpinLock()
: mOwner(nullptr)
, mInternalLock(SPIN_LOCK_UNLOCKED)
, mRefs(0)
{}
MemoryMultiSpinLock::~MemoryMultiSpinLock()
{
    // If this trips we didn't release the lock! Possible dead lock ahead.
    CriticalAssertEx(AtomicCompareExchangePointer<void>(&mOwner, nullptr, nullptr) == nullptr, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    CriticalAssertEx(AtomicCompareExchange64(&mRefs, 0, 0) == 0, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

void MemoryMultiSpinLock::Acquire(void* owner)
{
    AssertEx(owner != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    
    // lock(internal_lock)
    //      try_assign(external_lock);
    //      inc_ref_if_success(refs);
    // unlock(internal_lock);
    Atomic64 id = static_cast<Atomic64>(GetCallingThreadId());
    Atomic64 internalState;
    SizeT spin = DEFAULT_SPIN_COUNT;
    void* state;
    bool owned = false;
    do
    {
        if (spin == 0)
        {
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
        }

        internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
        if (internalState == SPIN_LOCK_UNLOCKED)
        {
            state = AtomicCompareExchangePointer<void>(&mOwner, owner, nullptr);
            owned = state == nullptr || state == owner;
            if (owned)
            {
                AssertEx(AtomicIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
            }
            internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
            AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips someone has corrupted the internal lock
        }
        --spin;
    } while (!owned);
    
}
bool MemoryMultiSpinLock::TryAcquire(void* owner)
{
    AssertEx(owner != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    
    Atomic64 id = static_cast<Atomic64>(GetCallingThreadId());
    Atomic64 internalState;
    void* state;
    bool owned = false;

    internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
    if (internalState == SPIN_LOCK_UNLOCKED)
    {
        state = AtomicCompareExchangePointer<void>(&mOwner, owner, nullptr);
        owned = state == nullptr || state == owner;
        if (owned)
        {
            AssertEx(AtomicIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
        }
        internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
        AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips someone has corrupted the internal lock
    }
    return owned;
}
bool MemoryMultiSpinLock::TryAcquire(void* owner, SizeT milliseconds)
{
    if (milliseconds == 0)
    {
        return TryAcquire(owner);
    }
    AssertEx(owner != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);

    Atomic64 id = static_cast<Atomic64>(GetCallingThreadId());
    Atomic64 internalState;
    SizeT spin = DEFAULT_SPIN_COUNT;
    void* state;
    bool owned = false;
    do
    {
        if (spin == 0)
        {
            if (milliseconds == 0)
            {
                return false;
            }
            spin = DEFAULT_SPIN_COUNT;
            SleepCallingThread(1);
            --milliseconds;
        }

        internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
        if (internalState == SPIN_LOCK_UNLOCKED)
        {
            state = AtomicCompareExchangePointer<void>(&mOwner, owner, nullptr);
            owned = state == nullptr || state == owner;
            if (owned)
            {
                AssertEx(AtomicIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
            }
            internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
            AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE);
        }
        --spin;
    } while (!owned);
    return true;
}
void MemoryMultiSpinLock::Release(void* owner)
{
    AssertEx(owner != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    Atomic64 id = static_cast<Atomic64>(GetCallingThreadId());
    Atomic64 internalState;
    bool success = false;
    do
    {
        internalState = AtomicCompareExchange64(&mInternalLock, id, SPIN_LOCK_UNLOCKED);
        if (internalState == SPIN_LOCK_UNLOCKED)
        {
            success = AtomicCompareExchangePointer<void>(&mOwner, nullptr, owner) == owner;
            AssertEx(success, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips you're trying to Release a lock not owned by you.
            AssertEx(AtomicDecrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
            internalState = AtomicCompareExchange64(&mInternalLock, SPIN_LOCK_UNLOCKED, id);
            AssertEx(internalState == id, LF_ERROR_BAD_STATE, ERROR_API_CORE);
        }
    } while (!success);
}
bool MemoryMultiSpinLock::IsOwner(void* owner) const
{
    AssertEx(owner != nullptr, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    return AtomicLoadPointer(&mOwner) == owner;
}
bool MemoryMultiSpinLock::IsOwned() const
{
    return AtomicLoadPointer(&mOwner) != nullptr;
}

}
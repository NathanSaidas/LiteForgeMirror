#include "SpinLock.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
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
    AssertError(_InterlockedCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, SPIN_LOCK_UNLOCKED) == SPIN_LOCK_UNLOCKED, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
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
        AssertError(state != id, LF_ERROR_DEADLOCK, ERROR_API_CORE); // If this trips were in deadlock
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
        AssertError(state != id, LF_ERROR_DEADLOCK, ERROR_API_CORE); // If this trips were in deadlock
    } while (state != SPIN_LOCK_UNLOCKED);
    return state == SPIN_LOCK_UNLOCKED;
}
void SpinLock::Release()
{
    long long id = static_cast<long long>(GetCallingThreadId());
    long long state = _InterlockedCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, id);
    AssertError(state == id, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips someone has corrupted the ownership of this spin lock, or you're attemping to release without acquiring.
}

MultiSpinLock::MultiSpinLock() :
    mOwner(SPIN_LOCK_UNLOCKED),
    mRefs(0)
{}

MultiSpinLock::~MultiSpinLock()
{
    // If this trips we didn't release the lock! Possible dead lock ahead.
    AssertError(_InterlockedCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, SPIN_LOCK_UNLOCKED) == SPIN_LOCK_UNLOCKED, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    AssertError(_InterlockedCompareExchange64(&mRefs, 0, 0) == 0, LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

void MultiSpinLock::Acquire()
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
    } while (state != SPIN_LOCK_UNLOCKED && state != id);
    AssertError(_InterlockedIncrement64(&mRefs) > 0, LF_ERROR_DEADLOCK, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
}

bool MultiSpinLock::TryAcquire()
{
    long long id = static_cast<long long>(GetCallingThreadId());
    long long state = _InterlockedCompareExchange64(&mOwner, id, SPIN_LOCK_UNLOCKED);
    bool owned = state == SPIN_LOCK_UNLOCKED || state == id;
    if (owned)
    {
        AssertError(_InterlockedIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
    }
    return owned;
}

bool MultiSpinLock::TryAcquire(SizeT milliseconds)
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

    } while (state != SPIN_LOCK_UNLOCKED && state != id);

    bool owned = state == SPIN_LOCK_UNLOCKED || state == id;
    if (owned)
    {
        AssertError(_InterlockedIncrement64(&mRefs) > 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // If this trips were not keeping track of refs correctly
    }
    return owned;
}

void MultiSpinLock::Release()
{
    long long id = static_cast<long long>(GetCallingThreadId());
    long long ref = _InterlockedDecrement64(&mRefs);
    AssertError(ref >= 0, LF_ERROR_BAD_STATE, ERROR_API_CORE);
    if (ref == 0)
    {
        long long state = _InterlockedCompareExchange64(&mOwner, SPIN_LOCK_UNLOCKED, id);
        AssertError(state == id, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE); // If this trips someone has corrupted the ownership of this spin lock, or you're attemping to release without acquiring.
    }
}

}
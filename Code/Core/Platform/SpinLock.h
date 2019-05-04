#ifndef LF_CORE_SPIN_LOCK_H
#define LF_CORE_SPIN_LOCK_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

// Light-weight spin lock, not very good in high contention
// Non-recursive so you can only lock it once!
class LF_CORE_API SpinLock
{
public:
    SpinLock();
    SpinLock(const SpinLock&) = delete;
    SpinLock(SpinLock&&) = delete;
    ~SpinLock();

    SpinLock& operator=(const SpinLock&) = delete;
    SpinLock& operator=(SpinLock&&) = delete;

    // Blocks execution on the current thread until the lock acquires ownership
    void Acquire();
    // Attempts to claim-ownership, returns true if the current thread has ownership
    bool TryAcquire();
    // Attempts to claim-ownership and waits until it has ownership or until `milliseconds` has
    // expired
    bool TryAcquire(SizeT milliseconds);
    // Releases ownership from the current thread. Must acquire first!
    void Release();
private:
    volatile long long mOwner; // Owning ThreadId
};

// Light-weight spin lock, not very good in high contention
// Recursive so you can only lock it multiple times in a single thread
class LF_CORE_API MultiSpinLock
{
public:
    MultiSpinLock();
    MultiSpinLock(const SpinLock&) = delete;
    MultiSpinLock(SpinLock&&) = delete;
    ~MultiSpinLock();

    MultiSpinLock& operator=(const MultiSpinLock&) = delete;
    MultiSpinLock& operator=(MultiSpinLock&&) = delete;

    // Blocks execution on the current thread until the lock acquires ownership
    void Acquire();
    // Attempts to claim-ownership, returns true if the current thread has ownership
    bool TryAcquire();
    // Attempts to claim-ownership and waits until it has ownership or until `milliseconds` has
    // expired
    bool TryAcquire(SizeT milliseconds);
    // Releases ownership from the current thread. Must acquire first!
    void Release();
private:
    volatile long long mOwner; // Owning ThreadId
    volatile long long mRefs; // Thread Ownership Ref-Count
};

// Utilities to safely lock/unlock SpinLocks in a scope.
// eg. ScopeLock lock(mMyLock);
struct LF_CORE_API ScopeLock
{
public:
    ScopeLock(SpinLock& lock) : mLock(lock) { mLock.Acquire(); }
    ~ScopeLock() { mLock.Release(); }
private:
    SpinLock& mLock;
};

// eg. ScopeTryLock lock(mMyLock);
// if(lock.isLocked()) { ... }
struct LF_CORE_API ScopeTryLock
{
public:
    ScopeTryLock(SpinLock& lock) : mLock(lock), mIsLocked(false) { mIsLocked = mLock.TryAcquire(); }
    ScopeTryLock(SpinLock& lock, SizeT waitMilliseconds) : mLock(lock), mIsLocked(false) { mIsLocked = mLock.TryAcquire(waitMilliseconds); }
    ~ScopeTryLock() { if (mIsLocked) { mLock.Release(); } }
    bool IsLocked() const { return mIsLocked; }
private:
    SpinLock& mLock;
    bool mIsLocked;
};

// eg. ScopeMultiLock lock(mMyLock);
struct LF_CORE_API ScopeMultiLock
{
public:
    ScopeMultiLock(MultiSpinLock& lock) : mLock(lock) { mLock.Acquire(); }
    ~ScopeMultiLock() { mLock.Release(); }
private:
    MultiSpinLock& mLock;
};

// eg. ScopeTryMultiLock lock(mMyLock);
// if(lock.isLocked()) { ... }
struct LF_CORE_API ScopeTryMultiLock
{
public:
    ScopeTryMultiLock(MultiSpinLock& lock) : mLock(lock), mIsLocked(false) { mIsLocked = mLock.TryAcquire(); }
    ScopeTryMultiLock(MultiSpinLock& lock, SizeT waitMilliseconds) : mLock(lock), mIsLocked(false) { mIsLocked = mLock.TryAcquire(waitMilliseconds); }
    ~ScopeTryMultiLock() { if (mIsLocked) { mLock.Release(); } }
    bool IsLocked() const { return mIsLocked; }
private:
    MultiSpinLock& mLock;
    bool mIsLocked;
};

}

#endif // LF_CORE_SPIN_LOCK_H
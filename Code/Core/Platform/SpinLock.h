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
#pragma once

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
// Non-recursive so you can only lock it once!
// Locks on a specific memory address rather than thread id
class LF_CORE_API MemorySpinLock
{
public:
    MemorySpinLock();
    MemorySpinLock(const MemorySpinLock&) = delete;
    MemorySpinLock(MemorySpinLock&&) = delete;
    ~MemorySpinLock();

    MemorySpinLock& operator=(const MemorySpinLock&) = delete;
    MemorySpinLock& operator=(MemorySpinLock&&) = delete;

    void Acquire(void* owner);
    bool TryAcquire(void* owner);
    bool TryAcquire(void* owner, SizeT milliseconds);
    void Release(void* owner);
    bool IsOwner(void* owner) const;
    bool IsOwned() const;
private:
    void* volatile mOwner;

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
    volatile Atomic64 mOwner; // Owning ThreadId
    volatile Atomic64 mInternalLock;
    volatile Atomic64 mRefs; // Thread Ownership Ref-Count
};

// Light-weight spin lock, not very good in high contention
// Recursive so you can only lock it multiple times
// Locks on a specific memory address rather than thread id
class LF_CORE_API MemoryMultiSpinLock
{
public:
    MemoryMultiSpinLock();
    MemoryMultiSpinLock(const MemoryMultiSpinLock&) = delete;
    MemoryMultiSpinLock(MemoryMultiSpinLock&&) = delete;
    ~MemoryMultiSpinLock();

    MemoryMultiSpinLock& operator=(const MemoryMultiSpinLock&) = delete;
    MemoryMultiSpinLock& operator=(MemoryMultiSpinLock&&) = delete;

    void Acquire(void* owner);
    bool TryAcquire(void* owner);
    bool TryAcquire(void* owner, SizeT milliseconds);
    void Release(void* owner);
    bool IsOwner(void* owner) const;
    bool IsOwned() const;
private:
    void* volatile     mOwner;
    volatile Atomic64 mInternalLock;
    volatile Atomic64 mRefs; // Thread Ownership Ref-Count
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
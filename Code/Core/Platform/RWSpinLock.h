// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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

#ifndef LF_CORE_RW_SPIN_LOCK_H
#define LF_CORE_RW_SPIN_LOCK_H

#include "Core/Platform/SpinLock.h"

namespace lf {

// A reader/writer lock implementation based off Raynal https://en.wikipedia.org/wiki/Readers%E2%80%93writer_lock
// using our already established spin locks. 
// 
// This allows multiple readers but only one writer.
// 
// note: Do not use in high contention!
class LF_CORE_API RWSpinLock
{
public:
    RWSpinLock();
    RWSpinLock(const RWSpinLock&) = delete;
    RWSpinLock(RWSpinLock&&) = delete;
    ~RWSpinLock();

    RWSpinLock& operator=(const SpinLock&) = delete;
    RWSpinLock& operator=(SpinLock&&) = delete;

    void AcquireRead();
    void AcquireWrite();

    void ReleaseRead();
    void ReleaseWrite();
private:
    // SpinLock mGlobal;
    volatile Atomic32 mGlobal;
    SpinLock mReaderLock;

    volatile Atomic32 mReaderCount;

};

struct LF_CORE_API ScopeRWLockRead
{
    ScopeRWLockRead(RWSpinLock& lock) : mLock(lock) { mLock.AcquireRead(); }
    ~ScopeRWLockRead() { mLock.ReleaseRead(); }
private:
    RWSpinLock& mLock;
};

struct LF_CORE_API ScopeRWLockWrite
{
    ScopeRWLockWrite(RWSpinLock& lock) : mLock(lock) { mLock.AcquireWrite(); }
    ~ScopeRWLockWrite() { mLock.ReleaseWrite(); }
private:
    RWSpinLock& mLock;
};

} // namespace lf

#endif // LF_CORE_RW_SPIN_LOCK_H
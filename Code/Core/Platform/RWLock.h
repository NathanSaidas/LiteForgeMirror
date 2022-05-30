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

namespace lf {

// **********************************
// This is a synchronization primitive similar to RWSpinLock
// it provides the functionality of having multiple readers
// and a single writer however it also gives writers priority
// over the lock such that if a writer attempts to acquire the
// lock reader will wait until the writer has acquired and released
// the lock.
// **********************************
class LF_CORE_API RWLock
{
public:
    RWLock();
    RWLock(const RWLock&) = delete;
    RWLock(RWLock&&) = delete;
    ~RWLock();

    RWLock& operator=(const RWLock&) = delete;
    RWLock& operator=(RWLock&&) = delete;

    bool TryAcquireRead();
    bool TryAcquireWrite();

    void AcquireRead();
    void AcquireWrite();

    void ReleaseRead();
    void ReleaseWrite();

    bool IsWrite() const;
    bool IsRead() const;
private:
    // ** This is the atomic readers/writers lock on to ensure the reader/writer/null state.
    volatile Atomic32 mGlobal;
    // ** This is the atomic we use to count the number of readers, the first reader locks and last reader unlocks
    volatile Atomic32 mReaderCount;
    // ** This is a lock we use to synchronize reader access to the global lock
    SpinLock mReaderLock;
    // ** This is the atomic we use to count the number of writers. Readers will wait on the fence before attempting to lock
    volatile Atomic32 mWriterCount;
    // ** This is the fence readers wait on for writers.
    ThreadFence mReaderFence;

};

struct ScopeRWLockRead
{
    LF_INLINE ScopeRWLockRead(RWLock& lock) : mLock(lock) { mLock.AcquireRead(); }
    LF_INLINE ~ScopeRWLockRead() { mLock.ReleaseRead(); }
private:
    RWLock& mLock;
};

struct ScopeRWLockWrite
{
    LF_INLINE ScopeRWLockWrite(RWLock& lock) : mLock(lock) { mLock.AcquireWrite(); }
    LF_INLINE ~ScopeRWLockWrite() { mLock.ReleaseWrite(); }
private:
    RWLock& mLock;
};

}
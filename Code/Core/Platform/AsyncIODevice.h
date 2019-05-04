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
#ifndef LF_CORE_ASYNC_IO_DEVICE_H
#define LF_CORE_ASYNC_IO_DEVICE_H

#include "Core/Common/Types.h"
#include "Core/Utility/Array.h"

#if defined(LF_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#endif

namespace lf {
#if defined(LF_OS_WINDOWS)
struct FileHandle;
class AsyncIOBuffer;

struct AsyncIOUserData : public OVERLAPPED
{
    AsyncIOUserData()
    {
        Internal = InternalHigh = 0;
        Offset = OffsetHigh = 0;
        hEvent = NULL;
        mHandle = INVALID_HANDLE_VALUE;
        mFileHandle = nullptr;
        mPendingBuffer = nullptr;
        mLastBytesRead = INVALID;
    }

    HANDLE                  mHandle;
    FileHandle*             mFileHandle;
    volatile AsyncIOBuffer* mPendingBuffer;
    volatile SizeT          mLastBytesRead;
};
#endif

class AsyncIODevice
{
public:
    AsyncIODevice();
    ~AsyncIODevice();

    bool Create(SizeT numThreads = 0);
    bool Close();

#if defined(LF_OS_WINDOWS)
    bool AssociateDevice(HANDLE device, void* userKey);
#endif

    bool QueuePacket(void* userKey, SizeT numBytes, void* userData);
    bool DequeuePacket(void*& userKey, SizeT& numBytes, void*& userData);
    bool TryDequeuePacket(void*& userKey, SizeT& numBytes, void*& userData, SizeT waitMilliseconds);

    bool IsRunning() const;
private:
#if defined(LF_OS_WINDOWS)
    using ThreadHandleArray = TStaticArray<HANDLE, 16>;
    HANDLE mHandle;
    ThreadHandleArray mThreads;
    volatile long mRunning;
#endif
    
};

} // namespace lf

#endif // LF_CORE_ASYNC_IO_DEVICE_H
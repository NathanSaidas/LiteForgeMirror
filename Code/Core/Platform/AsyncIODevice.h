// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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
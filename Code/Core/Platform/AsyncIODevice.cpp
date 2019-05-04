// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "AsyncIODevice.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Platform/AsyncIOBuffer.h"

#include <atomic>

namespace lf {

#if defined(LF_OS_WINDOWS)

DWORD WINAPI AsyncIOCompletionCallback(LPVOID param)
{
    AsyncIODevice* self = reinterpret_cast<AsyncIODevice*>(param);

    while (self->IsRunning())
    {
        void* userKey = nullptr;
        SizeT bytesTransferred = 0;
        void* userData = nullptr;
        if (self->TryDequeuePacket(userKey, bytesTransferred, userData, 1000))
        {
            AsyncIOUserData* ioUserData = reinterpret_cast<AsyncIOUserData*>(userData);
            if (ioUserData)
            {
                if (ioUserData->mPendingBuffer)
                {
                    AssertError(Valid(bytesTransferred), LF_ERROR_BAD_STATE, ERROR_API_CORE);
                    LARGE_INTEGER cursor;
                    cursor.QuadPart = bytesTransferred;
                    AssertError(SetFilePointerEx(ioUserData->mHandle, cursor, NULL, FILE_CURRENT) == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
                    ioUserData->mLastBytesRead = bytesTransferred;
                    ioUserData->mPendingBuffer->SetBytesTransferred(bytesTransferred);
                    ioUserData->mPendingBuffer->SetState(ASYNC_IO_DONE);
                    ioUserData->mPendingBuffer = nullptr;
                }
                _ReadWriteBarrier();
            }
            else
            {
                Crash("Unknown data in AsyncIO", LF_ERROR_INTERNAL, ERROR_API_CORE);
            }
        }
    }

    return 0;
}

#endif
  

AsyncIODevice::AsyncIODevice() : 
mHandle(NULL),
mThreads(),
mRunning(0)
{}
AsyncIODevice::~AsyncIODevice()
{
    AssertError(Close(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

bool AsyncIODevice::Create(SizeT numThreads)
{
#if defined(LF_OS_WINDOWS)
    AssertError(mHandle == nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    if (numThreads == 0)
    {
        SYSTEM_INFO sysInfo;
        GetNativeSystemInfo(&sysInfo);
        numThreads = sysInfo.dwNumberOfProcessors;
    }

    if (numThreads == 0)
    {
        return false;
    }
    _InterlockedExchange(&mRunning, 1);
    mThreads.Reserve(numThreads);
    for (SizeT i = 0; i < numThreads; ++i)
    {
        HANDLE thread = CreateThread(0, 0, AsyncIOCompletionCallback, this, 0, NULL);
        AssertError(thread != NULL, LF_ERROR_INTERNAL, ERROR_API_CORE);
        mThreads.Add(thread);
    }
    mHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, static_cast<DWORD>(numThreads));
    AssertError(mHandle != NULL, LF_ERROR_INTERNAL, ERROR_API_CORE);
    return true;
#endif
}
bool AsyncIODevice::Close()
{
#if defined(LF_OS_WINDOWS)
    if (mHandle == NULL)
    {
        return true;
    }

    _InterlockedExchange(&mRunning, 0);
    WaitForMultipleObjects(static_cast<DWORD>(mThreads.Size()), mThreads.GetData(), TRUE, INFINITE);
    for (SizeT i = 0; i < mThreads.Size(); ++i)
    {
        AssertError(CloseHandle(mThreads[i]), LF_ERROR_INTERNAL, ERROR_API_CORE);
    }
    mThreads.Clear();
    AssertError(CloseHandle(mHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
    mHandle = NULL;
#endif
    return true;
}

#if defined(LF_OS_WINDOWS)
bool AsyncIODevice::AssociateDevice(HANDLE device, void* userKey)
{
    if (mHandle == NULL)
    {
        return false;
    }
    _ReadWriteBarrier();
    if (mRunning == 0)
    {
        return false;
    }
    HANDLE handle = CreateIoCompletionPort(device, mHandle, reinterpret_cast<ULONG_PTR>(userKey), 0);
    return handle == mHandle;
}


bool AsyncIODevice::QueuePacket(void* userKey, SizeT numBytes, void* userData)
{
    return PostQueuedCompletionStatus(mHandle, static_cast<DWORD>(numBytes), reinterpret_cast<ULONG_PTR>(userKey), reinterpret_cast<LPOVERLAPPED>(userData)) == TRUE;
}
bool AsyncIODevice::DequeuePacket(void*& userKey, SizeT& numBytes, void*& userData)
{
    DWORD bytes = 0;
    ULONG_PTR key = 0;
    LPOVERLAPPED data = nullptr;
    BOOL  result = GetQueuedCompletionStatus(mHandle, &bytes, &key, &data, INFINITE);
    userKey = reinterpret_cast<void*>(key);
    userData = reinterpret_cast<void*>(data);
    numBytes = static_cast<SizeT>(bytes);
    return result == TRUE;
}
bool AsyncIODevice::TryDequeuePacket(void*& userKey, SizeT& numBytes, void*& userData, SizeT waitMilliseconds)
{
    DWORD bytes = 0;
    ULONG_PTR key = 0;
    LPOVERLAPPED data = nullptr;
    BOOL  result = GetQueuedCompletionStatus(mHandle, &bytes, &key, &data, static_cast<DWORD>(waitMilliseconds));
    userKey = reinterpret_cast<void*>(key);
    userData = reinterpret_cast<void*>(data);
    numBytes = static_cast<SizeT>(bytes);
    return result == TRUE;
}
#endif

bool AsyncIODevice::IsRunning() const
{
    _ReadWriteBarrier();
    return mRunning != 0;
}

} // namespace lf
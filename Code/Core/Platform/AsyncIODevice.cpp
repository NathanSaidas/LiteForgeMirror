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
                    AssertEx(Valid(bytesTransferred), LF_ERROR_BAD_STATE, ERROR_API_CORE);
                    LARGE_INTEGER cursor;
                    cursor.QuadPart = bytesTransferred;
                    AssertEx(SetFilePointerEx(ioUserData->mHandle, cursor, NULL, FILE_CURRENT) == TRUE, LF_ERROR_INTERNAL, ERROR_API_CORE);
                    ioUserData->mLastBytesRead = bytesTransferred;
                    ioUserData->mPendingBuffer->SetBytesTransferred(bytesTransferred);
                    ioUserData->mPendingBuffer->SetState(ASYNC_IO_DONE);
                    ioUserData->mPendingBuffer = nullptr;
                }
                _ReadWriteBarrier();
            }
            else
            {
                CriticalAssertMsgEx("Unknown data in AsyncIO", LF_ERROR_INTERNAL, ERROR_API_CORE);
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
    CriticalAssertEx(Close(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

bool AsyncIODevice::Create(SizeT numThreads)
{
#if defined(LF_OS_WINDOWS)
    AssertEx(mHandle == nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

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
        AssertEx(thread != NULL, LF_ERROR_INTERNAL, ERROR_API_CORE);
        mThreads.Add(thread);
    }
    mHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, static_cast<DWORD>(numThreads));
    AssertEx(mHandle != NULL, LF_ERROR_INTERNAL, ERROR_API_CORE);
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
        AssertEx(CloseHandle(mThreads[i]), LF_ERROR_INTERNAL, ERROR_API_CORE);
    }
    mThreads.Clear();
    AssertEx(CloseHandle(mHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
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
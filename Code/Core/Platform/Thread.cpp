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
#include "Thread.h"
#include "Core/Common/Assert.h"
#include "Core/Memory/Memory.h"
#include "Core/String/StringUtil.h"
#include "Core/Platform/Atomic.h"
#include "Core/Utility/ErrorCore.h"

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#ifdef Yield
#undef Yield
#endif
#endif

namespace lf {

static const SizeT THREAD_STACK_SIZE = (2ULL * 1024ULL * 1024ULL);
LF_THREAD_LOCAL bool        gIsMainThread = false;
LF_THREAD_LOCAL SizeT       gCurrentThreadId = 0;
LF_THREAD_LOCAL ThreadData* gCurrentThread = nullptr;

static Atomic32             gActiveThreads = 0;

struct ThreadData
{
    void*          mArgs;
    ThreadCallback mCallback;
    SizeT          mThreadId;
    volatile long  mRefCount;
#if defined(LF_OS_WINDOWS)
    HANDLE         mHandle;
#endif
#if defined(LF_DEBUG) || defined(LF_TEST)
    char*    mDebugName;
#endif
};

#if defined(LF_OS_WINDOWS)

DWORD WINAPI PlatformThreadCallback(LPVOID param)
{
    AtomicIncrement32(&gActiveThreads);
    ThreadData* thread = reinterpret_cast<ThreadData*>(param);
    gCurrentThread = thread;
    gCurrentThreadId = thread->mThreadId;
    if (thread->mCallback)
    {
        thread->mCallback(thread->mArgs);
    }
    gCurrentThread = nullptr;
    gCurrentThreadId = INVALID_THREAD_ID;
    AtomicDecrement32(&gActiveThreads);
    return 0;
}

// http ://stackoverflow.com/questions/10121560/stdthread-naming-your-thread
#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO
{
    DWORD dwType;
    LPCSTR szName;
    DWORD dwThreadID;
    DWORD dwFlags;
} THREADNAME_INFO;
#pragma pack(pop)

void PlatformSetThreadName(const char* name, SizeT threadId)
{
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = name;
    info.dwThreadID = static_cast<DWORD>(threadId);
    info.dwFlags = 0;

    const DWORD MS_VC_THREAD_NAME_EXCEPTION = 0x406D1388;

    __try
    {
        RaiseException(MS_VC_THREAD_NAME_EXCEPTION, 0, sizeof(info) / sizeof(ULONG_PTR), reinterpret_cast<ULONG_PTR*>(&info));
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {

    }
}
#endif
#if defined(LF_DEBUG) || defined(LF_TEST)
static void SetThreadDebugName(ThreadData* data, const char* name)
{
    if (data->mDebugName)
    {
        LFFree(data->mDebugName);
        data->mDebugName = nullptr;
    }
    SizeT length = StrLen(name);
    data->mDebugName = reinterpret_cast<char*>(LFAlloc(length + 1, 16));
    memcpy(data->mDebugName, name, length);
    data->mDebugName[length] = '\0';
    PlatformSetThreadName(name, data->mThreadId);
}
#endif


Thread::Thread() : 
mData(nullptr)
{
}
Thread::Thread(const Thread& other) :
mData(other.mData)
{
    AddRef();
}
Thread::Thread(Thread&& other) :
mData(other.mData)
{
    other.mData = nullptr;
}
Thread::~Thread()
{
    RemoveRef();
}

void Thread::Fork(ThreadCallback callback, void* data)
{
    AssertEx(IsMainThread(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(mData == nullptr, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    mData = LFNew<ThreadData>();
    AssertEx(mData, LF_ERROR_OUT_OF_MEMORY, ERROR_API_CORE);

    mData->mArgs = data;
    mData->mCallback = callback;
    mData->mRefCount = 1;
#if defined(LF_DEBUG) || defined(LF_TEST)
    mData->mDebugName = nullptr;
#endif

#if defined(LF_OS_WINDOWS)
    DWORD threadId = 0;
    mData->mHandle = CreateThread(NULL, static_cast<SIZE_T>(THREAD_STACK_SIZE), PlatformThreadCallback, mData, 0, &threadId);
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
    mData->mThreadId = static_cast<SizeT>(threadId);
    AssertEx(mData->mHandle != NULL, LF_ERROR_INTERNAL, ERROR_API_CORE);
}
void Thread::Join()
{
    AssertEx(IsMainThread(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    AssertEx(mData && mData->mHandle, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
#if defined(LF_OS_WINDOWS)
    WaitForSingleObject(mData->mHandle, INFINITE);
    AssertEx(CloseHandle(mData->mHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
    mData->mHandle = 0;
    mData->mThreadId = INVALID_THREAD_ID;
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif

#if defined(LF_DEBUG) || defined(LF_TEST)
    if (mData->mDebugName != nullptr)
    {
        LFFree(mData->mDebugName);
        mData->mDebugName = nullptr;
    }
#endif
    RemoveRef();
}

bool Thread::IsRunning() const
{
    return mData && mData->mHandle && WaitForSingleObject(mData->mHandle, 0) != WAIT_OBJECT_0;
}
SizeT Thread::GetRefs() const
{
    _ReadWriteBarrier();
    return mData ? mData->mRefCount : 0;
}
SizeT Thread::GetThreadId() const
{
    _ReadWriteBarrier();
    return mData ? mData->mThreadId : INVALID_THREAD_ID;
}

Thread& Thread::operator=(const Thread& other)
{
    if (this == &other)
    {
        return *this;
    }
    RemoveRef();
    mData = other.mData;
    AddRef();
    return *this;
}
Thread& Thread::operator=(Thread&& other)
{
    if (this == &other)
    {
        return *this;
    }
    RemoveRef();
    mData = other.mData;
    other.mData = nullptr;
    return *this;

}
bool Thread::operator==(const Thread& other)
{
    return mData == other.mData;
}
bool Thread::operator!=(const Thread& other)
{
    return mData != other.mData;
}

#if defined(LF_DEBUG) || defined(LF_TEST)
const char* Thread::GetDebugName() const
{
    return mData ? mData->mDebugName : "";
}

void Thread::SetDebugName(const char* name)
{
    if (!mData)
    {
        return;
    }

    SetThreadDebugName(mData, name);
}
#endif

void Thread::JoinAll(Thread* threadArray, const size_t numThreads)
{
    AssertEx(IsMainThread(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
#if defined(LF_OS_WINDOWS)
    const SizeT STACK_ARRAY_SIZE = 64;
    if (numThreads > STACK_ARRAY_SIZE)
    {
        HANDLE* handles = reinterpret_cast<HANDLE*>(LFAlloc(sizeof(HANDLE) * numThreads, alignof(HANDLE)));
        for (SizeT i = 0; i < numThreads; ++i)
        {
            AssertEx(threadArray[i].IsRunning(), LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
            handles[i] = threadArray[i].mData->mHandle;
        }
        WaitForMultipleObjects(static_cast<DWORD>(numThreads), handles, TRUE, INFINITE);
        for (SizeT i = 0; i < numThreads; ++i)
        {
            Thread& thread = threadArray[i];
            AssertEx(CloseHandle(thread.mData->mHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
            thread.mData->mHandle = nullptr;
            thread.mData->mThreadId = INVALID_THREAD_ID;
#if defined(LF_DEBUG) || defined(LF_TEST)
            if (thread.mData->mDebugName != nullptr)
            {
                LFFree(thread.mData->mDebugName);
                thread.mData->mDebugName = nullptr;
            }
#endif
            thread.RemoveRef();
        }
        LFFree(handles);
    }
    else
    {
        HANDLE handles[STACK_ARRAY_SIZE];
        for (SizeT i = 0; i < numThreads; ++i)
        {
            // AssertEx(threadArray[i].IsRunning(), LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
            handles[i] = threadArray[i].mData->mHandle;
        }
        WaitForMultipleObjects(static_cast<DWORD>(numThreads), handles, TRUE, INFINITE);
        for (SizeT i = 0; i < numThreads; ++i)
        {
            Thread& thread = threadArray[i];
            AssertEx(CloseHandle(thread.mData->mHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
            thread.mData->mHandle = nullptr;
            thread.mData->mThreadId = INVALID_THREAD_ID;
#if defined(LF_DEBUG) || defined(LF_TEST)
            if (thread.mData->mDebugName != nullptr)
            {
                LFFree(thread.mData->mDebugName);
                thread.mData->mDebugName = nullptr;
            }
#endif
            thread.RemoveRef();
        }
    }
#else 
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}

void Thread::Sleep(SizeT milliseconds)
{
#if defined(LF_OS_WINDOWS)
    ::Sleep(static_cast<DWORD>(milliseconds));
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}

void Thread::SleepPrecise(SizeT microseconds)
{
#if defined(LF_OS_WINDOWS)
    HANDLE timer;
    LARGE_INTEGER li;
    LONGLONG us = static_cast<LONGLONG>(microseconds);

    timer = CreateWaitableTimerA(nullptr, TRUE, nullptr);
    if (!timer)
    {
        return;
    }

    li.QuadPart = -us;
    if (!SetWaitableTimer(timer, &li, 0, nullptr, nullptr, FALSE))
    {
        CloseHandle(timer);
        return;
    }

    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
#else


    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}

void Thread::Yield()
{
#if defined(LF_OS_WINDOWS)
    SwitchToThread();
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}

SizeT Thread::GetId()
{
#if defined(LF_OS_WINDOWS)
    return static_cast<SizeT>(GetCurrentThreadId());
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}
SizeT Thread::GetExecutingCore()
{
#if defined(LF_OS_WINDOWS)
    return static_cast<SizeT>(GetCurrentProcessorNumber());
#else
    LF_STATIC_CRASH("Missing platform implementation.");
#endif
}

void Thread::AddRef()
{
    if (mData)
    {
        _InterlockedIncrement(&mData->mRefCount);
    }
}
void Thread::RemoveRef()
{
    if(mData)
    {
        if (_InterlockedDecrement(&mData->mRefCount) == 0)
        {
            AssertEx(IsMainThread(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
#if defined(LF_OS_WINDOWS)
            if (mData->mHandle != NULL)
            {
                WaitForSingleObject(mData->mHandle, INFINITE);
                AssertEx(CloseHandle(mData->mHandle), LF_ERROR_INTERNAL, ERROR_API_CORE);
                mData->mHandle = NULL;
                mData->mThreadId = INVALID_THREAD_ID;
            }
#else
            LF_STATIC_CRASH("Missing platform implementation.");
#endif

#if defined(LF_DEBUG) || defined(LF_TEST)
            if (mData->mDebugName != nullptr)
            {
                LFFree(mData->mDebugName);
                mData->mDebugName = nullptr;
            }
#endif
            LFDelete(mData);
            mData = nullptr;
        }
    }
}


SizeT GetPlatformThreadId()
{
    return static_cast<SizeT>(GetCurrentThreadId());
}

SizeT GetCallingThreadId()
{
    return gCurrentThreadId;
}
bool IsMainThread()
{
    return gIsMainThread;
}
void SleepCallingThread(SizeT milliseconds)
{
    Sleep(static_cast<DWORD>(milliseconds));
}
void SetMainThread()
{
    gIsMainThread = true;
    gCurrentThreadId = GetCurrentThreadId();
}

const char* GetThreadName()
{
#if defined(LF_DEBUG) || defined(LF_TEST)
    if (gCurrentThread)
    {
        return gCurrentThread->mDebugName ? gCurrentThread->mDebugName : "";
    }
    if (IsMainThread())
    {
        return "Main";
    }
#endif
    return "Unknown";
}

SizeT GetActiveThreadCount()
{
    return static_cast<SizeT>(AtomicLoad(&gActiveThreads));
}

void SetThreadName(const char* name)
{
#if defined(LF_DEBUG) || defined(LF_TEST)
    if (gCurrentThread != nullptr)
    {
        SetThreadDebugName(gCurrentThread, name);
    }
    else
    {
        PlatformSetThreadName(name, GetCurrentThreadId());
    }
#else
    (name);
#endif
}

} // namespace lf
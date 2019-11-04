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

#include "ThreadFence.h"
#include "Core/Memory/Memory.h"

#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

namespace lf {

struct ThreadFenceHandle
{
#if defined(LF_OS_WINDOWS)
    HANDLE   mEvent;
#endif
    volatile Atomic32 mRefs;
};

ThreadFence::ThreadFence()
: mHandle(nullptr)
{
}
ThreadFence::ThreadFence(const ThreadFence& other)
: mHandle(other.mHandle)
{
    AddRef();
}
ThreadFence::ThreadFence(ThreadFence&& other)
: mHandle(other.mHandle)
{
    other.mHandle = nullptr;
}
ThreadFence::~ThreadFence()
{
    RemoveRef();
}

ThreadFence& ThreadFence::operator=(const ThreadFence& other)
{
    if (this != &other)
    {
        mHandle = other.mHandle;
        AddRef();
    }
    return *this;
}
ThreadFence& ThreadFence::operator=(ThreadFence&& other)
{
    if (this != &other)
    {
        mHandle = other.mHandle;
        other.mHandle = nullptr;
    }
    return *this;
}

bool ThreadFence::Initialize()
{
#if defined(LF_OS_WINDOWS)
    if (mHandle)
    {
        return false;
    }

    HANDLE event = CreateEventA(NULL, TRUE, FALSE, NULL);;
    if (event == NULL)
    {
        return false;
    }

    mHandle = LFNew<ThreadFenceHandle>();
    mHandle->mRefs = 1;
    mHandle->mEvent = event;
    return true;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}
void ThreadFence::Destroy()
{
#if defined(LF_OS_WINDOWS)
    if (mHandle && mHandle->mEvent)
    {
        CloseHandle(mHandle->mEvent);
        mHandle->mEvent = NULL;
    }
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
    RemoveRef();
}
bool ThreadFence::Set(bool isBlocking)
{
#if defined(LF_OS_WINDOWS)
    if (mHandle && mHandle->mEvent)
    {
        if (isBlocking)
        {
            return ResetEvent(mHandle->mEvent) == TRUE;
        }
        else
        {
            return SetEvent(mHandle->mEvent) == TRUE;
        }
    }
    return false;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}

bool ThreadFence::Signal()
{
#if defined(LF_OS_WINDOWS)
    if (mHandle && mHandle->mEvent)
    {
        return PulseEvent(mHandle->mEvent) == TRUE;
    }
    return false;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}

ThreadFence::WaitStatus ThreadFence::Wait(SizeT milliseconds)
{
#if defined(LF_OS_WINDOWS)
    if (mHandle && mHandle->mEvent)
    {
        DWORD ms = Invalid(milliseconds) ? INFINITE : static_cast<DWORD>(milliseconds);
        switch (WaitForSingleObject(mHandle->mEvent, ms))
        {
            case WAIT_OBJECT_0: return WaitStatus::WS_SUCCESS;
            case WAIT_TIMEOUT: return WaitStatus::WS_TIMED_OUT;
            default: 
                break;
        }
    }
    return WaitStatus::WS_FAILED;
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}

void ThreadFence::AddRef()
{
    if (mHandle)
    {
#if defined(LF_OS_WINDOWS)
        _InterlockedIncrement(&mHandle->mRefs);
#endif
    }
}
void ThreadFence::RemoveRef()
{
#if defined(LF_OS_WINDOWS)
    if (mHandle)
    {
        if (_InterlockedDecrement(&mHandle->mRefs) == 0)
        {
            DestroyEvent();
        }
        mHandle = nullptr;
    }
#endif
}

void ThreadFence::DestroyEvent()
{
#if defined(LF_OS_WINDOWS)
    if (mHandle)
    {
        if (mHandle->mEvent)
        {
            CloseHandle(mHandle->mEvent);
            mHandle->mEvent = NULL;
        }

        if (_InterlockedCompareExchange(&mHandle->mRefs, 0, 0) == 0)
        {
            LFDelete(mHandle);
        }
        mHandle = nullptr;
    }
#else
    LF_STATIC_CRASH("Missing implementation.");
#endif
}

} // namespace lf
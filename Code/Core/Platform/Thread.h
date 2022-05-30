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

#include "Core/Common/API.h"
#include "Core/Common/Types.h"

namespace lf {

struct ThreadData;
using  ThreadCallback = void(*)(void*);

const SizeT INVALID_THREAD_ID = INVALID;

// todo: Might introduce these attribs later when we begin organizing/scheduling our threads
// struct ThreadAttributes
// {
//     UInt32 mThreadClass;
//     UInt32 mThreadClassID;
//     void*  mTaskScheduler;
//     void*  mTaskWorker;
// };

// Wrapper around platform specific thread functions
// Provides the most basic feature of starting a new execution
// context that may run on a seperate core.
class LF_CORE_API Thread
{
public:
    Thread();
    Thread(const Thread& other);
    Thread(Thread&& other);
    ~Thread();

    void Fork(ThreadCallback callback, void* data);
    void Join();

    bool IsRunning() const;
    SizeT GetRefs() const;
    SizeT GetThreadId() const;

    Thread& operator=(const Thread& other);
    Thread& operator=(Thread&& other);
    bool operator==(const Thread& other);
    bool operator!=(const Thread& other);

#if defined(LF_DEBUG) || defined(LF_TEST)
    const char* GetDebugName() const;
    void SetDebugName(const char* name);
#else
    void SetDebugName(const char*) {};
#endif
    
    static void JoinAll(Thread* threadArray, const size_t numThreads);
    static void  Sleep(SizeT milliseconds);
    // Sleep in ~100 microseconds.
    static void  SleepPrecise(SizeT microseconds);
    static void  Yield();
    static SizeT GetId();
    static SizeT GetExecutingCore();
private:
    void AddRef();
    void RemoveRef();

    ThreadData* mData;
};


LF_CORE_API SizeT GetPlatformThreadId();
LF_CORE_API SizeT GetCallingThreadId();
LF_CORE_API bool  IsMainThread();
LF_CORE_API void  SleepCallingThread(SizeT milliseconds);
LF_CORE_API void  SetMainThread();
LF_CORE_API const char* GetThreadName();
LF_CORE_API SizeT GetActiveThreadCount();
LF_CORE_API void SetThreadName(const char* name);

} // namespace lf
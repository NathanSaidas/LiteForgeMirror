#ifndef LF_CORE_THREAD_H
#define LF_CORE_THREAD_H

#include "Core/Common/API.h"
#include "Core/Common/Types.h"

namespace lf {

struct ThreadData;
using  ThreadCallback = void(*)(void*);

const SizeT INVALID_THREAD_ID = INVALID;

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

#if defined(LF_DEBUG)
    const char* GetDebugName() const;
    void SetDebugName(const char* name);
#endif
    
    static void JoinAll(Thread* threadArray, const size_t numThreads);
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

} // namespace lf

#endif // LF_CORE_THREAD_H
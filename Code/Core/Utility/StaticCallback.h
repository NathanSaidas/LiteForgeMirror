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

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {
// This files serves the purpose of enforcing static initialization to happen in a specific order.
// 
// Use STATIC_INIT for registering a for a callback


// **********************************
// Priority tags for static initialization, feel free to use them with +- Number
// 
// Example: 
//      STATIC_INIT(Foo, SCP_PRE_INIT_CORE)
//      STATIC_INIT(Bar, SCP_PRE_INIT_CORE + 500);
// 
// Bar is guaranteed to run after Foo.
// **********************************
enum StaticCallPriority
{
    SCP_PRE_INIT_CORE = 1000,
    SCP_PRE_INIT_RUNTIME = 2000,
    SCP_PRE_INIT_SERVICE = 3000,
    
    SCP_INIT_CORE = 5000,
    SCP_INIT_RUNTIME = 6000,
    SCP_INIT_SERVICE = 7000,
    SCP_INIT_ENGINE = 8000,
    
    SCP_POST_INIT = 10000
};


struct ProgramContext;

// Static called functions get called before main however order is not guaranteed to be careful!
using StaticCallback = void(*)();
using ProgramContextCallback = void(*)(ProgramContext*);
struct SafeStaticCallback;

LF_CORE_API extern SafeStaticCallback* gStaticInitCallbacks;
LF_CORE_API extern SizeT               gStaticInitCallbacksCount;
LF_CORE_API extern SafeStaticCallback* gStaticDestroyCallbacks;
LF_CORE_API extern SizeT               gStaticDestroyCallbacksCount;
LF_CORE_API void RegisterStaticInit(SafeStaticCallback* callback);
LF_CORE_API void RegisterStaticDestroy(SafeStaticCallback* callback);
LF_CORE_API void ExecuteStaticInit(SizeT maxPriority, ProgramContext* programContext);
LF_CORE_API void ExecuteStaticDestroy(SizeT minPriority, ProgramContext* programContext);
LF_CORE_API void StaticInitFence();
LF_CORE_API void StaticDestroyFence();

struct StaticCall
{
    StaticCall(StaticCallback cb) { cb(); }
};
#define STATIC_CALL(FunctionM) void FunctionM(); ::lf::StaticCall LF_ANONYMOUS_NAME(FunctionM)(FunctionM); void FunctionM() 

struct SafeStaticCallback
{
    enum InitTag { INIT };
    enum DestroyTag { DESTROY };

    SafeStaticCallback(ProgramContextCallback callback, SizeT priority, InitTag) :
        mCallback(callback),
        mPriority(priority),
        mNext(nullptr)
    {
        RegisterStaticInit(this);
    }

    SafeStaticCallback(ProgramContextCallback callback, SizeT priority, DestroyTag) :
        mCallback(callback),
        mPriority(priority),
        mNext(nullptr)
    {
        RegisterStaticDestroy(this);
    }

    bool operator<(const SafeStaticCallback& other) const
    {
        return mPriority < other.mPriority;
    }

    bool operator>(const SafeStaticCallback& other) const
    {
        return mPriority > other.mPriority;
    }

    ProgramContextCallback  mCallback;
    SizeT                   mPriority;
    SafeStaticCallback*     mNext;
};

#define STATIC_INIT(FunctionM, PriorityM)                                               \
    static void FunctionM(::lf::ProgramContext*);                                                                 \
    ::lf::SafeStaticCallback LF_ANONYMOUS_NAME(FunctionM)(FunctionM, PriorityM, ::lf::SafeStaticCallback::INIT);     \
    static void FunctionM(::lf::ProgramContext*)                                                                  \

#define STATIC_DESTROY(FunctionM, PriorityM)                                               \
    static void FunctionM(::lf::ProgramContext*);                                                                 \
    ::lf::SafeStaticCallback LF_ANONYMOUS_NAME(FunctionM)(FunctionM, PriorityM, ::lf::SafeStaticCallback::DESTROY);     \
    static void FunctionM(::lf::ProgramContext*)                                                                  \

} // namespace lf
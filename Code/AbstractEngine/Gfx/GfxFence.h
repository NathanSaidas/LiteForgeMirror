// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "Core/Platform/SpinLock.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadFence.h"
#include "Core/Reflection/Object.h"
#include "Core/Utility/SmartCallback.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "AbstractEngine/Gfx/GfxTypes.h"

namespace lf {

// how to use:
// 
// GfxDevice -- Create 3 fences (1 for each frame)
// GfxDevice -- Call StartThread(), this will kick off the thread an automatically dispatch callbacks
// GfxDevice -- [begin record], if we must wait for the cmdlist to complete, just call 'Wait'
// GfxDevice -- [set callback], fence->SetCallback( [ frameNum ]() { /* frameCompleteCallbackHere */ });
// GfxDevice -- [record cmd list]
// GfxDevice -- Context->Signal(fence)
// GfxDevice -- [shutdown], wait on call GfxFence, then call StopThread
class LF_ABSTRACT_ENGINE_API GfxFence : public Object
{
    DECLARE_CLASS(GfxFence, Object);
public:
    using FenceWaitCallback = TCallback<void>;

    GfxFence();
    ~GfxFence() override;

    void StartThread();
    void StopThread();

    void Wait();
    virtual UInt64 GetCompletedValue() const = 0;

    UInt64 NextValue()
    {
        AtomicRWBarrier();
        return ++mFenceValue;
    }

    UInt64 GetFenceValue() const { AtomicRWBarrier(); return mFenceValue; }
    void SetFenceValue(UInt64 value) { AtomicRWBarrier(); mFenceValue = value; }

    void SetCallback(const FenceWaitCallback& callback);
    void Signal();
protected:
    void WaitComplete();
    virtual void WaitImpl() = 0;

private:
    static void SignalThread(void* param);

    volatile UInt64 mFenceValue;
    Thread mSignalThread;
    ThreadFence mSignalThreadFence;
    SpinLock mWaitLock;
    FenceWaitCallback mWaitCallback;
    Atomic32 mThreadRunning;
};

} // namespace lf
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
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Platform/RWSpinLock.h"

namespace lf {

// Multi-producer, Single Consumer
class LF_ABSTRACT_ENGINE_API GfxTaskScheduler : public TaskSchedulerBase
{
    using SlotItemType = TaskTypes::TaskItemType;
    using SlotType = ConcurrentRingBufferSlot<SlotItemType>;
    using SlotTraits = TaskTypes::TaskRingBufferTraits;

    struct Buffer
    {
        TVector<SlotType> mAtomicSlots;
        TVector<SlotType> mLockSlots; // todo: We should use a 'SlabVector' to keep pointers alive

        Atomic32 mAtomicSize;
    };

    enum BufferType
    {
        Read,
        Write,

        MAX_VALUE,
        INVALID_VALUE
    };

    enum State
    {
        None,
        Running
    };

public:
    GfxTaskScheduler();

    // todo: Proper options type
    void Initialize(TaskScheduler::OptionsType options);
    void Shutdown();

    TaskHandle RunTask(TaskCallback func, void* param = nullptr) override;
    void Execute();

private:

    bool IsRunning() const { return AtomicLoad(&mState) == Running; }
    void SwapBuffers();
    void SwapIndex();

    SizeT WriteIndex() const { return static_cast<SizeT>(AtomicLoad(&mWriteIndex)); }
    SizeT ReadIndex() const { return static_cast<SizeT>(AtomicLoad(&mReadIndex)); }

    // ** Buffers to store our data
    Buffer mBuffers[ENUM_SIZE(BufferType)];
    // ** RW Lock ( Write Lock to swap buffers, Read Lock to write to buffers )
    RWSpinLock mReadWriteLock;
    // ** Lock for buffer-spill
    SpinLock mBufferWriteLock;

    volatile Atomic32 mWriteIndex;
    volatile Atomic32 mReadIndex;
    volatile Atomic32 mWriteSerial;
    volatile Atomic32 mState;

};

} // namespace lf
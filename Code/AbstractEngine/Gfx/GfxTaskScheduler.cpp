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
#include "AbstractEngine/PCH.h"
#include "GfxTaskScheduler.h"

namespace lf {

GfxTaskScheduler::GfxTaskScheduler()
    : TaskSchedulerBase()
    , mBuffers()
    , mReadWriteLock()
    , mBufferWriteLock()
    , mWriteIndex(BufferType::Write)
    , mReadIndex(BufferType::Read)
    , mWriteSerial(0)
{}

void GfxTaskScheduler::Initialize(TaskScheduler::OptionsType options)
{
    if (IsRunning())
    {
        return;
    }

    // Initialize our buffers...
    for (SizeT i = 0; i < ENUM_SIZE(BufferType); ++i)
    {
        Buffer& buffer = mBuffers[i];
        AtomicStore(&buffer.mAtomicSize, 0);
        buffer.mAtomicSlots.resize(options.mDispatcherSize);
        for (SlotType& slot : buffer.mAtomicSlots)
        {
            slot.mData.mCallback = TaskCallback();
            slot.mData.mParam = nullptr;
            AtomicStore(&slot.mSerial, mWriteSerial);
            AtomicStore(&slot.mState, CRBS_PRODUCER_READY);
        }
    }

    AtomicStore(&mState, Running);
}

void GfxTaskScheduler::Shutdown()
{
    if (!IsRunning())
    {
        return;
    }

    AtomicStore(&mState, None);
    Execute();
}

TaskHandle GfxTaskScheduler::RunTask(TaskCallback func, void* param)
{
    if (!func.IsValid())
    {
        return TaskHandle();
    }

    if (!IsRunning())
    {
        func.Invoke(param);
        return TaskHandle();
    }

    // ** Ensure we don't get swapped on
    ScopeRWSpinLockRead readLock(mReadWriteLock);
    Buffer& writeBuffer = mBuffers[WriteIndex()];
    TaskTypes::TaskRingBufferResult result;

    // ** Try to allocate to our lock-free array
    SizeT atomicIndex = AtomicIncrement32(&writeBuffer.mAtomicSize) - 1;
    if (atomicIndex < writeBuffer.mAtomicSlots.size())
    {
        SlotType& slot = writeBuffer.mAtomicSlots[atomicIndex];

        slot.mData.mCallback = std::move(func);
        slot.mData.mParam = param;
        Assert(CRBS_PRODUCER_READY == AtomicCompareExchange(&slot.mState, CRBS_CONSUMER_READY, CRBS_PRODUCER_READY));
        result.mData = SlotTraits::ToResultType(slot);
        result.mValid = true;
    }
    else // ** Allocate from our locking array
    {
        ScopeLock lock(mBufferWriteLock);
        writeBuffer.mLockSlots.push_back(SlotType());
        SlotType& slot = writeBuffer.mLockSlots.back();

        slot.mData.mCallback = std::move(func);
        slot.mData.mParam = param;
        Assert(CRBS_PRODUCER_READY == AtomicCompareExchange(&slot.mState, CRBS_CONSUMER_READY, CRBS_PRODUCER_READY));
        AtomicStore(&slot.mSerial, AtomicLoad(&mWriteSerial));
        result.mData = SlotTraits::ToResultType(slot);
        result.mValid = true;
    }

    return TaskHandle(result);
}



void GfxTaskScheduler::Execute()
{
    SwapBuffers();
    ScopeRWSpinLockRead readLock(mReadWriteLock);
    Buffer& readBuffer = mBuffers[ReadIndex()];
    for (SizeT i = 0, size = static_cast<SizeT>(AtomicLoad(&readBuffer.mAtomicSize)); i < size; ++i)
    {
        SlotType& slot = readBuffer.mAtomicSlots[i];
        Assert(AtomicLoad(&slot.mState) == CRBS_CONSUMER_READY);
        if (slot.mData.mCallback.IsValid())
        {
            slot.mData.mCallback.Invoke(slot.mData.mParam);
        }
    }

    for (SlotType& slot : readBuffer.mLockSlots)
    {
        Assert(AtomicLoad(&slot.mState) == CRBS_CONSUMER_READY);
        if (slot.mData.mCallback.IsValid())
        {
            slot.mData.mCallback.Invoke(slot.mData.mParam);
        }
    }
}

void GfxTaskScheduler::SwapBuffers()
{
    ScopeRWSpinLockWrite writeLock(mReadWriteLock);
    AtomicIncrement32(&mWriteSerial);

    // Clear our buffer
    Buffer& readBuffer = mBuffers[ReadIndex()];
    readBuffer.mLockSlots.resize(0);
    for (SizeT i = 0, size = readBuffer.mAtomicSlots.size(); i < size; ++i)
    {
        SlotType& slot = readBuffer.mAtomicSlots[i];
        slot.mData = TaskItem();
        slot.mSerial = mWriteSerial;
        AtomicStore(&slot.mState, CRBS_PRODUCER_READY);
    }
    AtomicStore(&readBuffer.mAtomicSize, 0);

    // Swap buffers..
    SwapIndex();
}

void GfxTaskScheduler::SwapIndex()
{
    Atomic32 tmp = AtomicLoad(&mWriteIndex);
    AtomicStore(&mWriteIndex, AtomicLoad(&mReadIndex));
    AtomicStore(&mReadIndex, tmp);
}

} // namespace lf
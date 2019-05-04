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
#ifndef LF_CORE_CONCURRENT_RING_BUFFER_H
#define LF_CORE_CONCURRENT_RING_BUFFER_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/Atomic.h"
#include "Core/Utility/Array.h"

namespace lf {

template<typename T>
struct ConcurrentRingBufferWorkValueResult
{
    using WorkItem = T;
    operator bool() const { return mValid; }
    WorkItem            mData;
    bool                mValid;
};

template<typename T>
struct ConcurrentRingBufferWorkPointerResult
{
    using WorkItem = T;
    operator bool() const { return mValid; }
    WorkItem*           mData;
    bool                mValid;
};

template<typename T>
struct ConcurrentRingBufferSlot
{
    using WorkItem = T;
    volatile Atomic32   mState;
    WorkItem            mData;
    volatile Atomic32   mSerial;
};

// **********************************
// These traits serve to provide overrides to some of the concurrent ring buffer operations.
// **********************************
template<typename T, typename ResultT = T>
struct ConcurrentRingBufferTraits
{
    using WorkResult = ConcurrentRingBufferWorkValueResult<T>;

    // **********************************
    // Implement to return a constructed object in it's default state.
    // **********************************
    static T Default()
    {
        return T();
    }
    // **********************************
    // Implement to reset the 'item' to it's default state.
    // **********************************
    static void Reset(T& item)
    {
        item = Default();
    }
    // **********************************
    // Implement to atomically assign the slot's data based on the input.
    //
    // Eg.slot.mData = input; where slot.mData == output
    // **********************************
    static void Push(T& output, T& input)
    {
        output = input;
    }
    // **********************************
    // Implement to return the result type you want from the TryPush/TryPop functions
    // **********************************
    static ResultT ToResultType(ConcurrentRingBufferSlot<T>& item)
    {
        return item.mData;
    }
    // **********************************
    // Implement to return the default value of the result type you want from the TryPush/TryPop functions
    // **********************************
    static ResultT ToResultTypeDefault()
    {
        return Default();
    }
};

enum ConcurrentRingBufferState
{
    CRBS_PRODUCER_READY,
    CRBS_CONSUMER_READY
};

// **********************************
// This class serves to simplify transferring data between threads in a 'thread safe' manner
// 
// note: Input/Output order is not guaranteed
// **********************************
template<typename T, typename TraitsT = ConcurrentRingBufferTraits<T>>
class LF_CORE_API ConcurrentRingBuffer
{
public:
    using WorkItem = T;
    using WorkTraits = TraitsT;
    using WorkSlot = ConcurrentRingBufferSlot<T>;
    using WorkSlots = TArray<WorkSlot>;
    using WorkResult = typename WorkTraits::WorkResult;

    ConcurrentRingBuffer() :
        mSlots(),
        mPushId(0),
        mPopId(0),
        mSize(0)
    {
        InitializeSlots(128);
    }

    ConcurrentRingBuffer(SizeT bufferSize) :
        mSlots(),
        mPushId(0),
        mPopId(0),
        mSize(0)
    {
        AssertError(bufferSize > 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
        InitializeSlots(bufferSize);
    }

    void Resize(SizeT bufferSize)
    {
        AssertError(bufferSize > 0, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
        InitializeSlots(bufferSize);
    }
    
    // **********************************
    // Attempts to push an item into the ring buffer.
    //
    // Fails to push a work item into the ring buffer if
    //   a) The allocated slot is in use
    //   b) The ring buffer is full
    //
    // @param data -- The work item you want to transfer
    // @returns    -- True if the work item was successfully inserted into the ring buffer, False otherwise
    // **********************************
    WorkResult TryPush(WorkItem data)
    {
        // Get id we can reserve a slot with
        Atomic32 reserve = static_cast<Atomic32>(GetPlatformThreadId());
        // We assume the platform will not give thread ids equal to the producer/consumer ready states.
        AssertError(reserve != CRBS_PRODUCER_READY && reserve != CRBS_CONSUMER_READY, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        // Get our 'unique' slot. (note: Uniqueness is not guaranteed in multithreaded environment)
        WorkSlot& slot = AllocatePushSlot();
        // Try acquire ownership
        Atomic32 result = AtomicCompareExchange(&slot.mState, reserve, CRBS_PRODUCER_READY);
        if (result == CRBS_PRODUCER_READY)
        {
            // Transfer data
            // slot.mData = data;
            WorkTraits::Push(slot.mData, data);
            AtomicIncrement32(&mSize);
            if (Invalid(AtomicIncrement32(&slot.mSerial)))
            {
                AtomicIncrement32(&slot.mSerial);
            }

            auto workResult = WorkTraits::ToResultType(slot);
            // Release ownership to any consumer
            result = AtomicCompareExchange(&slot.mState, CRBS_CONSUMER_READY, reserve);
            AssertError(result == reserve, LF_ERROR_BAD_STATE, ERROR_API_CORE);
            return{ workResult, true };
        }
        else
        {
            return{ WorkTraits::ToResultTypeDefault(), false };
        }
    }

    // **********************************
    // Attempts to pop an item off the ring buffer.
    //
    // Fails to pop a work item into the ring buffer if
    //   a) The allocated slot is in use
    //   b) The ring buffer is full
    //
    // @param data -- The work item you want to transfer
    // @returns    -- True if the work item was successfully inserted into the ring buffer, False otherwise
    // **********************************
    WorkResult TryPop()
    {
        // Get id we can reserve a slot with
        Atomic32 reserve = static_cast<Atomic32>(GetPlatformThreadId());
        // We assume the platform will not give thread ids equal to the producer/consumer ready states.
        AssertError(reserve != CRBS_PRODUCER_READY && reserve != CRBS_CONSUMER_READY, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        // Get our 'unique' slot. (note: Uniqueness is not guaranteed in multithreaded environment)
        WorkSlot& slot = AllocatePopSlot();
        // Try acquire ownership
        Atomic32 result = AtomicCompareExchange(&slot.mState, reserve, CRBS_CONSUMER_READY);
        if (result == CRBS_CONSUMER_READY)
        {
            // Transfer data
            auto workResult = WorkTraits::ToResultType(slot);
            WorkTraits::Reset(slot.mData);
            if (Invalid(AtomicIncrement32(&slot.mSerial)))
            {
                AtomicIncrement32(&slot.mSerial);
            }
            AtomicDecrement32(&mSize);

            // Release ownership to any producer
            result = AtomicCompareExchange(&slot.mState, CRBS_PRODUCER_READY, reserve);
            AssertError(result == reserve, LF_ERROR_BAD_STATE, ERROR_API_CORE);
            return { workResult, true };
        }
        else
        {
            return { WorkTraits::ToResultTypeDefault(), false };
        }
    }

    SizeT Size() const { return static_cast<SizeT>(AtomicLoad(&mSize)); }
    SizeT Capacity() const { return mSlots.Size(); }
private:
    WorkSlot& AllocatePushSlot() { return mSlots[AtomicIncrement32(&mPushId) % mSlots.Size()]; }
    WorkSlot& AllocatePopSlot() { return mSlots[AtomicIncrement32(&mPopId) % mSlots.Size()]; }

    void InitializeSlots(SizeT size)
    {
        if (size < mSlots.Size())
        {
            mSlots.Clear();
        }
        mSlots.Resize(size);
        for (WorkSlot& slot : mSlots)
        {
            AtomicStore(&slot.mState, CRBS_PRODUCER_READY);
            slot.mData = WorkTraits::Default();
        }
    }

    // Slots that contain a state/work item
    WorkSlots           mSlots;
    volatile Atomic32   mPushId;
    volatile Atomic32   mPopId;
    // Number of work items currently in the pipe, ( mSize <= mSlots.Size() )
    volatile Atomic32   mSize;
};

} // namespace lf

#endif // LF_CORE_CONCURRENT_RING_BUFFER_H
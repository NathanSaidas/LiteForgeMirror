// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "TaskHandle.h"

namespace lf {

TaskHandle::TaskHandle() : 
mSlot(nullptr),
mSerial(INVALID32)
{

}
TaskHandle::TaskHandle(const TaskTypes::TaskRingBufferResult& ringBufferResult) :
mSlot(ringBufferResult ? ringBufferResult.mData.mSlot : nullptr),
mSerial(ringBufferResult ? ringBufferResult.mData.mSerial : INVALID32)
{

}

void TaskHandle::Wait()
{
    if (mSlot)
    {
        // Figure out reserve id for locking the slot
        Atomic32 reserve = static_cast<Atomic32>(GetPlatformThreadId());
        // Ensure were not overlapping with state
        AssertError(reserve != CRBS_PRODUCER_READY && reserve != CRBS_CONSUMER_READY, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        // Try lock for consumption
        Atomic32 result = AtomicCompareExchange(&mSlot->mState, reserve, CRBS_CONSUMER_READY);
        if (result == CRBS_CONSUMER_READY)
        {
            // Ensure we have the same serial
            if (AtomicLoad(&mSlot->mSerial) == mSerial)
            {
                if (mSlot->mData.mCallback)
                {
                    mSlot->mData.mCallback.Invoke(mSlot->mData.mParam);
                    mSlot->mData.mCallback = TaskCallback();
                }
                // Give back to consumer, they can do the pop and keep track of size correctly
                result = AtomicCompareExchange(&mSlot->mState, CRBS_CONSUMER_READY, reserve);
                AssertError(result == reserve, LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
            else // If we didn't just push it back for consumer ready because the task has already been completed!
            {
                result = AtomicCompareExchange(&mSlot->mState, CRBS_CONSUMER_READY, reserve);
                AssertError(result == reserve, LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
        }
    }
}

} // namespace lf
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
        AssertEx(reserve != CRBS_PRODUCER_READY && reserve != CRBS_CONSUMER_READY, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
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
                AssertEx(result == reserve, LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
            else // If we didn't just push it back for consumer ready because the task has already been completed!
            {
                result = AtomicCompareExchange(&mSlot->mState, CRBS_CONSUMER_READY, reserve);
                AssertEx(result == reserve, LF_ERROR_BAD_STATE, ERROR_API_CORE);
            }
        }
    }
}

} // namespace lf
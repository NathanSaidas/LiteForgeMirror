// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_TASK_TYPES_H
#define LF_CORE_TASK_TYPES_H

#include "Core/Concurrent/ConcurrentRingBuffer.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/SmartCallback.h"


// #define LF_MPMC_BOUNDLESS_EXP

namespace lf {
DECLARE_STRUCT_ATOMIC_PTR(TaskItem);
using TaskCallback = TCallback<void, void*>;
using TaskLambdaCallback = void(*)(void*);

struct TaskItem
{
    TaskItem() : mCallback(), mParam(nullptr) {}

    TaskCallback mCallback;
    void*        mParam;
};

// **********************************
// Override of ring buffer traits to ensure the defaults/reset is a NULL_PTR and not something else.
// **********************************
template<>
class ConcurrentRingBufferTraits<TaskItemAtomicPtr>
{
public:
    static TaskItemAtomicPtr Default() { return NULL_PTR; }
    static void Reset(TaskItemAtomicPtr& item) { item = NULL_PTR; }
    static void Push(TaskItemAtomicPtr& output, TaskItemAtomicPtr& input) { output = input; }
    static TaskItemAtomicPtr Pop(TaskItemAtomicPtr& input) { return input; }
};



} // namespace lf

namespace lf { namespace TaskTypes {

using TaskItemType = TaskItem;
using TaskRingBufferSlot = ConcurrentRingBufferSlot<TaskItemType>;

struct TaskRingBufferResultWrapper
{
    using WorkItem = TaskRingBufferSlot;
    TaskRingBufferResultWrapper() : mSlot(nullptr), mSerial(INVALID32), mCallback(), mParam(nullptr) {}
    TaskRingBufferResultWrapper(WorkItem* slot, Atomic32 serial, const TaskCallback& callback, void* param) : mSlot(slot), mSerial(serial), mCallback(callback), mParam(param) {}
    WorkItem* mSlot;
    Atomic32  mSerial;
    TaskCallback mCallback;
    void*        mParam;
};

// WorkResult
struct TaskRingBufferResult
{
    operator bool() const { return mValid; }
    TaskRingBufferResultWrapper mData;
    bool                        mValid;
};

struct TaskRingBufferTraits
{
    using SlotType = TaskRingBufferSlot;
    using WorkResult = TaskRingBufferResult;
    static TaskItemType Default() { return TaskItemType(); }
    static void Reset(TaskItemType& item) { item = Default(); }
    static void Push(TaskItemType& output, TaskItemType& input) { output = input; }
    static TaskRingBufferResultWrapper ToResultType(TaskRingBufferSlot& slot) { return TaskRingBufferResultWrapper(&slot, slot.mSerial, slot.mData.mCallback, slot.mData.mParam); }
    static TaskRingBufferResultWrapper ToResultTypeDefault() { return TaskRingBufferResultWrapper(); }
};

using RingBufferType = ConcurrentRingBuffer<TaskItemType, TaskRingBufferTraits>;

#if defined(LF_MPMC_BOUNDLESS_EXP)
using TaskItemType = TaskItemAtomicPtr;
using RingBufferType = ConcurrentRingBuffer<TaskItemType>;
#endif

enum TaskEnqueueStatus
{
    // Task was executed synchronously because async execution is disabled.
    TES_SYNCHRONOUS,
    // Task was queued in a lock-free fashion (fast)
    TES_LOCK_FREE,
    // Task was queued but had to wait for a lock to be acquired (slow)
    TES_LOCK
};

struct TaskDeliveryThreadOptions
{
    TaskDeliveryThreadOptions() :
    mFastBufferCapacity(256),
    mRingBufferDrain(16),
    mFatBufferDrain(128)
    {}

    // How large the MPMC container should be
    SizeT mFastBufferCapacity;
    // How many iterations should be done to pop something off the ring buffer each update
    SizeT mRingBufferDrain;
    // How many iterations should be done to pop something off the internal buffer to the dispatcher
    SizeT mFatBufferDrain;
};

struct TaskSchedulerOptions
{
    TaskSchedulerOptions() :
    mNumDeliveryThreads(4),
    mNumWorkerThreads(4),
    mDispatcherSize(512)
#if defined(LF_MPMC_BOUNDLESS_EXP)
    ,mDeliveryOptions()
#endif
    {}

    // The number of delivery threads the scheduler runs
    SizeT mNumDeliveryThreads;
    // The number of worker threads the scheduler runs (note: it's typically better to have more workers than delivery)
    SizeT mNumWorkerThreads;
    // The dispatcher buffer size
    SizeT mDispatcherSize;

#if defined(LF_MPMC_BOUNDLESS_EXP)
    // todo: Dynamic vs Static Behavior
    //       A dynamic scheduler should be able to detect congestion with delivery threads (filling ring buffers)
    //       and spawn new delivery threads temporarily to handle work to reduce wait-time under a lock for the caller
    // Options specific to the TaskDeliveryThread
    TaskDeliveryThreadOptions mDeliveryOptions;
#endif

    // todo: Debug callback hooks (for unit testing)
};

} } // namespace lf::TaskTypes

#endif // LF_CORE_TASK_TYPES_H
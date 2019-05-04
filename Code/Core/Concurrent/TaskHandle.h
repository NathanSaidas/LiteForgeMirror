// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_TASK_HANDLE_H
#define LF_CORE_TASK_HANDLE_H

#include "Core/Concurrent/TaskTypes.h"



namespace lf {

class LF_CORE_API TaskHandle
{
public:
    using SlotType = TaskTypes::TaskRingBufferSlot;
    TaskHandle();
    TaskHandle(const TaskTypes::TaskRingBufferResult& ringBufferResult);


    operator bool() const { return mSlot != nullptr; }
    // Wait for the task to complete
    // note: This returns control back to user code once the task has been popped off,
    //       there is no guarantee the task has actually run the function and completed on the worker thread
    //       so you must validate your data before proceeding
    void Wait();
private:
    SlotType* mSlot;
    Atomic32  mSerial;
};

} // namespace lf 

#endif // LF_CORE_TASK_HANDLE_H
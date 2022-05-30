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
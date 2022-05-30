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

struct ThreadFenceHandle;

// Thread fence is a synchronization primitive that allows one thread 
// to wait for another thread for a specific amount of time or infinite
// amount of time.
class LF_CORE_API ThreadFence
{
public:
    enum WaitStatus
    {
        // A call to Wait failed, resources are not allocated.
        WS_FAILED,
        // A call to Wait completed but the fence was in a suspended state.
        WS_TIMED_OUT,
        // A call to Wait completed and the fence is no longer in a suspended state.
        WS_SUCCESS
    };

    ThreadFence();
    ThreadFence(const ThreadFence& other);
    ThreadFence(ThreadFence&& other);
    ~ThreadFence();

    ThreadFence& operator=(const ThreadFence& other);
    ThreadFence& operator=(ThreadFence&& other);

    // Initializes the thread fence, the sychronization functions below will
    // fail if the fence is not initialized.
    bool Initialize();
    // Destroys resources allocated by the thread fence (internal handle will be kept
    // until all references have been destroyed)
    void Destroy();

    // Changes the fence value to suspend/resume execution of waiting threads.
    bool Set(bool isBlocking);
    // Sends a signal to all those waiting on the fence to continue execution
    bool Signal();
    // Wait for signal to be sent or for the event to resume execution.
    WaitStatus Wait(SizeT milliseconds = INVALID);
private:
    void AddRef();
    void RemoveRef();
    void DestroyEvent();

    ThreadFenceHandle* mHandle;
};

} // namespace lf

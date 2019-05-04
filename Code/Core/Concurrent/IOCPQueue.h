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
#ifndef LF_CORE_IOCP_QUEUE_H
#define LF_CORE_IOCP_QUEUE_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf { namespace InternalHooks {

struct IOCPQueueImpl;

void IOCPInitialize(IOCPQueueImpl*& self, SizeT numConsumers);
void IOCPRelease(IOCPQueueImpl*& self);
bool IOCPEnqueue(IOCPQueueImpl* self, SizeT itemSize, void* item);
bool IOCPDequeue(IOCPQueueImpl* self, void*& item);

} } // namespace lf::InternalHooks

namespace lf {

template<typename T>
class LF_CORE_API IOCPQueue
{
public:
    using WorkItem = T;
    struct WorkResult
    {
        operator bool() const { return mValid; }
        WorkItem* mData;
        bool      mValid;
    };

    IOCPQueue() :
        mImpl()
    {
        InternalHooks::IOCPInitialize(mImpl, 4);
    }

    IOCPQueue(SizeT numConsumers) :
        mImpl()
    {
        InternalHooks::IOCPInitialize(mImpl, numConsumers);
    }

    ~IOCPQueue()
    {
        InternalHooks::IOCPRelease(mImpl);
    }

    void SetConsumers(SizeT numConsumers)
    {
        InternalHooks::IOCPRelease(mImpl);
        InternalHooks::IOCPInitialize(mImpl, numConsumers);
    }

    bool TryPush(WorkItem* data)
    {
        return InternalHooks::IOCPEnqueue(mImpl, sizeof(WorkItem), data);
    }

    WorkResult TryPop()
    {
        void* data = nullptr;
        bool result = InternalHooks::IOCPDequeue(mImpl, data);
        if (result)
        {
            return{ static_cast<WorkItem*>(data), true };
        }
        else
        {
            return{ nullptr, false };
        }
    }

private:
    InternalHooks::IOCPQueueImpl* mImpl;

};

} // namespace lf

#endif // LF_CORE_IOCP_QUEUE_H
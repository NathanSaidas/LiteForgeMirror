// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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
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
#include "NetEventController.h"
#include "Core/Memory/DynamicPoolHeap.h"
#include "Core/Utility/Utility.h"

namespace lf {

#define NET_EVENT_TYPE(Type_) { EventTypeInfo info; info.mHeap = nullptr; info.mTypeName = #Type_; info.mSize = sizeof(Type_); info.mAlignment = alignof(Type_); mTypes.Add(info); }

NetEventController::NetEventController()
: mHeaps()
, mTypes()
{
}
NetEventController::NetEventController(NetEventController&& other)
: mHeaps(std::move(other.mHeaps))
, mTypes(std::move(other.mTypes))
{

}
NetEventController::~NetEventController()
{
    // todo: Might wanna assert there are 0 allocations left...
}
NetEventController& NetEventController::operator=(NetEventController&& other)
{
    if (this != &other)
    {
        // todo: Might wanna assert there are 0 allocations left...
        mHeaps = std::move(other.mHeaps);
        mTypes = std::move(other.mTypes);
    }
    return *this;
}

bool NetEventController::Initialize()
{
    if (!mTypes.Empty() || !mHeaps.Empty())
    {
        return false;
    }

    // constants for the allocated heaps.
    const SizeT MAX_OBJECTS = 100;
    const SizeT MAX_HEAPS = 3;
#if defined(LF_TEST)
    const UInt32 FLAGS = PoolHeap::PHF_DOUBLE_FREE | PoolHeap::PHF_DETECT_LOCAL_HEAP_CORRUPTION;
#else
    const UInt32 FLAGS = PoolHeap::PHF_DOUBLE_FREE;
#endif

    mTypes.Reserve(NetEventType::MAX_VALUE);

    // Manual Lightweight-RTTI, register in order of enum declaraction.
    NET_EVENT_TYPE(NetConnectSuccessEvent);
    NET_EVENT_TYPE(NetConnectFailedEvent);
    NET_EVENT_TYPE(NetConnectionCreatedEvent);
    NET_EVENT_TYPE(NetConnectionTerminatedEvent);
    NET_EVENT_TYPE(NetHeartbeatReceivedEvent);
    NET_EVENT_TYPE(NetDataReceivedRequestEvent);
    NET_EVENT_TYPE(NetDataReceivedResponseEvent);
    NET_EVENT_TYPE(NetDataReceivedActionEvent);
    NET_EVENT_TYPE(NetDataReceivedReplicationEvent);

    if (mTypes.Size() != NetEventType::MAX_VALUE)
    {
        ReportBugMsg("NetEventController failed to initialize. Missing net event types for lightweight RTTI.");
        mTypes.Clear();
        mHeaps.Clear();
        return false;
    }

    for(EventTypeInfo& info : mTypes)
    {
        // PoolHeaps require a minimum allocation of at least void* (to store the 'next' object)
        SizeT poolSize = Max(info.mSize, sizeof(void*));

        DynamicPoolHeap* heap = FindHeap(poolSize, info.mAlignment);
        if (!heap)
        {
            heap = LFNew<DynamicPoolHeap>();
            CriticalAssert(heap->Initialize(poolSize, info.mAlignment, MAX_OBJECTS, MAX_HEAPS, FLAGS));
            mHeaps.Add(DynamicPoolHeapPtr(heap));
        }
        info.mHeap = heap;
    }

    return true;
}

void NetEventController::Reset() 
{
    mTypes.Clear();
    mHeaps.Clear();
}

void NetEventController::GCCollect()
{
    for (DynamicPoolHeap* heap : mHeaps)
    {
        heap->GCCollect();
    }
}

DynamicPoolHeap* NetEventController::FindHeap(SizeT size, SizeT alignment)
{
    // todo: We can allocate 'exact' fit or not...
    for(DynamicPoolHeap* heap : mHeaps)
    {
        if (heap->GetObjectSize() == size && heap->GetObjectAlignment() == alignment)
        {
            return heap;
        }
    }
    return nullptr;
}

} // namespace lf
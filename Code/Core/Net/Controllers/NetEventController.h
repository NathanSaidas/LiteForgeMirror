#pragma once
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
#include "Core/Net/NetEvent.h"
#include "Core/Utility/Array.h"
#include "Core/Memory/SmartPointer.h"

namespace lf {

DECLARE_PTR(DynamicPoolHeap);

class LF_CORE_API NetEventController
{
public:
    struct EventTypeInfo
    {
        DynamicPoolHeap* mHeap;
        const char*      mTypeName;
        SizeT            mSize;
        SizeT            mAlignment;
    };

    NetEventController(const NetEventController& other) = delete;
    NetEventController& operator=(const NetEventController& other) = delete;

    NetEventController();
    NetEventController(NetEventController&& other);
    ~NetEventController();
    NetEventController& operator=(NetEventController&& other);

    bool Initialize();
    void Reset();

    void GCCollect();

    template<typename EventT>
    EventT* Allocate();

    template<typename EventT>
    void Free(EventT* event);

private:
    DynamicPoolHeap* FindHeap(SizeT size, SizeT alignment);

    TArray<DynamicPoolHeapPtr> mHeaps;
    TArray<EventTypeInfo>   mTypes;
};

template<typename EventT>
EventT* NetEventController::Allocate()
{
    EventTypeInfo& info = mTypes[EventT::EVENT_TYPE];
    void* memory = info.mHeap->Allocate();
    EventT* event = new(memory)EventT();
    CriticalAssert(event->GetType() == EventT::EVENT_TYPE);
    return event;
}

template<typename EventT>
void NetEventController::Free(EventT* event)
{
    if (event)
    {
        EventTypeInfo& info = mTypes[event->GetType()];
        event->~EventT();
        info.mHeap->Free(event);
    }
}

} // namespace lf
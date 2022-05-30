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
#include "Runtime/PCH.h"
#include "EventMgrImpl.h"
#include "Core/Reflection/Type.h"
#include "Core/Utility/Error.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "Runtime/Async/Async.h"

namespace lf
{

struct EventLock
{
    EventLock(volatile Atomic32& value) : mValue(value) { AtomicIncrement32(&mValue); }
    ~EventLock() { AtomicDecrement32(&mValue); }
    volatile Atomic32& mValue;
};

EventMgrImpl::EventMgrImpl()
: mEventPoolLock()
, mEventPool()
, mListenerRecursiveLock(0)
, mListenerLock()
, mListeners()
{
}
EventMgrImpl::~EventMgrImpl()
{

}

bool EventMgrImpl::Initialize()
{
    TVector<const Type*> types = GetReflectionMgr().FindAll(typeof(Event), false);

    // Build Event Pool
    {
        EventPoolMap::BuilderDataType data;
        data.resize(types.size());
        for (SizeT i = 0; i < data.size(); ++i)
        {
            data[i].first = types[i];
        }
        std::sort(data.begin(), data.end(),
            [](const EventPoolMap::PairType& a, const EventPoolMap::PairType& b)
            {
                return a.first < b.first;
            });
        mEventPool.Build(data);
    }

    // Build Event Listener
    {
        EventListenerMap::BuilderDataType data;
        data.resize(types.size());
        for (SizeT i = 0; i < data.size(); ++i)
        {
            data[i].first = types[i];
        }
        std::sort(data.begin(), data.end(),
            [](const EventListenerMap::PairType& a, const EventListenerMap::PairType& b)
            {
                return a.first < b.first;
            });
        mListeners.Build(data);
    }

    return true;
}
void EventMgrImpl::Shutdown()
{
    mEventPool.Clear();
    mListeners.Clear();
}

EventAtomicPtr EventMgrImpl::CreateEvent(const Type* type)
{
    if (type->IsA(typeof(Event)))
    {
        return NULL_PTR;
    }

    EventAtomicPtr event;

    // try pool
    {
        ScopeLock lock(mEventPoolLock);
        EventPool& pool = mEventPool.FindRef(type);
        if (!pool.empty())
        {
            event = pool.back();
            pool.erase(pool.rend().base());
            return event;
        }
    }

    // create new
    event = GetReflectionMgr().CreateAtomic<Event>(type);
    if (!event)
    {
        return NULL_PTR;
    }
    event->GetWeakPointer() = event;
    return event;
}

APIResult<bool> EventMgrImpl::Post(Event* event, AppThreadId threadID)
{
    EventAtomicPtr ptr = GetAtomicPointer(event);
    if (!ptr)
    {
        return ReportError(false, ArgumentNullError, "event");
    }
    if (!event->GetType())
    {
        return ReportError(false, InvalidArgumentError, "event", "Object not initialized with reflection!");
    }
    if (Invalid(threadID))
    {
        return ReportError(false, InvalidArgumentError, "threadID", "Invalid thread id.");
    }

    auto callback = AppThreadDispatchCallback::Make([ptr, this]()
    {
        EventLock eventLock(mListenerRecursiveLock);
        ScopeRWSpinLockRead lock(mListenerLock);
        EventListeners& listeners = mListeners.FindRef(ptr->GetType());
        for (EventCallback& listener : listeners)
        {
            listener.Invoke(ptr);
        }
    });

    if (GetAsync().ExecuteOn(threadID, callback))
    {
        return ReportError(false, OperationFailureError, "Failed to execute async callback, is the thread not an app thread?", "<TODO: THREAD_ID>");
    }

    return APIResult<bool>(true);
}
APIResult<bool> EventMgrImpl::Emit(Event* event) 
{
    EventAtomicPtr ptr = GetAtomicPointer(event);
    if (!ptr)
    {
        return ReportError(false, ArgumentNullError, "event");
    }
    if (!event->GetType())
    {
        return ReportError(false, InvalidArgumentError, "event", "Object not initialized with reflection!");
    }
    if (!event->GetType()->IsA(typeof(Event)))
    {
        return ReportError(false, InvalidTypeArgumentError, "event->GetType()", typeof(Event), event->GetType());
    }

    EventLock eventLock(mListenerRecursiveLock);
    ScopeRWSpinLockRead lock(mListenerLock);
    EventListeners& listeners = mListeners.FindRef(event->GetType());
    for (EventCallback& listener : listeners)
    {
        listener.Invoke(event);
    }

    return APIResult<bool>(true);
}

APIResult<bool> EventMgrImpl::Register(const Type* eventType, const EventCallback& callback)
{
    if (eventType == nullptr)
    {
        return ReportError(false, ArgumentNullError, "eventType");
    }
    if (!eventType->IsA(typeof(Event)))
    {
        return ReportError(false, InvalidTypeArgumentError, "eventType", typeof(Event), eventType);
    }

    Assert(AtomicLoad(&mListenerRecursiveLock) == 0);
    ScopeRWSpinLockWrite lock(mListenerLock);
    mListeners.FindRef(eventType).push_back(callback);
    return APIResult<bool>(true);

}
APIResult<bool> EventMgrImpl::Unregister(const Type* eventType, const EventCallback& callback)
{
    if (eventType == nullptr)
    {
        return ReportError(false, ArgumentNullError, "eventType");
    }
    if (!eventType->IsA(typeof(Event)))
    {
        return ReportError(false, InvalidTypeArgumentError, "eventType", typeof(Event), eventType);
    }

    Assert(AtomicLoad(&mListenerRecursiveLock) == 0);
    ScopeRWSpinLockWrite lock(mListenerLock);
    EventListeners& listeners = mListeners.FindRef(eventType);
    auto listener = std::find(listeners.begin(), listeners.end(), callback);
    if (listener != listeners.end())
    {
        listeners.erase(listener);
    }
    return APIResult<bool>(true);

}

}
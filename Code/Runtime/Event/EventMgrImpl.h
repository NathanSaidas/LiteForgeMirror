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
#include "Runtime/Event/EventMgr.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/Array.h"
#include "Core/Platform/RWSpinLock.h"

#include "Runtime/Asset/AssetIndex.h"

namespace lf
{

// Senders send an event on a specific thread, while listeners have the option
// to automatically re-route traffic.
class LF_RUNTIME_API EventMgrImpl : public EventMgr
{
public:
    EventMgrImpl();
    ~EventMgrImpl();

    bool Initialize();
    void Shutdown();

    EventAtomicPtr CreateEvent(const Type* type) override;

    APIResult<bool> Post(Event* event, AppThreadId threadID = APP_THREAD_ID_MAIN) override;
    APIResult<bool> Emit(Event* event) override;

    APIResult<bool> Register(const Type* eventType, const EventCallback& callback) override;
    APIResult<bool> Unregister(const Type* eventType, const EventCallback& callback) override;

private:
    using EventPool = TVector<EventAtomicPtr>;
    using EventPoolMap = TAssetIndex<const Type*, EventPool>;
    using EventListeners = TVector<EventCallback>;
    using EventListenerMap = TAssetIndex<const Type*, EventListeners>;

    SpinLock     mEventPoolLock;
    EventPoolMap mEventPool;

    volatile Atomic32 mListenerRecursiveLock;
    RWSpinLock       mListenerLock;
    EventListenerMap mListeners;

};

}
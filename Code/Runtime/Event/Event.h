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
#include "Core/Reflection/Object.h"
#include "Core/Utility/SmartCallback.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf
{
class Event;

DECLARE_WPTR(Object);
DECLARE_ATOMIC_WPTR(Object);
DECLARE_HASHED_CALLBACK(EventCallback, void, const Event*);

class LF_RUNTIME_API Event : public Object, public TAtomicWeakPointerConvertible<Event>
{
    DECLARE_CLASS(Event, Object)
public:
    using PointerConvertible = PointerConvertibleType;
    virtual ~Event() = default;

    virtual void Reset();

    // Syntax: OnComplete = MyCallback
    EventCallback& OnComplete() { return mOnComplete; }
    const EventCallback& OnComplete() const { return mOnComplete; }

    const Object* GetSender() const { return mSender; }
    void SetSender(const ObjectWPtr& value) { mSender = value; }
    void SetSender(const ObjectAtomicWPtr& value) { mSenderAtomic = value; }

private:
    EventCallback       mOnComplete;
    ObjectWPtr          mSender;
    ObjectAtomicWPtr    mSenderAtomic;
};
} // namespace lf
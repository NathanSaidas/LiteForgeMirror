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
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/APIResult.h"
#include "Runtime/Async/AppThread.h"
#include "Runtime/Event/Event.h"

namespace lf
{

DECLARE_ATOMIC_PTR(Event);

class LF_RUNTIME_API EventMgr
{
public:
    virtual EventAtomicPtr CreateEvent(const Type* type) = 0;

    virtual APIResult<bool> Post(Event* event, AppThreadId threadID = APP_THREAD_ID_MAIN) = 0;
    virtual APIResult<bool> Emit(Event* event) = 0;

    virtual APIResult<bool> Register(const Type* eventType, const EventCallback& callback) = 0;
    virtual APIResult<bool> Unregister(const Type* eventType, const EventCallback& callback) = 0;
};

LF_RUNTIME_API EventMgr& GetEventMgr();

} // namespace lf
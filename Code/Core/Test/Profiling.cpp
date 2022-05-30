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
#include "Core/PCH.h"
#include "Profiling.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Test/ProfilerHooks.h"
#include "Core/Utility/Time.h"
#include <string.h>

namespace lf {

static void NullSubmit(const ProfileScopeCaptureData&) {}
static void NullSubmit(const ProfileScopeObjectCaptureData&) {}

UInt64 Profiling::gFrame = 0;
Profiling::SubmitScopeCallback Profiling::gSubmitScope = Profiling::SubmitScopeCallback::Make(NullSubmit);
Profiling::SubmitScopeObjectCallback Profiling::gSubmitScopeObject= Profiling::SubmitScopeObjectCallback::Make(NullSubmit);
volatile Atomic32 Profiling::gEnabled = 0;

ProfileScope::ProfileScope(const char* label, bool groupEnabled)
{
    if (!groupEnabled || AtomicLoad(&Profiling::gEnabled) == 0)
    {
        mCapture.mLabel = nullptr;
        return;
    }

    mCapture.mFrame = Profiling::gFrame;
    mCapture.mThreadID = static_cast<UInt16>(Thread::GetId());
    mCapture.mThreadTag = 0; // todo: When we have thread attributes, assign this.
    mCapture.mLabel = label;
    mCapture.mThreadEndCore = mCapture.mThreadBeginCore = static_cast<UInt16>(Thread::GetExecutingCore());
    mCapture.mEndTick = mCapture.mBeginTick = GetClockTime();
}
ProfileScope::~ProfileScope()
{
    if (mCapture.mLabel != nullptr)
    {
        mCapture.mEndTick = GetClockTime();
        mCapture.mThreadEndCore = static_cast<UInt16>(Thread::GetExecutingCore());
        Profiling::gSubmitScope.Invoke(mCapture);
    }
}

ProfileScopeObject::ProfileScopeObject(const char* label, const char* objectName, UInt32 objectID, bool groupEnabled)
{
    if (!groupEnabled || AtomicLoad(&Profiling::gEnabled) == 0)
    {
        mCapture.mLabel = nullptr;
        return;
    }

    mCapture.mFrame = Profiling::gFrame;
    mCapture.mThreadID = static_cast<UInt16>(Thread::GetId());
    mCapture.mThreadTag = 0; // todo: When we have thread attributes, assign this.
    mCapture.mLabel = label;
    strcpy_s(mCapture.mObjectName, LF_ARRAY_SIZE(mCapture.mObjectName), objectName);
    mCapture.mObjectID = objectID;
    mCapture.mThreadEndCore = mCapture.mThreadBeginCore = static_cast<UInt16>(Thread::GetExecutingCore());
    mCapture.mEndTick = mCapture.mBeginTick = GetClockTime();
}
ProfileScopeObject::~ProfileScopeObject()
{
    if (mCapture.mLabel != nullptr)
    {
        mCapture.mEndTick = GetClockTime();
        mCapture.mThreadEndCore = static_cast<UInt16>(Thread::GetExecutingCore());
        Profiling::gSubmitScopeObject.Invoke(mCapture);
    }
}

} // namespace 
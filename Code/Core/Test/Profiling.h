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

struct ProfileScopeCaptureData
{
    Int64 mBeginTick;
    Int64 mEndTick;
    UInt64 mFrame;
    UInt16 mThreadBeginCore;
    UInt16 mThreadEndCore;
    UInt16 mThreadID;
    UInt16 mThreadTag;
    const char* mLabel;
};

struct ProfileScopeObjectCaptureData
{
    Int64 mBeginTick;
    Int64 mEndTick;
    UInt64 mFrame;
    UInt16 mThreadBeginCore;
    UInt16 mThreadEndCore;
    UInt16 mThreadID;
    UInt16 mThreadTag;
    const char* mLabel;
    char   mObjectName[64];
    UInt32 mObjectID;
};

struct LF_CORE_API ProfileScope
{
    ProfileScope(const char* label, bool groupEnabled);
    ~ProfileScope();
private:
    ProfileScopeCaptureData mCapture;
};

struct LF_CORE_API ProfileScopeObject
{
    ProfileScopeObject(const char* label, const char* objectName, UInt32 objectID, bool groupEnabled);
    ~ProfileScopeObject();

private:
    ProfileScopeObjectCaptureData mCapture;
};

#define PROFILE_SCOPE(label_, group_) ::lf::ProfileScope LF_ANONYMOUS_NAME(profileScope_)(label_, group_);
#define PROFILE_SCOPE_OBJECT(label_, objectName_, objectID_, group_) ::lf::ProfileScopeObject LF_ANONYMOUS_NAME(profileScopeObject_)(label_, objectName_, objectID_, group_)

#define PROFILE_GROUP_ENABLED true
#define PROFILE_GROUP_DISABLED false

#define PROFILE_GROUP_DEFAULT PROFILE_GROUP_ENABLED



} // namespace lf
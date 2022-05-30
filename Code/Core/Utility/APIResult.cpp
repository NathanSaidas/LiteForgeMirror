// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "APIResult.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/StackTrace.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Memory/TempHeap.h"

namespace lf {

static TempHeap* sErrorHeap = nullptr;
static SpinLock sErrorLock;
static bool sErrorLockLocal = false;
static ErrorBase::ReportCallback sReport = [](const ErrorBase&, UInt32) {};

TempHeap* ErrorBase::GetAllocator()
{
    CriticalAssert(sErrorHeap != nullptr);
    return sErrorHeap;
}
void ErrorBase::SetAllocator(TempHeap* heap)
{
    sErrorHeap = heap;
}

ErrorBase::ReportCallback ErrorBase::SetReportCallback(ReportCallback callback)
{
    ReportCallback previous = sReport;
    sReport = callback;
    return previous;
}

void* ErrorBase::BeginError(SizeT size, SizeT alignment)
{
    sErrorLock.Acquire();
    if (sErrorLockLocal)
    {
        // If you hit this, you're trying to create an error while another 
        // one is currently reporting on the same thread.
        // 
        // This should be next to impossible!
        LF_ERROR_DEBUG_BREAK; 
    }
    sErrorLockLocal = true;
    return GetAllocator()->Allocate(size, alignment);
}
void ErrorBase::EndError()
{
    if (sErrorLockLocal == false)
    {
        // If you hit this, you likely forgot to call BeginErrorStd!
        LF_ERROR_DEBUG_BREAK;
    }
    GetAllocator()->Reset();
    sErrorLockLocal = false;
    sErrorLock.Release();
}

ErrorBase::ErrorBase()
: mStackTraceBuffer()
, mStackTrace(nullptr)
, mFilename("")
, mFlags(0)
, mLine(0)
, mReleased(false)
{
    LF_STATIC_ASSERT(sizeof(mStackTraceBuffer) == sizeof(StackTrace));
    LF_STATIC_ASSERT(alignof(ErrorBase::StackTraceBuffer) == alignof(StackTrace));
    mStackTrace = new(mStackTraceBuffer.mBytes)StackTrace();
}
ErrorBase::~ErrorBase()
{
    mStackTrace->~StackTrace();
    mStackTrace = nullptr;
}

void ErrorBase::Initialize(const char* file, SizeT line, UInt32 flags)
{
    mFilename = file;
    mLine = static_cast<UInt32>(line);
    mFlags = flags;

    if ((flags & ERROR_FLAG_LOG_CALLSTACK) > 0)
    {
        CaptureStackTrace(*mStackTrace, 16);
    }
}

void ErrorBase::Release()
{
    if (mStackTrace->frames != nullptr)
    {
        ReleaseStackTrace(*mStackTrace);
    }
    mReleased = true;
}

void ErrorBase::Ignore()
{
    if (mReleased)
    {
        return;
    }
    // Debug will always report but no callstack!
#if defined(LF_DEBUG)
    mFlags = mFlags & ~(ERROR_FLAG_LOG_CALLSTACK);
    mFlags = mFlags | ERROR_FLAG_LOG;
    sReport(*this, mFlags);
#endif
    Release();
}
void ErrorBase::Report()
{
    if (mReleased)
    {
        return;
    }
    sReport(*this, mFlags);
    Release();
}

const StackTrace& ErrorBase::GetStackTrace() const
{
    return *mStackTrace;
}

void* ErrorBase::Allocate(SizeT size, SizeT alignment)
{
    return GetAllocator()->Allocate(size, alignment);
}

const char* ErrorBase::GetUnknownErrorString() const
{
    return "Unknown Error";
}


} // namespace lf
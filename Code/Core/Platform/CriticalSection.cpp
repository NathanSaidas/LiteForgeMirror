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
#include "CriticalSection.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Memory/Memory.h"


#if defined(LF_OS_WINDOWS)
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#error "Missing implementation for platform."
#endif

namespace lf {

struct CriticalSectionData
{
    volatile long mRefCount;
#if defined(LF_DEBUG)
    const char*   mDebugName;
#endif
#if defined(LF_OS_WINDOWS)
    CRITICAL_SECTION mNativeHandle;
#endif
};


CriticalSection::CriticalSection() : 
mData(nullptr)
{}
CriticalSection::CriticalSection(const CriticalSection& cs) :
mData(cs.mData)
{
    AddRef();
}
CriticalSection::CriticalSection(CriticalSection&& cs) : 
mData(cs.mData)
{
    cs.mData = nullptr;
}
CriticalSection::~CriticalSection()
{
    RemoveRef();
}

void CriticalSection::Initialize(SizeT spinCount)
{
    AssertError(spinCount < 0xFFFF, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    AssertError(!mData, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    mData = LFNew<CriticalSectionData>();
    AssertError(mData, LF_ERROR_OUT_OF_MEMORY, ERROR_API_CORE);
    AssertError(InitializeCriticalSectionAndSpinCount(&mData->mNativeHandle, static_cast<DWORD>(spinCount)), LF_ERROR_INTERNAL, ERROR_API_CORE);
    _InterlockedExchange(&mData->mRefCount, 1);
#if defined(LF_DEBUG)
    mData->mDebugName = "";
#endif
}

void CriticalSection::Destroy()
{
    AssertError(mData, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    DeleteCriticalSection(&mData->mNativeHandle);
    LFDelete(mData);
    mData = nullptr;
}

void CriticalSection::Acquire()
{
    AssertError(mData, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    EnterCriticalSection(&mData->mNativeHandle);
}
bool CriticalSection::TryAcquire()
{
    AssertError(mData, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    return TryEnterCriticalSection(&mData->mNativeHandle) == TRUE;
}
void CriticalSection::Release()
{
    AssertError(mData, LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
    return LeaveCriticalSection(&mData->mNativeHandle);
}

CriticalSection& CriticalSection::operator=(const CriticalSection& cs)
{
    if (&cs == this)
    {
        return *this;
    }
    RemoveRef();
    mData = cs.mData;
    AddRef();
    return *this;
}

CriticalSection& CriticalSection::operator=(CriticalSection&& cs)
{
    if (&cs == this)
    {
        return *this;
    }
    RemoveRef();
    mData = cs.mData;
    cs.mData = nullptr;
    return *this;
}
bool CriticalSection::operator==(const CriticalSection& other) const
{
    return mData == other.mData;
}
bool CriticalSection::operator!=(const CriticalSection& other) const
{
    return mData != other.mData;
}
    
void CriticalSection::AddRef()
{
    if (mData)
    {
        _InterlockedIncrement(&mData->mRefCount);
    }
}
void CriticalSection::RemoveRef()
{
    if (mData)
    {
        if (_InterlockedDecrement(&mData->mRefCount) == 0)
        {
            Destroy();
        }
        mData = nullptr;
    }
}

} // namespace lf
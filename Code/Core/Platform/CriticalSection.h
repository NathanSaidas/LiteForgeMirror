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
#ifndef LF_CORE_CRITICAL_SECTION_H
#define LF_CORE_CRITICAL_SECTION_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

struct CriticalSectionData;

// Acts as a lock for this specific application.
// The critical section manages internal data which
// strongly ref counted.
class LF_CORE_API CriticalSection
{
public:
    CriticalSection();
    CriticalSection(const CriticalSection& cs);
    CriticalSection(CriticalSection&& cs);
    ~CriticalSection();

    void Initialize(SizeT spinCount = 1000);
    void Destroy();

    void Acquire();
    bool TryAcquire();
    void Release();

    CriticalSection& operator=(const CriticalSection& cs);
    CriticalSection& operator=(CriticalSection&& cs);
    bool operator==(const CriticalSection& other) const;
    bool operator!=(const CriticalSection& other) const;
private:
    void AddRef();
    void RemoveRef();

    CriticalSectionData* mData;
};

class LF_CORE_API ScopedCriticalSection
{
public:
    ScopedCriticalSection(CriticalSection& cs) : 
    mCriticalSection(cs)
    {
        mCriticalSection.Acquire();
    }
    ~ScopedCriticalSection()
    {
        mCriticalSection.Release();
    }
private:
    CriticalSection& mCriticalSection;
};

} // namespace lf

#endif // LF_CORE_CRITICAL_SECTION_H
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
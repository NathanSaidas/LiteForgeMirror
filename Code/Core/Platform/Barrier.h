#ifndef LF_CORE_BARRIER_H
#define LF_CORE_BARRIER_H

#include "Core/Common/Types.h"
#include <intrin.h>

#if defined(LF_OS_WINDOWS)

namespace lf {
LF_FORCE_INLINE void ReadWriteBarrier()
{
    _ReadWriteBarrier();
}
LF_FORCE_INLINE void ReadBarrier()
{
    _ReadBarrier();
}
LF_FORCE_INLINE void WriteBarrier()
{
    _WriteBarrier();
}
} // namespace lf

#else
#error "Missing Implementation"
#endif


#endif // LF_CORE_BARRIER_H
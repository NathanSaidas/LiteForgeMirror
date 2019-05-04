#ifndef LF_CORE_ATOMIC_H
#define LF_CORE_ATOMIC_H

#include "Core/Common/Types.h"
#include <intrin.h>

#if defined(LF_OS_WINDOWS)

namespace lf {

LF_FORCE_INLINE void AtomicRWBarrier() { _ReadWriteBarrier(); }

LF_FORCE_INLINE Atomic16 AtomicIncrement16(volatile Atomic16* value) { return _InterlockedIncrement16(value); }
LF_FORCE_INLINE Atomic32 AtomicIncrement32(volatile Atomic32* value) { return _InterlockedIncrement(value); }
LF_FORCE_INLINE Atomic64 AtomicIncrement64(volatile Atomic64* value) { return _InterlockedIncrement64(value); }

LF_FORCE_INLINE Atomic16 AtomicDecrement16(volatile Atomic16* value) { return _InterlockedDecrement16(value); }
LF_FORCE_INLINE Atomic32 AtomicDecrement32(volatile Atomic32* value) { return _InterlockedDecrement(value); }
LF_FORCE_INLINE Atomic64 AtomicDecrement64(volatile Atomic64* value) { return _InterlockedDecrement64(value); }

LF_FORCE_INLINE void AtomicStore(void* volatile* target, const void* value)
{
    _InterlockedExchangePointer(target, const_cast<void*>(value));
}
LF_FORCE_INLINE void AtomicStore(volatile Atomic32* target, Atomic32 value)
{
    _InterlockedExchange(target, value);
}
template<typename ValueT>
LF_FORCE_INLINE ValueT* AtomicStorePointer(ValueT* volatile* target, const ValueT* value)
{
    _InterlockedExchangePointer(reinterpret_cast<void* volatile*>(target), const_cast<ValueT*>(value));
    return 0;
}


LF_FORCE_INLINE void* AtomicLoad(void* volatile* target)
{
    void* value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
LF_FORCE_INLINE void* AtomicLoad(void* const volatile* target)
{
    void* value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
LF_FORCE_INLINE long AtomicLoad(volatile Atomic32* target)
{
    Atomic32 value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
LF_FORCE_INLINE long AtomicLoad(volatile const Atomic32* target)
{
    Atomic32 value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
template<typename ValueT>
LF_FORCE_INLINE ValueT* AtomicLoadPointer(ValueT* volatile* target)
{
    ValueT* value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
template<typename ValueT>
LF_FORCE_INLINE const ValueT* AtomicLoadPointer(const ValueT* volatile* target)
{
    const ValueT* value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}

LF_FORCE_INLINE Atomic32 AtomicCompareExchange(volatile Atomic32* target, Atomic32 value, Atomic32 compare)
{
    return _InterlockedCompareExchange(target, value, compare);
}
} // namespace lf

#else
#error "Missing implementation"
#endif

#endif // LF_CORE_ATOMIC_H
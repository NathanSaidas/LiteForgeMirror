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
#include <intrin.h>

#if defined(LF_OS_WINDOWS)

namespace lf {

LF_FORCE_INLINE void AtomicRWBarrier() { _ReadWriteBarrier(); }

// 
LF_FORCE_INLINE void AtomicSFence() { _mm_sfence(); }
// Wait for loading instructions before preceding.
LF_FORCE_INLINE void AtomicLFence() { _mm_lfence(); }
// Ensure every load/store becomes program visible before continueing
LF_FORCE_INLINE void AtomicMFence() { _mm_mfence(); }

LF_FORCE_INLINE Atomic16 AtomicIncrement16(volatile Atomic16* value) { return _InterlockedIncrement16(value); }
LF_FORCE_INLINE Atomic32 AtomicIncrement32(volatile Atomic32* value) { return _InterlockedIncrement(value); }
LF_FORCE_INLINE Atomic64 AtomicIncrement64(volatile Atomic64* value) { return _InterlockedIncrement64(value); }

LF_FORCE_INLINE Atomic16 AtomicDecrement16(volatile Atomic16* value) { return _InterlockedDecrement16(value); }
LF_FORCE_INLINE Atomic32 AtomicDecrement32(volatile Atomic32* value) { return _InterlockedDecrement(value); }
LF_FORCE_INLINE Atomic64 AtomicDecrement64(volatile Atomic64* value) { return _InterlockedDecrement64(value); }

LF_FORCE_INLINE Atomic16 AtomicAdd16(volatile Atomic16* value, Atomic16 amount) { return _InterlockedExchangeAdd16(value, amount); }
LF_FORCE_INLINE Atomic32 AtomicAdd32(volatile Atomic32* value, Atomic32 amount) { return _InterlockedExchangeAdd(value, amount); }
LF_FORCE_INLINE Atomic64 AtomicAdd64(volatile Atomic64* value, Atomic64 amount) { return _InterlockedExchangeAdd64(value, amount); }
LF_FORCE_INLINE Atomic16 AtomicSub16(volatile Atomic16* value, Atomic16 amount) { return _InterlockedExchangeAdd16(value, -amount); }
LF_FORCE_INLINE Atomic32 AtomicSub32(volatile Atomic32* value, Atomic32 amount) { return _InterlockedExchangeAdd(value, -amount); }
LF_FORCE_INLINE Atomic64 AtomicSub64(volatile Atomic64* value, Atomic64 amount) { return _InterlockedExchangeAdd64(value, -amount); }

LF_FORCE_INLINE void AtomicStore(void* volatile* target, const void* value)
{
    _InterlockedExchangePointer(target, const_cast<void*>(value));
}
LF_FORCE_INLINE void AtomicStore(volatile Atomic32* target, Atomic32 value)
{
    _InterlockedExchange(target, value);
}
LF_FORCE_INLINE void AtomicStore(volatile AtomicU32* target, AtomicU32 value)
{
    AtomicStore(reinterpret_cast<volatile Atomic32*>(target), reinterpret_cast<Atomic32&>(value));
}
template<typename T>
LF_FORCE_INLINE void TAtomicStore(volatile T* target, Atomic32 value)
{
    LF_STATIC_ASSERT(sizeof(T) == sizeof(Atomic32));
    AtomicStore(reinterpret_cast<volatile Atomic32*>(target), value);
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
template<typename T>
LF_FORCE_INLINE T TAtomicLoad(volatile T* target)
{
    T value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}

LF_FORCE_INLINE AtomicU32 AtomicLoad(volatile AtomicU32* target)
{
    AtomicU32 value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
LF_FORCE_INLINE AtomicU32 AtomicLoad(volatile const AtomicU32* target)
{
    AtomicU32 value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
LF_FORCE_INLINE Atomic64 AtomicLoad(volatile Atomic64* target)
{
    Atomic64 value;
    value = *target;
    _ReadWriteBarrier();
    return value;
}
LF_FORCE_INLINE Atomic64 AtomicLoad(volatile const Atomic64* target)
{
    Atomic64 value;
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
LF_FORCE_INLINE const ValueT* AtomicLoadPointer(ValueT* const volatile* target)
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

LF_FORCE_INLINE Atomic64 AtomicCompareExchange64(volatile Atomic64* target, Atomic64 value, Atomic64 compare)
{
    return _InterlockedCompareExchange64(target, value, compare);
}

template<typename ValueT>
LF_FORCE_INLINE ValueT* AtomicCompareExchangePointer(ValueT* volatile* destination, ValueT* value, ValueT* compare)
{
    void* volatile* destVoid = reinterpret_cast<void* volatile*>(destination);
    void* result = _InterlockedCompareExchangePointer(destVoid, value, compare);
    return reinterpret_cast<ValueT*>(result);
}

} // namespace lf

#else
#error "Missing implementation"
#endif

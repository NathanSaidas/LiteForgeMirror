#ifndef LF_CORE_UTILITY_H
#define LF_CORE_UTILITY_H

#include "Core/Common/Types.h"
#include <intrin.h>

namespace lf {
template<typename T>
LF_FORCE_INLINE T Max(T a, T b) { return a > b ? a : b; }

template<typename T>
LF_FORCE_INLINE T Min(T a, T b) { return a < b ? a : b; }

LF_FORCE_INLINE SizeT BitCount(UInt32 value)
{
    return static_cast<SizeT>(_mm_popcnt_u32(value));
}
LF_FORCE_INLINE SizeT BitCount(UInt64 value)
{
#if defined(LF_PLATFORM_32)
    return BitCount(static_cast<UInt32>(value)) + BitCount(static_cast<UInt32>(value >> 32));
#else
    return static_cast<SizeT>(_mm_popcnt_u64(value));
#endif
}

/** Reinterprets an address to AddressPtr*/
LF_FORCE_INLINE UIntPtrT AddressConvert(void* address)
{
    return reinterpret_cast<UIntPtrT>(address);
}

/** Advances an address the amount.*/
LF_FORCE_INLINE void* AddressAdd(void* address, SizeT amount)
{
    return reinterpret_cast<void*>(AddressConvert(address) + static_cast<UIntPtrT>(amount));
}

/** Subtracts an address the amount.*/
LF_FORCE_INLINE void* AddressSub(void* address, SizeT amount)
{
    return reinterpret_cast<void*>(AddressConvert(address) - static_cast<UIntPtrT>(amount));
}

} // namespace lf


#endif // LF_CORE_UTILITY_H 
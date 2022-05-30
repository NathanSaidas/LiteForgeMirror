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

namespace lf {
template<typename T>
LF_FORCE_INLINE constexpr const T Max(const T a, const T b) { return a > b ? a : b; }

template<typename T>
LF_FORCE_INLINE constexpr const T Min(const T a, const T b) { return a < b ? a : b; }

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
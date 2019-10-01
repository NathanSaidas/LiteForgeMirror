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
#ifndef LF_CORE_BYTE_ORDER_H
#define LF_CORE_BYTE_ORDER_H

#include "Core/Common/Types.h"

namespace lf {

// ABCD1234 < -- > 4321DCBA
//     ABCD < -- > DCBA
//       CD < -- > DC

LF_INLINE UInt64 SwapBytes(UInt64 value)
{
    return ((value >> 56) & 0x00000000000000FF)
        | ((value >> 40) & 0x000000000000FF00)
        | ((value >> 24) & 0x0000000000FF0000)
        | ((value >> 8) & 0x00000000FF000000)
        | ((value << 8) & 0x000000FF00000000)
        | ((value << 24) & 0x0000FF0000000000)
        | ((value << 40) & 0x00FF000000000000)
        | ((value << 56) & 0xFF00000000000000);
}

LF_INLINE Int64 SwapBytes(Int64 value)
{
    UInt64 result = SwapBytes(reinterpret_cast<UInt64&>(value));
    return reinterpret_cast<Int64&>(result);
}

LF_INLINE UInt32 SwapBytes(UInt32 value)
{
    return ((value >> 24) & 0x000000FF)
        | ((value >> 8) & 0x0000FF00)
        | ((value << 8) & 0x00FF0000)
        | ((value << 24) & 0xFF000000);
}

LF_INLINE Int32 SwapBytes(Int32 value)
{
    UInt32 result = SwapBytes(reinterpret_cast<UInt32&>(value));
    return reinterpret_cast<Int32&>(result);
}

LF_INLINE UInt16 SwapBytes(UInt16 value)
{
    return ((value >> 8) & 0x00FF)
        | ((value << 8) & 0xFF00);
}

LF_INLINE Int16 SwapBytes(Int16 value)
{
    UInt16 result = SwapBytes(reinterpret_cast<UInt16&>(value));
    return reinterpret_cast<Int16&>(result);
}

LF_INLINE bool IsBigEndian()
{
    union {
        UInt32 ival;
        ByteT  bval[4];
    } test = { 0x01020304 };
    return test.bval[0] == 0x01;
}

LF_INLINE bool IsLittleEndian()
{
    return !IsBigEndian();
}

}

#endif // LF_CORE_BYTE_ORDER_H
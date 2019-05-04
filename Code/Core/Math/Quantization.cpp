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
#include "Quantization.h"
#include "Core/Common/Assert.h"
#include "Core/Math/MathFunctions.h"

namespace lf {
SizeT CountDigits(SizeT value, SizeT digits)
{
    SizeT digVal = 1;
    for (SizeT i = 0; i < digits; ++i)
    {
        if (value < digVal)
        {
            return i;
        }
        digVal *= 10;
    }
    return digits;
}

SizeT ComputeMaxValue(const SizeT bits)
{
    SizeT value = 0;
    for (SizeT i = 0; i < bits; ++i)
    {
        value += (1ULL << i);
    }
    return value;
}

UInt32 SlowQuantizeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success)
{
    success = leftShift > 0 && bits >= MIN_FLOAT_BITS && bits <= MAX_FLOAT_BITS;

    // Shift the float to the left, making it hopefully an integer.
    SizeT shift;
    SizeT digits = leftShift;
    SizeT numDigits = 0;
    value = Abs(value);
    while (digits != 0)
    {
        shift = Min(MAX_FLOAT_SHIFT, digits);
        // Range Check:
        numDigits += CountDigits(static_cast<SizeT>(value), MAX_FLOAT_SHIFT);
        if (numDigits > leftShift)
        {
            success = false; // too many digits
        }

        //  Shift:
        digits -= shift;
        value *= FLT_LEFT_SHIFT[shift];
    }

    // Range Check:
    SizeT maxValue = ComputeMaxValue(bits + 2) - 3;
    if (static_cast<SizeT>(value) > maxValue)
    {
        success = false; // number to larger
        value = static_cast<Float32>(maxValue);
    }
    return (static_cast<UInt32>(value) >> 2) & QUANTIZATION_BIT_MASKS[bits];
}

Float32 SlowQuantizeDecode(UInt32 value, SizeT rightShift)
{
    Float32 unpacked = static_cast<Float32>(value << 2);
    SizeT shift;
    while (rightShift != 0)
    {
        shift = Min(MAX_FLOAT_SHIFT, rightShift);
        rightShift -= shift;
        unpacked *= FLT_RIGHT_SHIFT[shift];
    }
    return unpacked;
}

UInt32 SafeQuantizeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success)
{
    success = leftShift > 0 && leftShift <= MAX_FLOAT_SHIFT && bits >= MIN_FLOAT_BITS && bits <= MAX_FLOAT_BITS;
            
    // Range Check:
    SizeT numDigits = CountDigits(static_cast<SizeT>(value), MAX_FLOAT_SHIFT);
    if (numDigits > leftShift)
    {
        success = false; // too many digits
    }

    // Shift the float to the left, making it hopefully an integer.
    value = Abs(value);
    value *= FLT_LEFT_SHIFT[Min(MAX_FLOAT_SHIFT, leftShift)];

    // Range Check:
    SizeT maxValue = (ComputeMaxValue(bits + 2) - 3);
    if (static_cast<SizeT>(value) > maxValue)
    {
        success = false; // number to larger
        value = static_cast<Float32>(maxValue);
    }

    return (static_cast<UInt32>(value) >> 2) & QUANTIZATION_BIT_MASKS[bits];
}

Float32 SafeQuantizeDecode(UInt32 value, SizeT rightShift)
{
    return static_cast<Float32>(value << 2) * FLT_RIGHT_SHIFT[Min(MAX_FLOAT_SHIFT, rightShift)];
}

UInt32 FastQuantizeEncode(const Float32 value, const SizeT leftShift, const SizeT bits)
{
    return (static_cast<UInt32>(Abs(value) * FLT_LEFT_SHIFT[leftShift]) >> 2) & QUANTIZATION_BIT_MASKS[bits];
}

Float32 FastQuantizeDecode(const UInt32 value, const SizeT rightShift)
{
    return static_cast<Float32>(value << 2) * FLT_RIGHT_SHIFT[rightShift];
}


UInt32 SlowQuantizeRangeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success)
{
    success = leftShift > 0 && bits >= MIN_FLOAT_BITS && bits <= (MAX_FLOAT_BITS-1);

    // Shift the float to the left, making it hopefully an integer.
    UInt32 sign = (*reinterpret_cast<UInt32*>(&value)) & FLT_SIGN_BIT;
    SizeT shift;
    SizeT digits = leftShift;
    SizeT numDigits = 0;
    value = Abs(value);
    while (digits != 0)
    {
        shift = Min(MAX_FLOAT_SHIFT, digits);
        // Range Check:
        numDigits += CountDigits(static_cast<SizeT>(value), MAX_FLOAT_SHIFT);
        if (numDigits > leftShift)
        {
            success = false; // too many digits
        }

        //  Shift:
        digits -= shift;
        value *= FLT_LEFT_SHIFT[shift];
    }

    // Range Check:
    SizeT maxValue = ComputeMaxValue(bits + 1) - 3;
    if (static_cast<SizeT>(value) > maxValue)
    {
        success = false; // number to larger
        value = static_cast<Float32>(maxValue);
    }

    UInt32 result = (static_cast<UInt32>(value) >> 2) & QUANTIZATION_BIT_MASKS[bits-1];
    return result | (sign >> (32-bits));
}

Float32 SlowQuantizeRangeDecode(UInt32 value, SizeT rightShift, SizeT bits)
{
    SizeT bitMask = (1ULL << (bits -1));
    bool isNegative = (value & bitMask) == bitMask;
    value = value & (~bitMask);
    Float32 unpacked = static_cast<Float32>(value << 2);
    SizeT shift;
    while (rightShift != 0)
    {
        shift = Min(MAX_FLOAT_SHIFT, rightShift);
        rightShift -= shift;
        unpacked *= FLT_RIGHT_SHIFT[shift];
    }
    return isNegative ? -unpacked : unpacked;
}
        
UInt32 SafeQuantizeRangeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success)
{
    success = leftShift > 0 && leftShift <= MAX_FLOAT_SHIFT && bits >= MIN_FLOAT_BITS && bits <= (MAX_FLOAT_BITS-1);
        
    // Get Sign:
    UInt32 sign = (*reinterpret_cast<UInt32*>(&value)) & FLT_SIGN_BIT;
    value = Abs(value);

    // Range Check:
    SizeT numDigits = CountDigits(static_cast<SizeT>(value), MAX_FLOAT_SHIFT);
    if (numDigits > leftShift)
    {
        success = false; // too many digits
    }
        
    // Shift the float to the left, making it hopefully an integer.
    value *= FLT_LEFT_SHIFT[Min(MAX_FLOAT_SHIFT, leftShift)];
        
    // Range Check:
    SizeT maxValue = ComputeMaxValue(bits + 1) - 3;
    if (static_cast<SizeT>(value) > maxValue)
    {
        success = false;
        value = static_cast<Float32>(maxValue);
    }

    UInt32 result = (static_cast<UInt32>(value) >> 2) & QUANTIZATION_BIT_MASKS[bits - 1];
    return result | (sign >> (32 - bits));
}
        
Float32 SafeQuantizeRangeDecode(UInt32 value, SizeT rightShift, SizeT bits)
{
    SizeT bitMask = (1ULL << (bits - 1));
    bool isNegative = (value & bitMask) == bitMask;
    value = value & (~bitMask);

    Float32 unpacked = static_cast<Float32>(value << 2) * FLT_RIGHT_SHIFT[Min(MAX_FLOAT_SHIFT, rightShift)];
    return isNegative ? -unpacked : unpacked;
}
        
UInt32 FastQuantizeRangeEncode(const Float32 value, const SizeT leftShift, const SizeT bits)
{
    UInt32 sign = (*reinterpret_cast<const UInt32*>(&value)) & FLT_SIGN_BIT;
    UInt32 result = (static_cast<UInt32>(Abs(value) * FLT_LEFT_SHIFT[leftShift]) >> 2) & QUANTIZATION_BIT_MASKS[bits-1];
    return result | (sign >> (32 - bits));
}
        
Float32 FastQuantizeRangeDecode(const UInt32 value, const SizeT rightShift, const SizeT bits)
{
    SizeT bitMask = (1ULL << (bits - 1));
    bool isNegative = (value & bitMask) == bitMask;
    Float32 result = static_cast<Float32>((value & (~bitMask)) << 2) * FLT_RIGHT_SHIFT[rightShift];
    return isNegative ? -result : result;
}
} // namespace lf
#ifndef LF_CORE_QUANTIZATION_H
#define LF_CORE_QUANTIZATION_H

// **********************************
// Floating point Quantization 
// 
// There are a few non-template functions that do the actual quantization
// SlowQuantizeEncode/Decode
// SafeQuantizeEncode/Decode
// FastQuantizeEncode/Decode
//
// Encoding - When we encode/quantize floats we store them in an integer format. (Shift 
//            significant digits to left side of the decimal and truncate the right.)
//            All Encode/Decode functions produce the same result so long as success is returned 
//            true.
//
// Range Checking - This is the process of checking that the value specified fits in both
//                  the number of bits specified and less or equal of the amount of digits.
//                  It means the function will return a boolean (success/fail) value indicating
//                  whether or not truncation/clamping occured. 
// 
// Slow - This is called slow because it allows you to specify number of shifting digits 
//        greater than MAX_FLOAT_SHIFT. It is slower than other methods though because
//        it does BOTH range checks and loops.
// 
// Safe - This is called safe because it does range checks. (Note that the maximum digits 
//        that can be shifted is MAX_FLOAT_SHIFT.) 
//
// Fast - This is called fast since it has no range checks and no loops.
//
// Common Functions - Encode(value,shift, bits) Decode(value,shift)
//      @value - For encode this is the float value you want to compress 
//               For Decode its the int value you want to decompress
//
//      @shift - Must be greater than 0. If it was 0 you might as well use an integer since there
//               is no gain in quantization for floats.
//
//      @bits - Must be within MIN_FLOAT_BITS and MAX_FLOAT_BITS range. 
//              (fwiw we ignore the first 2 bits since its small error (+-3)
//
// Policy Templates - We use templates to create a "policy" which is basically a compile time 
//                    guarantee that Encode/Decode will use the same shift/bit size. (Reduce user
//                    error. )
// 
// Object Templates - We use this to make it easy to use a quantized value with non-quantized.
// 
// Casting - TQuantizeCast is provided to do casting between different Shift/Bit policies. If the Shift/Bit 
//           is the same then you can just implicitly cast the storage types. 
//           For example, if you had used Slow for error checking on shift/bit cast, you can then implicit cast
//           to fast and the decode later will be the same result.
//
// How To - Choose a number range. 
//          Specify the shift amount, if the number range requires a shift greater than MAX_FLOAT_SHIFT 
//          then you will need to use a Slow Encode/Decode policy.
//          Examples: 0.123456789 can be shifted as an integer with a shift of 9. While 1.234567891 
//                    would require 10 and 0.000000000123456789 would require 18.
//          Specify the amount of bits you will use for the encoded integer value. Remember that 
//          you gain an extra 2 bits for a +-3 value loss and QuantizeRange policies  will use 1 bit for the sign.  
//          Examples: 
//                    0.123456 requires Shift=6 and at least 15 bits.
//                    0.199999 requires Shift=6 and at least 16 bits.
//                    9.999    requires Shift=3 and at least 14 bits.
//                    123.456  requires Shift=3 and at least 15 bits.
//                    199.999  requires Shift=3 and at least 16 bits.
//                   -0.123456 requires Shift=3 and at least 16 bits
//                   -0.199999 requires Shift=3 and at least 17 bits
//          
//        Loss/Error: 
//                   0.123456 with Shift=3 and 14 bit precision would result as 0.123 when encoded/decoded. (0.000456 loss)
//                   9.999    with Shift=3 and 10 bit precision would result as 4.095 when encoded/decoded. (5.904 loss)
// 
//
//  Extra - I did try to compress mantissa/exponent but looses more precision than converting to integer would.
// **********************************

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include <cmath>

namespace lf
{
// **********************************
// Constants
// **********************************
const SizeT MAX_FLOAT_SHIFT = 9;
const SizeT MIN_FLOAT_BITS = 1;
const SizeT MAX_FLOAT_BITS = 30;
const UInt32 FLT_SIGN_BIT = (1U << 31);

const Float32 FLT_LEFT_SHIFT[] = 
{
    0.0f, // Constant 0 here to avoid digit-1 every call to shift
    std::powf(10.0f,1.0f),
    std::powf(10.0f,2.0f),
    std::powf(10.0f,3.0f),
    std::powf(10.0f,4.0f),
    std::powf(10.0f,5.0f),
    std::powf(10.0f,6.0f),
    std::powf(10.0f,7.0f),
    std::powf(10.0f,8.0f),
    std::powf(10.0f,9.0f)
};

const Float32 FLT_RIGHT_SHIFT[] =
{
    0.0f, // Constant 0 here to avoid digit-1 every call to shift
    std::powf(10.0f,-1.0f),
    std::powf(10.0f,-2.0f),
    std::powf(10.0f,-3.0f),
    std::powf(10.0f,-4.0f),
    std::powf(10.0f,-5.0f),
    std::powf(10.0f,-6.0f),
    std::powf(10.0f,-7.0f),
    std::powf(10.0f,-8.0f),
    std::powf(10.0f,-9.0f)
};

static const UInt32 QUANTIZATION_BIT_MASKS[] =
{
    0x00000000, // Constant 0 here to avoid bit-1 every call
    0x00000001, // 1
    0x00000003, // 2
    0x00000007, // 3
    0x0000000F, // 4
    0x0000001F, // 5
    0x0000003F, // 6
    0x0000007F, // 7
    0x000000FF, // 8
    0x000001FF, // 9
    0x000003FF, // 10
    0x000007FF, // 11
    0x00000FFF, // 12
    0x00001FFF, // 13
    0x00003FFF, // 14
    0x00007FFF, // 15
    0x0000FFFF, // 16
    0x0001FFFF, // 17
    0x0003FFFF, // 18
    0x0007FFFF, // 19
    0x000FFFFF, // 20
    0x001FFFFF, // 21
    0x003FFFFF, // 22
    0x007FFFFF, // 23
    0x00FFFFFF, // 24
    0x01FFFFFF, // 25
    0x03FFFFFF, // 26
    0x07FFFFFF, // 27
    0x0FFFFFFF, // 28
    0x1FFFFFFF, // 29
    0x3FFFFFFF, // 30
    0x7FFFFFFF, // 31
    0xFFFFFFFF  // 32
};


// **********************************
// Functions
// **********************************
LF_CORE_API UInt32 SlowQuantizeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success);
LF_CORE_API Float32 SlowQuantizeDecode(UInt32 value, SizeT rightShift);

LF_CORE_API UInt32 SafeQuantizeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success);
LF_CORE_API Float32 SafeQuantizeDecode(UInt32 value, SizeT rightShift);

LF_CORE_API UInt32 FastQuantizeEncode(const Float32 value, const SizeT leftShift, const SizeT bits);
LF_CORE_API Float32 FastQuantizeDecode(const UInt32 value, const SizeT rightShift);

// Signed Variants
LF_CORE_API UInt32 SlowQuantizeRangeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success);
LF_CORE_API Float32 SlowQuantizeRangeDecode(UInt32 value, SizeT rightShift, SizeT bits);
// 
LF_CORE_API UInt32 SafeQuantizeRangeEncode(Float32 value, SizeT leftShift, SizeT bits, bool& success);
LF_CORE_API Float32 SafeQuantizeRangeDecode(UInt32 value, SizeT rightShift, SizeT bits);
// 
LF_CORE_API UInt32 FastQuantizeRangeEncode(const Float32 value, const SizeT leftShift, const SizeT bits);
LF_CORE_API Float32 FastQuantizeRangeDecode(const UInt32 value, const SizeT rightShift, const SizeT bits);

// **********************************
// Policy Templates
// **********************************

// Categories
struct SlowQuantize;
struct SafeQuantize;
struct FastQuantize;
struct SlowQuantizeRange;
struct SafeQuantizeRange;
struct FastQuantizeRange;


// Default = Compile Error
template<SizeT TShift, SizeT TBits, typename TValue, typename TStorage, typename TCategory>
class TQuantizationPolicy
{
public:
    typedef TValue value_type;
    typedef TStorage storage_type;
    typedef TCategory category_type;
            
    static const SizeT SHIFT_AMOUNT = TShift;
    static const SizeT BIT_SIZE = TBits;

    LF_INLINE static storage_type Encode(value_type value, bool& success)
    {
        LF_STATIC_CRASH("Invalid category type, use SlowQuantize/SafeQuantize/FastQuantize.");
        return storage_type(0);
    }

    LF_INLINE static value_type Decode(storage_type value)
    {
        LF_STATIC_CRASH("Invalid category type, use SlowQuantize/SafeQuantize/FastQuantize.");
        return value_type(0);
    }
};

template<SizeT TShift, SizeT TBits, typename TValue, typename TStorage>
class TQuantizationPolicy<TShift, TBits, TValue, TStorage, SlowQuantize>
{
public:
    typedef TValue value_type;
    typedef TStorage storage_type;
    typedef SlowQuantize category_type;

    static const SizeT SHIFT_AMOUNT = TShift;
    static const SizeT BIT_SIZE = TBits;

    LF_INLINE static storage_type Encode(value_type value, bool& success)
    {
        return SlowQuantizeEncode(value, SHIFT_AMOUNT, BIT_SIZE, success);
    }

    LF_INLINE static value_type Decode(storage_type value)
    {
        return SlowQuantizeDecode(value, SHIFT_AMOUNT);
    }
};

template<SizeT TShift, SizeT TBits, typename TValue, typename TStorage>
class TQuantizationPolicy<TShift, TBits, TValue, TStorage, SafeQuantize>
{
public:
    typedef TValue value_type;
    typedef TStorage storage_type;
    typedef SafeQuantize category_type;

    static const SizeT SHIFT_AMOUNT = TShift;
    static const SizeT BIT_SIZE = TBits;

    LF_INLINE static storage_type Encode(value_type value, bool& success)
    {
        return SafeQuantizeEncode(value, SHIFT_AMOUNT, BIT_SIZE, success);
    }

    LF_INLINE static value_type Decode(storage_type value)
    {
        return SafeQuantizeDecode(value, SHIFT_AMOUNT);
    }
};


template<SizeT TShift, SizeT TBits, typename TValue, typename TStorage>
class TQuantizationPolicy<TShift, TBits, TValue, TStorage, FastQuantize>
{
public:
    typedef TValue value_type;
    typedef TStorage storage_type;
    typedef FastQuantize category_type;

    static const SizeT SHIFT_AMOUNT = TShift;
    static const SizeT BIT_SIZE = TBits;

    LF_INLINE static storage_type Encode(value_type value)
    {
        return FastQuantizeEncode(value, SHIFT_AMOUNT, BIT_SIZE);
    }

    LF_INLINE static value_type Decode(storage_type value)
    {
        return FastQuantizeDecode(value, SHIFT_AMOUNT);
    }
};

template<SizeT TShift, SizeT TBits, typename TValue, typename TStorage>
class TQuantizationPolicy<TShift, TBits, TValue, TStorage, SlowQuantizeRange>
{
public:
    typedef TValue value_type;
    typedef TStorage storage_type;
    typedef SlowQuantizeRange category_type;

    static const SizeT SHIFT_AMOUNT = TShift;
    static const SizeT BIT_SIZE = TBits;

    LF_INLINE static storage_type Encode(value_type value, bool& success)
    {
        return SlowQuantizeRangeEncode(value, SHIFT_AMOUNT, BIT_SIZE, success);
    }

    LF_INLINE static value_type Decode(storage_type value)
    {
        return SlowQuantizeRangeDecode(value, SHIFT_AMOUNT, BIT_SIZE);
    }
};

template<SizeT TShift, SizeT TBits, typename TValue, typename TStorage>
class TQuantizationPolicy<TShift, TBits, TValue, TStorage, SafeQuantizeRange>
{
public:
    typedef TValue value_type;
    typedef TStorage storage_type;
    typedef SafeQuantizeRange category_type;

    static const SizeT SHIFT_AMOUNT = TShift;
    static const SizeT BIT_SIZE = TBits;

    LF_INLINE static storage_type Encode(value_type value, bool& success)
    {
        return SafeQuantizeRangeEncode(value, SHIFT_AMOUNT, BIT_SIZE, success);
    }

    LF_INLINE static value_type Decode(storage_type value)
    {
        return SafeQuantizeRangeDecode(value, SHIFT_AMOUNT, BIT_SIZE);
    }
};

template<SizeT TShift, SizeT TBits, typename TValue, typename TStorage>
class TQuantizationPolicy<TShift, TBits, TValue, TStorage, FastQuantizeRange>
{
public:
    typedef TValue value_type;
    typedef TStorage storage_type;
    typedef FastQuantizeRange category_type;

    static const SizeT SHIFT_AMOUNT = TShift;
    static const SizeT BIT_SIZE = TBits;

    LF_INLINE static storage_type Encode(value_type value)
    {
        return FastQuantizeRangeEncode(value, SHIFT_AMOUNT, BIT_SIZE);
    }

    LF_INLINE static value_type Decode(storage_type value)
    {
        return FastQuantizeRangeDecode(value, SHIFT_AMOUNT, BIT_SIZE);
    }
};

// For most types we can assume we want Float32/UInt32 these are "typedefs" so we can do shorthand
// shift / bits specification.

template<SizeT TShift, SizeT TBits>
class TSlowQuantizationPolicy : public TQuantizationPolicy<TShift, TBits, Float32, UInt32, SlowQuantize>
{
};

template<SizeT TShift, SizeT TBits>
class TSafeQuantizationPolicy : public TQuantizationPolicy<TShift, TBits, Float32, UInt32, SafeQuantize>
{
};

template<SizeT TShift, SizeT TBits>
class TFastQuantizationPolicy : public TQuantizationPolicy<TShift, TBits, Float32, UInt32, FastQuantize>
{
};

template<SizeT TShift, SizeT TBits>
class TSlowQuantizationRangePolicy : public TQuantizationPolicy<TShift, TBits, Float32, UInt32, SlowQuantizeRange>
{
};

template<SizeT TShift, SizeT TBits>
class TSafeQuantizationRangePolicy : public TQuantizationPolicy<TShift, TBits, Float32, UInt32, SafeQuantizeRange>
{
};

template<SizeT TShift, SizeT TBits>
class TFastQuantizationRangePolicy : public TQuantizationPolicy<TShift, TBits, Float32, UInt32, FastQuantizeRange>
{
};

// **********************************
// Object Templates
// **********************************


// This is intentionally not implemented. Only category ones are.
// The methods/operators do match across all however!
template<typename TPolicy, typename TCategory>
class TQuantizedFloat
{
public:
    typedef TPolicy policy;
    typedef typename TPolicy::value_type value_type;
    typedef typename TPolicy::storage_type storage_type;
    typedef typename TPolicy::category_type category_type;

    TQuantizedFloat();
    TQuantizedFloat(const TQuantizedFloat& other);
    TQuantizedFloat(TQuantizedFloat&& other);
    TQuantizedFloat(value_type value);
    ~TQuantizedFloat();

    value_type GetValue() const;
    bool HasError() const;
    // operators

    // assign
    TQuantizedFloat& operator=(const TQuantizedFloat& other);
    TQuantizedFloat& operator=(value_type value);
    TQuantizedFloat& operator+=(const TQuantizedFloat& other);
    TQuantizedFloat& operator+=(value_type value);
    TQuantizedFloat& operator-=(const TQuantizedFloat& other);
    TQuantizedFloat& operator-=(value_type value);
    TQuantizedFloat& operator*=(const TQuantizedFloat& other);
    TQuantizedFloat& operator*=(value_type value);
    TQuantizedFloat& operator/=(const TQuantizedFloat& other);
    TQuantizedFloat& operator/=(value_type value);

    // arithmetic
    TQuantizedFloat operator+(const TQuantizedFloat& other) const;
    TQuantizedFloat operator+(value_type value) const;
    TQuantizedFloat operator-(const TQuantizedFloat& other) const;
    TQuantizedFloat operator-(value_type value);
    TQuantizedFloat operator*(const TQuantizedFloat& other) const;
    TQuantizedFloat operator*(value_type value);
    TQuantizedFloat operator/(const TQuantizedFloat& other) const;
    TQuantizedFloat operator/(value_type value) const;

    // logical
    bool operator==(const TQuantizedFloat& other) const;
    bool operator==(value_type value) const;
    bool operator!=(const TQuantizedFloat& other) const;
    bool operator!=(value_type value) const;
    bool operator>(const TQuantizedFloat& other) const;
    bool operator>(value_type value) const;
    bool operator<(const TQuantizedFloat& other) const;
    bool operator<(value_type value) const;
    bool operator>=(const TQuantizedFloat& other) const;
    bool operator>=(value_type value) const;
    bool operator<=(const TQuantizedFloat& other) const;
    bool operator<=(value_type value) const;
};

template<typename TPolicy>
class TQuantizedFloat<TPolicy, SlowQuantize>
{
public:
    typedef TPolicy policy;
    typedef typename TPolicy::value_type value_type;
    typedef typename TPolicy::storage_type storage_type;
    typedef typename TPolicy::category_type category_type;

    TQuantizedFloat() : 
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess(false)
    {}

    TQuantizedFloat(const TQuantizedFloat& other) :
        mValue(other.mValue),
#if defined(LF_DEBUG)
        mDebugValue(other.mDebugValue),
#endif
        mSuccess(other.mSuccess)
    {}

    TQuantizedFloat(TQuantizedFloat&& other) :
        mValue(std::move(other.mValue)),
#if defined(LF_DEBUG)
        mDebugValue(std::move(other.mDebugValue)),
#endif
        mSuccess(std::move(other.mSuccess))
    {}

    TQuantizedFloat(value_type value) :
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess()
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
    }

    ~TQuantizedFloat() { }

    value_type GetValue() const
    {
        return policy::Decode(mValue);
    }

    bool HasError() const
    {
        return !mSuccess;
    }

    // operators

    // assign
    TQuantizedFloat& operator=(TQuantizedFloat&& other)
    {
        mValue = std::move(other.mValue);
        mSuccess = std::move(other.mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = std::move(other.mDebugValue);
#endif
        return *this;
    }

    TQuantizedFloat& operator=(const TQuantizedFloat& other)
    {
        mValue = other.mValue;
        mSuccess = other.mSuccess;
#if defined(LF_DEBUG)
        mDebugValue = other.mDebugValue;
#endif
        return *this;
    }

    TQuantizedFloat& operator=(value_type value)
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) + value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) - value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) * value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) / value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }

    // arithmetic
    TQuantizedFloat operator+(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator+(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }

    // logical
    bool operator==(const TQuantizedFloat& other) const
    {
        return mValue == other.mValue;
    }
    bool operator==(value_type value) const
    {
        return policy::Decode(mValue) == value;
    }
    bool operator!=(const TQuantizedFloat& other) const
    {
        return mValue != other.mValue;
    }
    bool operator!=(value_type value) const
    {
        return policy::Decode(mValue) != value;
    }
    bool operator>(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) > policy::Decode(other.mValue);
    }
    bool operator>(value_type value) const
    {
        return policy::Decode(mValue) > value;
    }
    bool operator<(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) < policy::Decode(other.mValue);
    }
    bool operator<(value_type value) const
    {
        return policy::Decode(mValue) < value;
    }
    bool operator>=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) >= policy::Decode(other.mValue);
    }
    bool operator>=(value_type value) const
    {
        return policy::Decode(mValue) >= value;
    }
    bool operator<=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) <= policy::Decode(other.mValue);
    }
    bool operator<=(value_type value) const
    {
        return policy::Decode(mValue) <= value;
    }

private:
    storage_type mValue;
#if defined(LF_DEBUG)
    value_type mDebugValue;
#endif
    bool mSuccess;
};

template<typename TPolicy>
class TQuantizedFloat<TPolicy, SafeQuantize>
{
public:
    typedef TPolicy policy;
    typedef typename TPolicy::value_type value_type;
    typedef typename TPolicy::storage_type storage_type;
    typedef typename TPolicy::category_type category_type;

    TQuantizedFloat() :
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess(true)
    {}

    TQuantizedFloat(const TQuantizedFloat& other) :
        mValue(other.mValue),
#if defined(LF_DEBUG)
        mDebugValue(other.mDebugValue),
#endif
        mSuccess(other.mSuccess)
    {}

    TQuantizedFloat(TQuantizedFloat&& other) :
        mValue(std::move(other.mValue)),
#if defined(LF_DEBUG)
        mDebugValue(std::move(other.mDebugValue)),
#endif
        mSuccess(std::move(other.mSuccess))
    {}

    TQuantizedFloat(value_type value) :
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess()
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
    }

    ~TQuantizedFloat() { }

    value_type GetValue() const
    {
        return policy::Decode(mValue);
    }

    bool HasError() const
    {
        return !mSuccess;
    }

    // operators

    // assign
    TQuantizedFloat& operator=(TQuantizedFloat&& other)
    {
        mValue = std::move(other.mValue);
        mSuccess = std::move(other.mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = std::move(other.mDebugValue);
#endif
        return *this;
    }

    TQuantizedFloat& operator=(const TQuantizedFloat& other)
    {
        mValue = other.mValue;
        mSuccess = other.mSuccess;
#if defined(LF_DEBUG)
        mDebugValue = other.mDebugValue;
#endif
        return *this;
    }

    TQuantizedFloat& operator=(value_type value)
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) + value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) - value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) * value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) / value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }

    // arithmetic
    TQuantizedFloat operator+(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator+(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }

    // logical
    bool operator==(const TQuantizedFloat& other) const
    {
        return mValue == other.mValue;
    }
    bool operator==(value_type value) const
    {
        return policy::Decode(mValue) == value;
    }
    bool operator!=(const TQuantizedFloat& other) const
    {
        return mValue != other.mValue;
    }
    bool operator!=(value_type value) const
    {
        return policy::Decode(mValue) != value;
    }
    bool operator>(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) > policy::Decode(other.mValue);
    }
    bool operator>(value_type value) const
    {
        return policy::Decode(mValue) > value;
    }
    bool operator<(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) < policy::Decode(other.mValue);
    }
    bool operator<(value_type value) const
    {
        return policy::Decode(mValue) < value;
    }
    bool operator>=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) >= policy::Decode(other.mValue);
    }
    bool operator>=(value_type value) const
    {
        return policy::Decode(mValue) >= value;
    }
    bool operator<=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) <= policy::Decode(other.mValue);
    }
    bool operator<=(value_type value) const
    {
        return policy::Decode(mValue) <= value;
    }

private:
    storage_type mValue;
#if defined(LF_DEBUG)
    value_type mDebugValue;
#endif
    bool mSuccess;
};

template<typename TPolicy>
class TQuantizedFloat<TPolicy, FastQuantize>
{
public:
    typedef TPolicy policy;
    typedef typename TPolicy::value_type value_type;
    typedef typename TPolicy::storage_type storage_type;
    typedef typename TPolicy::category_type category_type;

    TQuantizedFloat() :
        mValue()
#if defined(LF_DEBUG)
        ,mDebugValue()
#endif
    {}

    TQuantizedFloat(const TQuantizedFloat& other) :
        mValue(other.mValue)
#if defined(LF_DEBUG)
        ,mDebugValue(other.mDebugValue)
#endif
    {}

    TQuantizedFloat(TQuantizedFloat&& other) :
        mValue(std::move(other.mValue))
#if defined(LF_DEBUG)
        ,mDebugValue(std::move(other.mDebugValue))
#endif
    {}

    TQuantizedFloat(value_type value) :
        mValue()
#if defined(LF_DEBUG)
        ,mDebugValue()
#endif
    {
        mValue = policy::Encode(value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
    }

    ~TQuantizedFloat() { }

    value_type GetValue() const
    {
        return policy::Decode(mValue);
    }

    bool HasError() const
    {
        return false;
    }

    // operators

    // assign
    TQuantizedFloat& operator=(TQuantizedFloat&& other)
    {
        mValue = std::move(other.mValue);
#if defined(LF_DEBUG)
        mDebugValue = std::move(other.mDebugValue);
#endif
        return *this;
    }

    TQuantizedFloat& operator=(const TQuantizedFloat& other)
    {
        mValue = other.mValue;
#if defined(LF_DEBUG)
        mDebugValue = other.mDebugValue;
#endif
        return *this;
    }

    TQuantizedFloat& operator=(value_type value)
    {
        mValue = policy::Encode(value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) + value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) - value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) * value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) / value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }

    // arithmetic
    TQuantizedFloat operator+(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator+(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }

    // logical
    bool operator==(const TQuantizedFloat& other) const
    {
        return mValue == other.mValue;
    }
    bool operator==(value_type value) const
    {
        return policy::Decode(mValue) == value;
    }
    bool operator!=(const TQuantizedFloat& other) const
    {
        return mValue != other.mValue;
    }
    bool operator!=(value_type value) const
    {
        return policy::Decode(mValue) != value;
    }
    bool operator>(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) > policy::Decode(other.mValue);
    }
    bool operator>(value_type value) const
    {
        return policy::Decode(mValue) > value;
    }
    bool operator<(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) < policy::Decode(other.mValue);
    }
    bool operator<(value_type value) const
    {
        return policy::Decode(mValue) < value;
    }
    bool operator>=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) >= policy::Decode(other.mValue);
    }
    bool operator>=(value_type value) const
    {
        return policy::Decode(mValue) >= value;
    }
    bool operator<=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) <= policy::Decode(other.mValue);
    }
    bool operator<=(value_type value) const
    {
        return policy::Decode(mValue) <= value;
    }

private:
    storage_type mValue;
#if defined(LF_DEBUG)
    value_type mDebugValue;
#endif
};

template<typename TPolicy>
class TQuantizedFloat<TPolicy, SlowQuantizeRange>
{
public:
    typedef TPolicy policy;
    typedef typename TPolicy::value_type value_type;
    typedef typename TPolicy::storage_type storage_type;
    typedef typename TPolicy::category_type category_type;

    TQuantizedFloat() :
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess(true)
    {}

    TQuantizedFloat(const TQuantizedFloat& other) :
        mValue(other.mValue),
#if defined(LF_DEBUG)
        mDebugValue(other.mDebugValue),
#endif
        mSuccess(other.mSuccess)
    {}

    TQuantizedFloat(TQuantizedFloat&& other) :
        mValue(std::move(other.mValue)),
#if defined(LF_DEBUG)
        mDebugValue(std::move(other.mDebugValue)),
#endif
        mSuccess(std::move(other.mSuccess))
    {}

    TQuantizedFloat(value_type value) :
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess()
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
    }

    ~TQuantizedFloat() { }

    value_type GetValue() const
    {
        return policy::Decode(mValue);
    }

    bool HasError() const
    {
        return !mSuccess;
    }

    // operators

    // assign
    TQuantizedFloat& operator=(TQuantizedFloat&& other)
    {
        mValue = std::move(other.mValue);
        mSuccess = std::move(other.mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = std::move(other.mDebugValue);
#endif
        return *this;
    }

    TQuantizedFloat& operator=(const TQuantizedFloat& other)
    {
        mValue = other.mValue;
        mSuccess = other.mSuccess;
#if defined(LF_DEBUG)
        mDebugValue = other.mDebugValue;
#endif
        return *this;
    }

    TQuantizedFloat& operator=(value_type value)
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) + value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) - value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) * value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) / value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }

    // arithmetic
    TQuantizedFloat operator+(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator+(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }

    // logical
    bool operator==(const TQuantizedFloat& other) const
    {
        return mValue == other.mValue;
    }
    bool operator==(value_type value) const
    {
        return policy::Decode(mValue) == value;
    }
    bool operator!=(const TQuantizedFloat& other) const
    {
        return mValue != other.mValue;
    }
    bool operator!=(value_type value) const
    {
        return policy::Decode(mValue) != value;
    }
    bool operator>(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) > policy::Decode(other.mValue);
    }
    bool operator>(value_type value) const
    {
        return policy::Decode(mValue) > value;
    }
    bool operator<(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) < policy::Decode(other.mValue);
    }
    bool operator<(value_type value) const
    {
        return policy::Decode(mValue) < value;
    }
    bool operator>=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) >= policy::Decode(other.mValue);
    }
    bool operator>=(value_type value) const
    {
        return policy::Decode(mValue) >= value;
    }
    bool operator<=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) <= policy::Decode(other.mValue);
    }
    bool operator<=(value_type value) const
    {
        return policy::Decode(mValue) <= value;
    }

private:
    storage_type mValue;
#if defined(LF_DEBUG)
    value_type mDebugValue;
#endif
    bool mSuccess;
};

template<typename TPolicy>
class TQuantizedFloat<TPolicy, SafeQuantizeRange>
{
public:
    typedef TPolicy policy;
    typedef typename TPolicy::value_type value_type;
    typedef typename TPolicy::storage_type storage_type;
    typedef typename TPolicy::category_type category_type;

    TQuantizedFloat() :
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess(true)
    {}

    TQuantizedFloat(const TQuantizedFloat& other) :
        mValue(other.mValue),
#if defined(LF_DEBUG)
        mDebugValue(other.mDebugValue),
#endif
        mSuccess(other.mSuccess)
    {}

    TQuantizedFloat(TQuantizedFloat&& other) :
        mValue(std::move(other.mValue)),
#if defined(LF_DEBUG)
        mDebugValue(std::move(other.mDebugValue)),
#endif
        mSuccess(std::move(other.mSuccess))
    {}

    TQuantizedFloat(value_type value) :
        mValue(),
#if defined(LF_DEBUG)
        mDebugValue(),
#endif
        mSuccess()
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
    }

    ~TQuantizedFloat() { }

    value_type GetValue() const
    {
        return policy::Decode(mValue);
    }

    bool HasError() const
    {
        return !mSuccess;
    }

    // operators

    // assign
    TQuantizedFloat& operator=(TQuantizedFloat&& other)
    {
        mValue = std::move(other.mValue);
        mSuccess = std::move(other.mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = std::move(other.mDebugValue);
#endif
        return *this;
    }

    TQuantizedFloat& operator=(const TQuantizedFloat& other)
    {
        mValue = other.mValue;
        mSuccess = other.mSuccess;
#if defined(LF_DEBUG)
        mDebugValue = other.mDebugValue;
#endif
        return *this;
    }

    TQuantizedFloat& operator=(value_type value)
    {
        mValue = policy::Encode(value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) + value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) - value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) * value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) / value, mSuccess);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }

    // arithmetic
    TQuantizedFloat operator+(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator+(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue), tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / value, tmp.mSuccess);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }

    // logical
    bool operator==(const TQuantizedFloat& other) const
    {
        return mValue == other.mValue;
    }
    bool operator==(value_type value) const
    {
        return policy::Decode(mValue) == value;
    }
    bool operator!=(const TQuantizedFloat& other) const
    {
        return mValue != other.mValue;
    }
    bool operator!=(value_type value) const
    {
        return policy::Decode(mValue) != value;
    }
    bool operator>(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) > policy::Decode(other.mValue);
    }
    bool operator>(value_type value) const
    {
        return policy::Decode(mValue) > value;
    }
    bool operator<(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) < policy::Decode(other.mValue);
    }
    bool operator<(value_type value) const
    {
        return policy::Decode(mValue) < value;
    }
    bool operator>=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) >= policy::Decode(other.mValue);
    }
    bool operator>=(value_type value) const
    {
        return policy::Decode(mValue) >= value;
    }
    bool operator<=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) <= policy::Decode(other.mValue);
    }
    bool operator<=(value_type value) const
    {
        return policy::Decode(mValue) <= value;
    }

private:
    storage_type mValue;
#if defined(LF_DEBUG)
    value_type mDebugValue;
#endif
    bool mSuccess;
};

template<typename TPolicy>
class TQuantizedFloat<TPolicy, FastQuantizeRange>
{
public:
    typedef TPolicy policy;
    typedef typename TPolicy::value_type value_type;
    typedef typename TPolicy::storage_type storage_type;
    typedef typename TPolicy::category_type category_type;

    TQuantizedFloat() :
        mValue()
#if defined(LF_DEBUG)
        , mDebugValue()
#endif
    {}

    TQuantizedFloat(const TQuantizedFloat& other) :
        mValue(other.mValue)
#if defined(LF_DEBUG)
        , mDebugValue(other.mDebugValue)
#endif
    {}

    TQuantizedFloat(TQuantizedFloat&& other) :
        mValue(std::move(other.mValue))
#if defined(LF_DEBUG)
        , mDebugValue(std::move(other.mDebugValue))
#endif
    {}

    TQuantizedFloat(value_type value) :
        mValue()
#if defined(LF_DEBUG)
        , mDebugValue()
#endif
    {
        mValue = policy::Encode(value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
    }

    ~TQuantizedFloat() { }

    value_type GetValue() const
    {
        return policy::Decode(mValue);
    }

    bool HasError() const
    {
        return false;
    }

    // operators

    // assign
    TQuantizedFloat& operator=(TQuantizedFloat&& other)
    {
        mValue = std::move(other.mValue);
#if defined(LF_DEBUG)
        mDebugValue = std::move(other.mDebugValue);
#endif
        return *this;
    }

    TQuantizedFloat& operator=(const TQuantizedFloat& other)
    {
        mValue = other.mValue;
#if defined(LF_DEBUG)
        mDebugValue = other.mDebugValue;
#endif
        return *this;
    }

    TQuantizedFloat& operator=(value_type value)
    {
        mValue = policy::Encode(value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator+=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) + value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator-=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) - value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator*=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) * value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(const TQuantizedFloat& other)
    {
        mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }
    TQuantizedFloat& operator/=(value_type value)
    {
        mValue = policy::Encode(policy::Decode(mValue) / value);
#if defined(LF_DEBUG)
        mDebugValue = policy::Decode(mValue);
#endif
        return *this;
    }

    // arithmetic
    TQuantizedFloat operator+(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator+(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) + value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator-(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) - value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator*(value_type value)
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) * value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(const TQuantizedFloat& other) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / policy::Decode(other.mValue));
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }
    TQuantizedFloat operator/(value_type value) const
    {
        TQuantizedFloat tmp;
        tmp.mValue = policy::Encode(policy::Decode(mValue) / value);
#if defined(LF_DEBUG)
        tmp.mDebugValue = policy::Decode(tmp.mValue);
#endif
        return tmp;
    }

    // logical
    bool operator==(const TQuantizedFloat& other) const
    {
        return mValue == other.mValue;
    }
    bool operator==(value_type value) const
    {
        return policy::Decode(mValue) == value;
    }
    bool operator!=(const TQuantizedFloat& other) const
    {
        return mValue != other.mValue;
    }
    bool operator!=(value_type value) const
    {
        return policy::Decode(mValue) != value;
    }
    bool operator>(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) > policy::Decode(other.mValue);
    }
    bool operator>(value_type value) const
    {
        return policy::Decode(mValue) > value;
    }
    bool operator<(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) < policy::Decode(other.mValue);
    }
    bool operator<(value_type value) const
    {
        return policy::Decode(mValue) < value;
    }
    bool operator>=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) >= policy::Decode(other.mValue);
    }
    bool operator>=(value_type value) const
    {
        return policy::Decode(mValue) >= value;
    }
    bool operator<=(const TQuantizedFloat& other) const
    {
        return policy::Decode(mValue) <= policy::Decode(other.mValue);
    }
    bool operator<=(value_type value) const
    {
        return policy::Decode(mValue) <= value;
    }

private:
    storage_type mValue;
#if defined(LF_DEBUG)
    value_type mDebugValue;
#endif
};

template<typename TPolicy>
using TSlowQuantizedFloat = TQuantizedFloat<TPolicy, SlowQuantize>;

template<typename TPolicy>
using TSafeQuantizedFloat = TQuantizedFloat<TPolicy, SafeQuantize>;

template<typename TPolicy>
using TFastQuantizedFloat = TQuantizedFloat<TPolicy, FastQuantize>;

template<typename TPolicy>
using TSlowQuantizedRangeFloat = TQuantizedFloat<TPolicy, SlowQuantizeRange>;

template<typename TPolicy>
using TSafeQuantizedRangeFloat = TQuantizedFloat<TPolicy, SafeQuantizeRange>;

template<typename TPolicy>
using TFastQuantizedRangeFloat = TQuantizedFloat<TPolicy, FastQuantizeRange>;

// Convert from one format to another, assumes value_types are equal
// Decode src storage => src value_type 
// Encode dest value_type => dest storage_type
template<typename TDest, typename TSrc>
typename TDest::storage_type TQuantizeCast(typename TSrc::storage_type value)
{
    // Assume TDest category = fast, so if you get a compile error make sure you're using the right overloaded function.
    typename TSrc::value_type decoded = TSrc::Decode(value);
    return TDest::Encode(decoded);
}

template<typename TDest, typename TSrc>
typename TDest::storage_type TQuantizeCast(typename TSrc::storage_type value, bool& success)
{
    typename TSrc::value_type decoded = TSrc::Decode(value);
    return TDest::Encode(decoded, success);
}

// **********************************
// Common Typedefs
// - Debug uses "Safe" by default, otherwise we use "Fast" 
// **********************************

// Suggested Range [+- 0-1] with 5 decimal places of precision [0.99999]
// Max Value Unsigned 262140 / Signed 131068
typedef TSlowQuantizationPolicy<6, 16> SlowQuantize6_16;
typedef TSafeQuantizationPolicy<6, 16> SafeQuantize6_16;
typedef TFastQuantizationPolicy<6, 16> FastQuantize6_16;
typedef TSlowQuantizationRangePolicy<6, 16> SlowQuantizeRange6_16;
typedef TSafeQuantizationRangePolicy<6, 16> SafeQuantizeRange6_16;
typedef TFastQuantizationRangePolicy<6, 16> FastQuantizeRange6_16;
typedef TSlowQuantizedFloat<SlowQuantize6_16> SlowUFloat6_16;
typedef TSafeQuantizedFloat<SafeQuantize6_16> SafeUFloat6_16;
typedef TFastQuantizedFloat<FastQuantize6_16> FastUFloat6_16;
typedef TSlowQuantizedRangeFloat<SlowQuantizeRange6_16> SlowFloat6_16;
typedef TSafeQuantizedRangeFloat<SafeQuantizeRange6_16> SafeFloat6_16;
typedef TFastQuantizedRangeFloat<FastQuantizeRange6_16> FastFloat6_16;
#if defined(LF_DEBUG)
typedef SafeQuantize6_16            Quantize6_16;
typedef SafeQuantizeRange6_16       QuantizeRange6_16;
#else 
typedef FastQuantize6_16                Quantize6_16;
typedef FastQuantizeRange6_16           QuantizeRange6_16;
#endif
typedef TQuantizedFloat<Quantize6_16, typename Quantize6_16::category_type> UFloat6_16;
typedef TQuantizedFloat<QuantizeRange6_16, typename QuantizeRange6_16::category_type> Float6_16;

// Suggested Range[+- 0-999] with 3 decimal places of precision. [99.999]
// Max Value Unsigned 262140 / Signed 131068
typedef TSlowQuantizationPolicy<3, 16> SlowQuantize3_16;
typedef TSafeQuantizationPolicy<3, 16> SafeQuantize3_16;
typedef TFastQuantizationPolicy<3, 16> FastQuantize3_16;
typedef TSlowQuantizationRangePolicy<3, 16> SlowQuantizeRange3_16;
typedef TSafeQuantizationRangePolicy<3, 16> SafeQuantizeRange3_16;
typedef TFastQuantizationRangePolicy<3, 16> FastQuantizeRange3_16;
typedef TSlowQuantizedFloat<SlowQuantize3_16> SlowUFloat3_16;
typedef TSafeQuantizedFloat<SafeQuantize3_16> SafeUFloat3_16;
typedef TFastQuantizedFloat<FastQuantize3_16> FastUFloat3_16;
typedef TSlowQuantizedRangeFloat<SlowQuantizeRange3_16> SlowFloat3_16;
typedef TSafeQuantizedRangeFloat<SafeQuantizeRange3_16> SafeFloat3_16;
typedef TFastQuantizedRangeFloat<FastQuantizeRange3_16> FastFloat3_16;

#if defined(LF_DEBUG)
typedef SafeQuantize3_16            Quantize3_16;
typedef SafeQuantizeRange3_16       QuantizeRange3_16;
#else 
typedef FastQuantize3_16                Quantize3_16;
typedef FastQuantizeRange3_16           QuantizeRange3_16;
#endif
typedef TQuantizedFloat<Quantize3_16, typename Quantize3_16::category_type> UFloat3_16;
typedef TQuantizedFloat<QuantizeRange3_16, typename QuantizeRange3_16::category_type> Float3_16;

// Suggested Range[+ 0-1] with 3 decimal places of precision. [0.999]
// Max Value Unsigned 1020 / Signed = 508
typedef TSlowQuantizationPolicy<3, 8> SlowQuantize3_8;
typedef TSafeQuantizationPolicy<3, 8> SafeQuantize3_8;
typedef TFastQuantizationPolicy<3, 8> FastQuantize3_8;
typedef TSlowQuantizationRangePolicy<3, 8> SlowQuantizeRange3_8;
typedef TSafeQuantizationRangePolicy<3, 8> SafeQuantizeRange3_8;
typedef TFastQuantizationRangePolicy<3, 8> FastQuantizeRange3_8;
typedef TSlowQuantizedFloat<SlowQuantize3_8> SlowUFloat3_8;
typedef TSafeQuantizedFloat<SafeQuantize3_8> SafeUFloat3_8;
typedef TFastQuantizedFloat<FastQuantize3_8> FastUFloat3_8;
typedef TSlowQuantizedRangeFloat<SlowQuantizeRange3_8> SlowFloat3_8;
typedef TSafeQuantizedRangeFloat<SafeQuantizeRange3_8> SafeFloat3_8;
typedef TFastQuantizedRangeFloat<FastQuantizeRange3_8> FastFloat3_8;

#if defined(LF_DEBUG)
typedef SafeQuantize3_8            Quantize3_8;
typedef SafeQuantizeRange3_8       QuantizeRange3_8;
#else 
typedef FastQuantize3_8                Quantize3_8;
typedef FastQuantizeRange3_8           QuantizeRange3_8;
#endif
typedef TQuantizedFloat<Quantize3_8, typename Quantize3_8::category_type> UFloat3_8;
typedef TQuantizedFloat<QuantizeRange3_8, typename QuantizeRange3_8::category_type> Float3_8;

// Non-Traditional "half"
typedef Float3_16 Float16; 

typedef UFloat3_8 Float8;
}

#endif // LF_CORE_QUANTIZATION_H

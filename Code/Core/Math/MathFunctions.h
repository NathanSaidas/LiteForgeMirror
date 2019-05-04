#ifndef LF_CORE_MATH_FUNCTIONS_H
#define LF_CORE_MATH_FUNCTIONS_H

#include "Core/Common/Types.h"
#include "Core/Utility/Utility.h"
#include <cmath>

namespace lf {
const Float32 EPSILON = 0.00000000000001f;
const Float64 PI = 3.141592653589793238462643383279502884197169399375105820974944592307816406286;
const Float64 EULER_CONST = 2.71828182845904523536;
const Float32 FLT_PI = 3.1415926535897932384626433832795028841971693993f;

template<typename T>
LF_FORCE_INLINE T Abs(T value) { return value < T(0) ? -value : value; }

template<typename T>
LF_FORCE_INLINE T Sqr(T value) { return value * value; }

template<typename T>
LF_FORCE_INLINE T Clamp(T value, T min, T max)
{
    return value > max ? max : value < min ? min : value;
}

LF_FORCE_INLINE Float32 Deg2Rad(const Float32 degrees)
{
    return degrees * FLT_PI / 180.0f;
}

LF_FORCE_INLINE Float32 Rad2Deg(const Float32 radians)
{
    return radians * 180.0f / FLT_PI;
}

LF_FORCE_INLINE Float32 Lerp(Float32 a, Float32 b, Float32 t)
{
    return a + (b - a) * t;
}

LF_FORCE_INLINE Float32 InverseLerp(Float32 a, Float32 b, Float32 n)
{
    // Solve for T (Inverse Lerp)
    // (n - a) / (b-a)
    // (6 - 5) / (7-5)
    return (n - a) / (b - a);
}

LF_FORCE_INLINE bool ApproxEquals(Float32 a, Float32 b, Float32 epsilon = 0.0000000001f)
{
    return Abs(a - b) <= epsilon;
}

LF_FORCE_INLINE bool ApproxEquals(Float64 a, Float64 b, Float64 epsilon = 0.00000000001)
{
    return Abs(a - b) <= epsilon;
}

LF_FORCE_INLINE void Index2Coord(Int32 index, Int32 width, Int32& x, Int32& y)
{
    x = index % width;
    y = index / width;
}

LF_FORCE_INLINE void Coord2Index(Int32 x, Int32 y, Int32 width, Int32& index)
{
    index = x + width * y;
}

LF_FORCE_INLINE Int32 NextPow2(Int32 x)
{
    if (x < 0)
    {
        return 0;
    }
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

LF_FORCE_INLINE size_t NextPow2(size_t x)
{
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x + 1;
}

LF_FORCE_INLINE SizeT NextMultiple(SizeT value, SizeT mult)
{
    if (mult == 0)
    {
        return value;
    }
    SizeT remainder = value % mult;
    if (remainder == 0)
    {
        return value;
    }
    return value + mult - remainder;
}

LF_FORCE_INLINE SizeT Round(Float32 value)
{
    return static_cast<size_t>(floorf(value + 0.5f));
}
LF_FORCE_INLINE SizeT Round(Float64 value)
{
    return static_cast<SizeT>(floor(value + 0.5));
}

LF_FORCE_INLINE Float32 InverseSqrt(Float32 value)
{
    Float32 x2 = value * 0.5f;
    Float32 y = value;
    UInt32 i = *reinterpret_cast<UInt32*>(&value);
    i = 0x5F3759DF - (i >> 1);
    y = *reinterpret_cast<Float32*>(&i);

    // Newton steps
    y = y * (1.5f - (x2 * y * y));
    y = y * (1.5f - (x2 * y * y));
    return y;
}
} // namespace lf
#endif // LF_CORE_MATH_FUNCTIONS_H
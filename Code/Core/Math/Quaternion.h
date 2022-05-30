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
#include "Core/Math/MathFunctions.h"
#include "Core/Math/Vector.h"

#include <math.h>
#include <string.h>
#include <limits>

namespace lf
{
    

    class LF_CORE_API Quaternion
    {
    public:
        typedef Quaternion quaternion_type;
        typedef Vector vector_type;
        typedef Float32 scalar_type;

        LF_INLINE Quaternion();
        LF_INLINE Quaternion(const Quaternion& other);
        LF_INLINE Quaternion(Quaternion&& other);
        LF_INLINE Quaternion(LazyTag);
        LF_INLINE Quaternion(Float32 x, Float32 y, Float32 z, Float32 w);
        LF_INLINE Quaternion(Float32 eulerX, Float32 eulerY, Float32 eulerZ);
        LF_INLINE ~Quaternion() {}

        LF_INLINE static scalar_type Dot(const quaternion_type& a, const quaternion_type& b);
        LF_INLINE static quaternion_type Cross(const quaternion_type& a, const quaternion_type& b);
        LF_INLINE static quaternion_type Lerp(const quaternion_type& a, const quaternion_type& b, const scalar_type t);
        LF_INLINE static quaternion_type Slerp(const quaternion_type& a, const quaternion_type& b, const scalar_type t);
        LF_INLINE static quaternion_type LookRotation(const vector_type& forward, const vector_type& up);
        LF_INLINE static quaternion_type LookRotation(const vector_type& forward);
        // LF_INLINE static quaternion_type RotateTowards(const quaternion_type& from, const quaternion_type& to, const scalar_type degrees);
        LF_INLINE static quaternion_type RotationBetween(const vector_type& a, const vector_type& b);
        LF_INLINE static quaternion_type Quaternion::AngleAxis(scalar_type angle, const vector_type& axis);

        LF_INLINE Quaternion Conjugate() const;
        LF_INLINE Quaternion Inverse() const;
        LF_INLINE Float32 Magnitude() const;
        LF_INLINE Float32 SqrMagnitude() const;
        LF_INLINE void Normalize();
        LF_INLINE Quaternion Normalized() const;
        LF_INLINE void Splat(const scalar_type scalar);

        LF_INLINE void SetEulerAngles(Float32 x, Float32 y, Float32 z);
        LF_INLINE Vector GetEulerAngles() const;
        LF_INLINE scalar_type Pitch() const;
        LF_INLINE scalar_type Yaw() const;
        LF_INLINE scalar_type Roll() const;

        LF_INLINE void SetAll(const Float32 x, const Float32 y, const Float32 z, const Float32 w);
        LF_INLINE void SetAll(const Float32 components[4]);
        LF_INLINE void GetAll(Float32 components[4]) const;

        LF_INLINE Quaternion& operator=(const Quaternion& other);
        LF_INLINE Quaternion operator+(const Quaternion& other) const;
        LF_INLINE Quaternion operator-(const Quaternion& other) const;
        LF_INLINE Quaternion operator*(Float32 scalar) const;
        LF_INLINE Quaternion operator/(Float32 scalar) const;
        LF_INLINE Quaternion operator*(const Quaternion& other) const;
        LF_INLINE vector_type operator*(const vector_type& vec) const;

        LF_INLINE bool operator==(const Quaternion& other) const;
        LF_INLINE bool operator!=(const Quaternion& other) const;
        LF_INLINE scalar_type operator[](const size_t i) const;

        LF_INLINE Quaternion operator-() const
        {
            return Quaternion(-mComponents[0], -mComponents[1], -mComponents[2], -mComponents[3]);
        }

        static const Quaternion Identity;

    private:
        Float32 mComponents[4];
    };
}

#include "QuaternionInline.h"
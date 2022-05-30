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
#include "Core/Common/API.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Math/SSECommon.h"
#include <utility>
#include <string.h>


namespace lf {
class LF_ALIGN(16) LF_CORE_API Vector
{
public:
    static const size_t MAX_COMPONENT = 4;
    typedef Vector vector_type;
    typedef Float32 scalar_type;

    //ctors
    LF_INLINE Vector();
    LF_INLINE Vector(const Vector& other);
    LF_INLINE Vector(Vector&& other);
    LF_INLINE Vector(LazyTag);
    LF_INLINE Vector(const scalar_type splat);
    LF_INLINE Vector(const scalar_type inX, const scalar_type inY, const scalar_type inZ);
    LF_INLINE Vector(const scalar_type inX, const scalar_type inY, const scalar_type inZ, const scalar_type inW);
    LF_INLINE Vector(const scalar_type components[4]);
    LF_INLINE ~Vector() {} 

    // static:
    LF_INLINE static scalar_type Angle(const vector_type& a, const vector_type& b);
    LF_INLINE static scalar_type Dot(const vector_type& a, const vector_type& b);
    LF_INLINE static vector_type Cross(const vector_type& a, const vector_type& b);
    LF_INLINE static vector_type Lerp(const vector_type& a, const vector_type& b, const scalar_type t);
    LF_INLINE static vector_type Slerp(const vector_type& a, const vector_type& b, const scalar_type t);
    LF_INLINE static vector_type Reflect(const vector_type& direction, const vector_type& normal);
    LF_INLINE static vector_type Refract(const vector_type& direction, const vector_type& normal, const scalar_type theta);
    LF_INLINE static vector_type FaceForward(const vector_type& direction, const vector_type& normal, const vector_type& normalRef);
    LF_INLINE static vector_type Project(const vector_type& a, const vector_type& b);
    LF_INLINE static scalar_type Distance(const vector_type& a, const vector_type& b);
    LF_INLINE static scalar_type SqrDistance(const vector_type& a, const vector_type& b);
    LF_INLINE static vector_type RotateX(const vector_type& v, const scalar_type angleDeg);
    LF_INLINE static vector_type RotateY(const vector_type& v, const scalar_type angleDeg);
    LF_INLINE static vector_type RotateZ(const vector_type& v, const scalar_type angleDeg);
    LF_INLINE static vector_type ClampMagnitude(const vector_type& v, const scalar_type length);

    // instance:
    LF_INLINE scalar_type Magnitude() const;
    LF_INLINE scalar_type SqrMagnitude() const;
    LF_INLINE void Normalize();
    LF_INLINE void SafeNormalize();
    LF_INLINE vector_type Normalized() const;
    LF_INLINE vector_type SafeNormalized() const;
    LF_INLINE void Splat(const scalar_type v);
    LF_INLINE scalar_type GetX() const;
    LF_INLINE scalar_type GetY() const;
    LF_INLINE scalar_type GetZ() const;
    LF_INLINE scalar_type GetW() const;
    LF_INLINE void SetX(const scalar_type v);
    LF_INLINE void SetY(const scalar_type v);
    LF_INLINE void SetZ(const scalar_type v);
    LF_INLINE void SetW(const scalar_type v);
    LF_INLINE void GetAll(scalar_type v[MAX_COMPONENT]) const;
    LF_INLINE void SetAll(const scalar_type v[MAX_COMPONENT]);
    LF_INLINE void SetAll(const scalar_type inX, const scalar_type inY, const scalar_type inZ, const scalar_type inW);

    // operators
    LF_INLINE vector_type& operator=(const vector_type& other);
    LF_INLINE vector_type& operator=(vector_type&& other);
    LF_INLINE vector_type& operator+=(const vector_type& other);
    LF_INLINE vector_type& operator-=(const vector_type& other);
    LF_INLINE vector_type& operator*=(const vector_type& other);
    LF_INLINE vector_type& operator*=(const scalar_type other);
    LF_INLINE vector_type& operator/=(const vector_type& other);
    LF_INLINE vector_type& operator/=(const scalar_type other);
         
    LF_INLINE vector_type operator+(const vector_type& other) const;
    LF_INLINE vector_type operator-(const vector_type& other) const;
    LF_INLINE vector_type operator*(const vector_type& other) const;
    LF_INLINE vector_type operator*(const scalar_type other) const;
    LF_INLINE vector_type operator/(const vector_type& other) const;
    LF_INLINE vector_type operator/(const scalar_type other) const;
         
    LF_INLINE bool operator==(const vector_type& other) const;
    LF_INLINE bool operator!=(const vector_type& other) const;
    LF_INLINE vector_type operator-() const;

    LF_INLINE scalar_type& operator[](const size_t index);
    LF_INLINE const scalar_type& operator[](const size_t index) const;


    // LF_FORCE_INLINE bool ApproxEquals(const Vector& other, Float32 precision = EPSILON) const
    // {
    //     internal_vector zeroVector = vector_zero;
    //     internal_vector r = vector_sub(vector_abs(vector_sub(mVector, other.mVector)), vector_set(precision, precision, precision, precision));
    //     return vector_ncmp(vector_less_equal(r, zeroVector), zeroVector);
    // }

    // LF_FORCE_INLINE bool AllLessEquals(const Vector& other) const
    // {
    //     return vector_ncmp(vector_less_equal(mVector, other.mVector), vector_zero);
    // }
    // 
    // LF_FORCE_INLINE bool AllGreaterEquals(const Vector& other) const
    // {
    //     return vector_ncmp(vector_greater_equal(mVector, other.mVector), vector_zero);
    // }

    // LF_FORCE_INLINE static Vector Abs(const Vector& v)
    // {
    //     return Vector(vector_abs(v.mVector));
    // }

    static const Vector Forward;
    static const Vector Up;
    static const Vector Right;
    static const Vector Zero;
    static const Vector One;

    
#if defined(LF_ENABLE_SSE_1)
        
    Vector(const internal_vector& vec) : mVector(vec)
    {

    }
    internal_vector mVector;
#else   
    Float32 x;
    Float32 y;
    Float32 z;
    Float32 w;
#endif
};

LF_INLINE Vector::vector_type operator*(const Vector::scalar_type f, const Vector::vector_type& v)
{
    return v * f;
}

LF_INLINE Vector::vector_type operator/(const Vector::scalar_type f, const Vector::vector_type& v)
{
    return v / f;
}
} // namespace lf
#include "VectorInline.h"
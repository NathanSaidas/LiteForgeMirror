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
#ifndef LF_CORE_VECTOR_3_H
#define LF_CORE_VECTOR_3_H

#include "Core/Common/Types.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Math/Vector.h"
#include <cmath> // sqrt
namespace lf {
class LF_CORE_API Vector3
{
public:
    static const size_t MAX_COMPONENT = 3;
    typedef Vector3 vector_type;
    typedef Float32 scalar_type;

         
    // ctors:
	LF_INLINE Vector3();
	LF_INLINE Vector3(const Vector3& other);
	LF_INLINE Vector3(Vector3&& other);
	LF_INLINE Vector3(LazyTag);
	LF_INLINE Vector3(const scalar_type splat);
	LF_INLINE Vector3(const scalar_type inX, const scalar_type inY, const scalar_type inZ);
	LF_INLINE explicit Vector3(const Vector& other);
	LF_INLINE ~Vector3() {}

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
    LF_INLINE void SetX(const scalar_type v);
    LF_INLINE void SetY(const scalar_type v);
    LF_INLINE void SetZ(const scalar_type v);
    LF_INLINE void GetAll(scalar_type v[MAX_COMPONENT]) const;
    LF_INLINE void SetAll(const scalar_type v[MAX_COMPONENT]);

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

        
    static const Vector3 One;
    static const Vector3 Zero;
    static const Vector3 Up;
    static const Vector3 Right;
    static const Vector3 Forward;

    Float32 x;
    Float32 y;
    Float32 z;
};

LF_INLINE Vector3::vector_type operator*(const Vector3::scalar_type f, const Vector3::vector_type& v)
{
    return v * f;
}

LF_INLINE Vector3::vector_type operator/(const Vector3::scalar_type f, const Vector3::vector_type& v)
{
    return v / f;
}
} // namespace lf

#include "Vector3Inline.h"

#endif // LF_CORE_VECTOR_3_H
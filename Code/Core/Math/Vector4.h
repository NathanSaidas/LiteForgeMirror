#ifndef LF_CORE_VECTOR_4_H
#define LF_CORE_VECTOR_4_H

#include "Core/Common/Types.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/MathFunctions.h"
#include <utility>

namespace lf {
class LF_CORE_API Vector4
{
public:
    static const size_t MAX_COMPONENT = 4;
    typedef Vector4 vector_type;
    typedef Float32 scalar_type;
         
        
    // ctors:
    LF_INLINE Vector4();
    LF_INLINE Vector4(const Vector4& other);
    LF_INLINE Vector4(Vector4&& other);
    LF_INLINE Vector4(LazyTag);
    LF_INLINE Vector4(const scalar_type splat);
    LF_INLINE Vector4(const scalar_type inX, const scalar_type inY, const scalar_type inZ);
    LF_INLINE Vector4(const scalar_type inX, const scalar_type inY, const scalar_type inZ, const scalar_type inW);
    LF_INLINE explicit Vector4(const Vector& other);
    LF_INLINE explicit Vector4(const Quaternion& other);
    LF_INLINE ~Vector4() {}

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

    Float32 x;
    Float32 y;
    Float32 z;
    Float32 w;
};

LF_INLINE Vector4::vector_type operator*(const Vector4::scalar_type f, const Vector4::vector_type& v)
{
    return v * f;
}

LF_INLINE Vector4::vector_type operator/(const Vector4::scalar_type f, const Vector4::vector_type& v)
{
    return v / f;
}
} // namespace lf

#include "Vector4Inline.h"

#endif // LF_CORE_VECTOR_4_H
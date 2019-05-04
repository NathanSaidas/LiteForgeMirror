#ifndef LF_CORE_VECTOR_2_H
#define LF_CORE_VECTOR_2_H

#include "Core/Common/Types.h"
#include "Core/Math/Vector.h"

namespace lf {
class LF_CORE_API Vector2
{
public:
    static const size_t MAX_COMPONENT = 2;
    typedef Vector2 vector_type;
    typedef Float32 scalar_type;

	LF_INLINE Vector2();
	LF_INLINE Vector2(const Vector2& other);
	LF_INLINE Vector2(Vector2&& other);
	LF_INLINE Vector2(LazyTag);
	LF_INLINE Vector2(const scalar_type splat);
	LF_INLINE Vector2(const scalar_type inX, const scalar_type inY);
	LF_INLINE explicit Vector2(const Vector& other);
	LF_INLINE ~Vector2() {}

    // static:
	LF_INLINE static scalar_type Angle(const vector_type& a, const vector_type& b);
	LF_INLINE static scalar_type Dot(const vector_type& a, const vector_type& b);
    LF_INLINE static vector_type Lerp(const vector_type& a, const vector_type& b, const scalar_type t);
    LF_INLINE static vector_type Project(const vector_type& a, const vector_type& b);
	LF_INLINE static scalar_type Distance(const vector_type& a, const vector_type& b);
	LF_INLINE static scalar_type SqrDistance(const vector_type& a, const vector_type& b);
    LF_INLINE static vector_type Rotate(const vector_type& v, const scalar_type angleDeg);
    LF_INLINE static vector_type ClampMagnitude(const vector_type& v, const scalar_type length);
    LF_INLINE static vector_type Clamp(const vector_type& v, const vector_type& min, const vector_type& max);

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
	LF_INLINE void SetX(const scalar_type v);
	LF_INLINE void SetY(const scalar_type v);
	LF_INLINE void GetAll(scalar_type v[MAX_COMPONENT]) const;
	LF_INLINE void SetAll(const scalar_type v[MAX_COMPONENT]);

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
		
    static const Vector2 One;
    static const Vector2 Zero;

    Float32 x;
    Float32 y;
};

LF_INLINE Vector2::vector_type operator*(const Vector2::scalar_type f, const Vector2::vector_type& v)
{
    return v * f;
}

LF_INLINE Vector2::vector_type operator/(const Vector2::scalar_type f, const Vector2::vector_type& v)
{
    return v / f;
}
} // namespace lf

#include "Vector2Inline.h"

#endif // LF_CORE_VECTOR_2_H
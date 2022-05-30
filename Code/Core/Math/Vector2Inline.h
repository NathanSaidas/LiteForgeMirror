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

namespace lf {
Vector2::Vector2() : x(0.0f), y(0.0f)
{}
Vector2::Vector2(const Vector2& other) : x(other.x), y(other.y)
{}
Vector2::Vector2(Vector2&& other) : x(other.x), y(other.y)
{
	other.x = 0.0f;
	other.y = 0.0f;
}
Vector2::Vector2(LazyTag)
{}
Vector2::Vector2(const scalar_type splat) : x(splat), y(splat)
{}
Vector2::Vector2(const scalar_type inX, const scalar_type inY) : x(inX), y(inY)
{}
Vector2::Vector2(const Vector& other) : x(other[0]), y(other[1])
{

}

Vector2::scalar_type Vector2::Angle(const vector_type& a, const vector_type& b)
{
	// dot = Dot(a,b)
	// clamped = Clamp(dot,-1,1)
	// angleRadians = acos(clamped)
	// angle = Rad2Deg(angleRadians)

	return Rad2Deg(std::acosf(::lf::Clamp(Dot(a, b), scalar_type(-1.0f), (1.0f))));
}
Vector2::scalar_type Vector2::Dot(const vector_type& a, const vector_type& b)
{
	vector_type temp = a * b;
	return temp.x + temp.y;
}
Vector2::vector_type Vector2::Lerp(const vector_type& a, const vector_type& b, const scalar_type t)
{
	return a + (b - a) * t;
}
Vector2::vector_type Vector2::Project(const vector_type& a, const vector_type& b)
{
	scalar_type dot = Dot(a, b);
	scalar_type mag = b.Magnitude();
	return (dot / mag) * b;
}
Vector2::scalar_type Vector2::Distance(const vector_type& a, const vector_type& b)
{
	return (a - b).Magnitude();
}
Vector2::scalar_type Vector2::SqrDistance(const vector_type& a, const vector_type& b)
{
	return (a - b).SqrMagnitude();
}
Vector2::vector_type Vector2::Rotate(const vector_type& v, const scalar_type angleDeg)
{
	scalar_type c = std::cosf(Deg2Rad(angleDeg));
	scalar_type s = std::sinf(Deg2Rad(angleDeg));

	vector_type r(
		v.x * c + v.y * s,
		v.x * s + v.y * c
	);
	return r;
}
Vector2::vector_type Vector2::ClampMagnitude(const vector_type& v, const scalar_type length)
{
	return v.Normalized() * length;
}

Vector2::vector_type Vector2::Clamp(const vector_type& v, const vector_type& min, const vector_type& max)
{
    return(Vector2(::lf::Clamp(v.x, min.x, max.x), ::lf::Clamp(v.y, min.y, max.y)));
}

Vector2::scalar_type Vector2::Magnitude() const
{
	return std::sqrtf(x * x + y * y);
}
Vector2::scalar_type Vector2::SqrMagnitude() const
{
	return x * x + y * y;
}
void Vector2::Normalize()
{
	scalar_type mag = InverseSqrt(SqrMagnitude());
	x /= mag;
	y /= mag;
}
void Vector2::SafeNormalize()
{
	scalar_type mag = Magnitude();
	x /= mag;
	y /= mag;
}
Vector2::vector_type Vector2::Normalized() const
{
	vector_type r = *this;
	r.Normalize();
	return r;
}
Vector2::vector_type Vector2::SafeNormalized() const
{
	vector_type r = *this;
	r.SafeNormalize();
	return r;
}
void Vector2::Splat(const scalar_type v)
{
	x = y = v;
}
Vector2::scalar_type Vector2::GetX() const
{
	return x;
}
Vector2::scalar_type Vector2::GetY() const
{
	return y;
}
void Vector2::SetX(const scalar_type v)
{
	x = v;
}
void Vector2::SetY(const scalar_type v)
{
	y = v;
}
void Vector2::GetAll(scalar_type v[MAX_COMPONENT]) const
{
	v[0] = x;
	v[1] = y;
}
void Vector2::SetAll(const scalar_type v[MAX_COMPONENT])
{
	x = v[0];
	y = v[1];
}

Vector2::vector_type& Vector2::operator=(const vector_type& other)
{
	x = other.x;
	y = other.y;
	return *this;
}
Vector2::vector_type& Vector2::operator=(vector_type&& other)
{
	x = other.x; other.x = 0.0f;
	y = other.y; other.y = 0.0f;
	return *this;
}
Vector2::vector_type& Vector2::operator+=(const vector_type& other)
{
	x += other.x;
	y += other.y;
	return *this;
}
Vector2::vector_type& Vector2::operator-=(const vector_type& other)
{
	x -= other.x;
	y -= other.y;
	return *this;
}
Vector2::vector_type& Vector2::operator*=(const vector_type& other)
{
	x *= other.x;
	y *= other.y;
	return *this;
}
Vector2::vector_type& Vector2::operator*=(const scalar_type other)
{
	x *= other;
	y *= other;
	return *this;
}
Vector2::vector_type& Vector2::operator/=(const vector_type& other)
{
	x /= other.x;
	y /= other.y;
	return *this;
}
Vector2::vector_type& Vector2::operator/=(const scalar_type other)
{
	x /= other;
	y /= other;
	return *this;
}

Vector2::vector_type Vector2::operator+(const vector_type& other) const
{
	vector_type r = *this;
	r += other;
	return r;
}
Vector2::vector_type Vector2::operator-(const vector_type& other) const
{
	vector_type r = *this;
	r -= other;
	return r;
}
Vector2::vector_type Vector2::operator*(const vector_type& other) const
{
	vector_type r = *this;
	r -= other;
	return r;
}
Vector2::vector_type Vector2::operator*(const scalar_type other) const
{
	vector_type r = *this;
	r *= other;
	return r;
}
Vector2::vector_type Vector2::operator/(const vector_type& other) const
{
	vector_type r = *this;
	r /= other;
	return r;
}
Vector2::vector_type Vector2::operator/(const scalar_type other) const
{
	vector_type r = *this;
	r /= other;
	return r;
}

bool Vector2::operator==(const vector_type& other) const
{
	return x == other.x && y == other.y;
}
bool Vector2::operator!=(const vector_type& other) const
{
	return x != other.x || y != other.y;
}
Vector2::vector_type Vector2::operator-() const
{
	return vector_type(-x, -y);
}

Vector2::scalar_type& Vector2::operator[](const size_t index)
{
	switch (index)
	{
	case 0: return x;
	case 1: return y;
	default:
		break;
	}

	CriticalAssertMsgEx("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
	return x;
}
const Vector2::scalar_type& Vector2::operator[](const size_t index) const
{
	switch (index)
	{
	case 0: return x;
	case 1: return y;
	default:
		break;
	}

	CriticalAssertMsgEx("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
	return x;
}
}


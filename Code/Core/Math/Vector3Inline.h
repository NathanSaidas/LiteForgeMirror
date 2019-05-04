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
namespace lf {
Vector3::Vector3() : x(0.0f), y(0.0f), z(0.0f)
{}
Vector3::Vector3(const Vector3& other) : x(other.x), y(other.y), z(other.z)
{}
Vector3::Vector3(Vector3&& other) : x(other.x), y(other.y), z(other.z)
{
    other.x = other.y = other.z = 0.0f;
}
Vector3::Vector3(LazyTag) 
{}
Vector3::Vector3(const scalar_type splat) : x(splat), y(splat), z(splat)
{}
Vector3::Vector3(const scalar_type inX, const scalar_type inY, const scalar_type inZ) : x(inX), y(inY), z(inZ)
{}
Vector3::Vector3(const Vector& other) : x(other[0]), y(other[1]), z(other[2])
{}
     
// static:
Vector3::scalar_type Vector3::Angle(const vector_type& a, const vector_type& b)
{
    // dot = Dot(a,b)
    // clamped = Clamp(dot,-1,1)
    // angleRadians = acos(clamped)
    // angle = Rad2Deg(angleRadians)

    return Rad2Deg(std::acosf(Clamp(Dot(a, b), scalar_type(-1.0f), (1.0f))));
}
Vector3::scalar_type Vector3::Dot(const vector_type& a, const vector_type& b)
{
    vector_type temp = a * b;
    return temp.x + temp.y + temp.z;
}
Vector3::vector_type Vector3::Cross(const vector_type& a, const vector_type& b)
{
    return vector_type(
        a.y * b.z - b.y * a.z,
        a.z * b.x - b.z * a.x,
        a.x * b.y - b.x * a.y
    );
}
Vector3::vector_type Vector3::Lerp(const vector_type& a, const vector_type& b, const scalar_type t)
{
    return a + (b - a) * t;
}
Vector3::vector_type Vector3::Slerp(const vector_type& a, const vector_type& b, const scalar_type t)
{
    scalar_type dot = Clamp(Dot(a, b), scalar_type(-1.0f), scalar_type(1.0f));
    scalar_type theta = std::acosf(dot) * t;
    vector_type relativeVec = b - a * t;
    relativeVec.Normalize();
    return ((a * std::cosf(theta)) + relativeVec * std::sinf(theta));
}
Vector3::vector_type Vector3::Reflect(const vector_type& direction, const vector_type& normal)
{
    return direction - 2 * Dot(direction, normal) * normal;
}
Vector3::vector_type Vector3::Refract(const vector_type& direction, const vector_type& normal, const scalar_type theta)
{
    scalar_type dot = Dot(direction, normal);
    scalar_type k = scalar_type(1) - theta * theta * (scalar_type(1) - dot * dot);
    if (k < scalar_type(0))
    {
        return vector_type(0.0f);
    }
    else
    {
        return theta * direction - (theta * dot + std::sqrt(k)) * normal;
    }
}
Vector3::vector_type Vector3::FaceForward(const vector_type& direction, const vector_type& normal, const vector_type& normalRef)
{
    return Dot(normalRef, direction) < 0 ? normal : -normal;
}
Vector3::vector_type Vector3::Project(const vector_type& a, const vector_type& b)
{
    scalar_type dot = Dot(a, b);
    scalar_type mag = b.Magnitude();
    return (dot / mag) * b;
}
Vector3::scalar_type Vector3::Distance(const vector_type& a, const vector_type& b)
{
    return (a - b).Magnitude();
}
Vector3::scalar_type Vector3::SqrDistance(const vector_type& a, const vector_type& b)
{
    return (a - b).SqrMagnitude();
}
Vector3::vector_type Vector3::RotateX(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    vector_type r(
        v.x,
        v.y * c - v.z * s,
        v.y * s + v.z * c
    );
    return r;
}
Vector3::vector_type Vector3::RotateY(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    vector_type r(
        v.x * c + v.z * s,
        v.y,
        v.x * s + v.z * c
    );
    return r;
}
Vector3::vector_type Vector3::RotateZ(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    vector_type r(
        v.x * c + v.y * s,
        v.x * s + v.y * c,
        v.z
    );
    return r;
}
Vector3::vector_type Vector3::ClampMagnitude(const vector_type& v, const scalar_type length)
{
    return v.Normalized() * length;
}

// instance:
Vector3::scalar_type Vector3::Magnitude() const
{
    return std::sqrtf(x * x + y * y + z * z);
}
Vector3::scalar_type Vector3::SqrMagnitude() const
{
    return x * x + y * y + z * z;
}
void Vector3::Normalize()
{
    scalar_type mag = InverseSqrt(SqrMagnitude());
    x /= mag;
    y /= mag;
    z /= mag;
}
void Vector3::SafeNormalize()
{
    scalar_type mag = Magnitude();
    x /= mag;
    y /= mag;
    z /= mag;
}
Vector3::vector_type Vector3::Normalized() const
{
    vector_type r = *this;
    r.Normalize();
    return r;
}
Vector3::vector_type Vector3::SafeNormalized() const
{
    vector_type r = *this;
    r.SafeNormalize();
    return r;
}
void Vector3::Splat(const scalar_type v)
{
    x = y = z = v;
}
Vector3::scalar_type Vector3::GetX() const
{
    return x;
}
Vector3::scalar_type Vector3::GetY() const
{
    return y;
}
Vector3::scalar_type Vector3::GetZ() const
{
    return z;
}
void Vector3::SetX(const scalar_type v)
{
    x = v;
}
void Vector3::SetY(const scalar_type v)
{
    y = v;
}
void Vector3::SetZ(const scalar_type v)
{
    z = v;
}
void Vector3::GetAll(scalar_type v[MAX_COMPONENT]) const
{
    v[0] = x;
    v[1] = y;
    v[2] = y;
}
void Vector3::SetAll(const scalar_type v[MAX_COMPONENT])
{
    x = v[0];
    y = v[1];
    z = v[2];
}

// operators
Vector3::vector_type& Vector3::operator=(const vector_type& other)
{
    x = other.x;
    y = other.y;
    z = other.z;
    return *this;
}
Vector3::vector_type& Vector3::operator=(vector_type&& other)
{
    x = other.x; other.x = 0.0f;
    y = other.y; other.y = 0.0f;
    z = other.z; other.z = 0.0f;
    return *this;
}
Vector3::vector_type& Vector3::operator+=(const vector_type& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    return *this;
}
Vector3::vector_type& Vector3::operator-=(const vector_type& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    return *this;
}
Vector3::vector_type& Vector3::operator*=(const vector_type& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    return *this;
}
Vector3::vector_type& Vector3::operator*=(const scalar_type other)
{
    x *= other;
    y *= other;
    z *= other;
    return *this;
}
Vector3::vector_type& Vector3::operator/=(const vector_type& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    return *this;
}
Vector3::vector_type& Vector3::operator/=(const scalar_type other)
{
    x /= other;
    y /= other;
    z /= other;
    return *this;
}

Vector3::vector_type Vector3::operator+(const vector_type& other) const
{
    vector_type r = *this;
    r += other;
    return r;
}
Vector3::vector_type Vector3::operator-(const vector_type& other) const
{
    vector_type r = *this;
    r -= other;
    return r;
}
Vector3::vector_type Vector3::operator*(const vector_type& other) const
{
    vector_type r = *this;
    r *= other;
    return r;
}
Vector3::vector_type Vector3::operator*(const scalar_type other) const
{
    vector_type r = *this;
    r *= other;
    return r;
}
Vector3::vector_type Vector3::operator/(const vector_type& other) const
{
    vector_type r = *this;
    r /= other;
    return r;
}
Vector3::vector_type Vector3::operator/(const scalar_type other) const
{
    vector_type r = *this;
    r /= other;
    return r;
}

bool Vector3::operator==(const vector_type& other) const
{
    return x == other.x && y == other.y && z == other.z;
}
bool Vector3::operator!=(const vector_type& other) const
{
    return x != other.x || y != other.y || z != other.z;
}
Vector3::vector_type Vector3::operator-() const
{
    return vector_type(-x, -y, -z);
}

Vector3::scalar_type& Vector3::operator[](const size_t index)
{
    switch (index)
    {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    default:
        break;
    }

    Crash("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
    return x;
}
const Vector3::scalar_type& Vector3::operator[](const size_t index) const
{
    switch (index)
    {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    default:
        break;
    }

    Crash("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
    return x;
}
} // namespace lf

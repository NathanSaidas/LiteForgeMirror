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
Vector4::Vector4() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
Vector4::Vector4(const Vector4& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
Vector4::Vector4(Vector4&& other) : x(std::move(other.x)), y(std::move(other.y)), z(std::move(other.z)), w(std::move(other.w)) {}
Vector4::Vector4(LazyTag) {}
Vector4::Vector4(const scalar_type splat) : x(splat), y(splat), z(splat), w(splat) {}
Vector4::Vector4(const scalar_type inX, const scalar_type inY, const scalar_type inZ) : x(inX), y(inY), z(inZ), w(1.0f) {}
Vector4::Vector4(const scalar_type inX, const scalar_type inY, const scalar_type inZ, const scalar_type inW) : x(inX), y(inY), z(inZ), w(inW) {}
Vector4::Vector4(const Vector& other) : x(other[0]), y(other[1]), z(other[2]), w(other[3]) {}
Vector4::Vector4(const Quaternion& other) : x(other[0]), y(other[1]), z(other[2]), w(other[3]) {}
Vector4::scalar_type Vector4::Angle(const vector_type& a, const vector_type& b)
{
    // dot = Dot(a,b)
    // clamped = Clamp(dot,-1,1)
    // angleRadians = acos(clamped)
    // angle = Rad2Deg(angleRadians)

    return Rad2Deg(std::acosf(Clamp(Dot(a, b), scalar_type(-1.0f), (1.0f))));
}
     
Vector4::scalar_type Vector4::Dot(const vector_type& a, const vector_type& b)
{
    // component multiply 
    // sum components
    vector_type temp = a * b;
    return temp.x + temp.y + temp.z + temp.w;
}

Vector4::vector_type Vector4::Cross(const vector_type& a, const vector_type& b)
{
    return vector_type(
        a.y * b.z - b.y * a.z,
        a.z * b.x - b.z * a.x,
        a.x * b.y - b.x * a.y,
        1.0f
    );
}

Vector4::vector_type Vector4::Lerp(const vector_type& a, const vector_type& b, const scalar_type t)
{
    return a + (b - a) * t;
}

Vector4::vector_type Vector4::Slerp(const vector_type& a, const vector_type& b, const scalar_type t)
{
    scalar_type dot = Clamp(Dot(a, b), scalar_type(-1.0f),scalar_type(1.0f));
    scalar_type theta = std::acosf(dot) * t;
    vector_type relativeVec = b - a * t;
    relativeVec.Normalize();
    return ((a * std::cosf(theta)) + relativeVec * std::sinf(theta));
}

Vector4::vector_type Vector4::Reflect(const vector_type& direction, const vector_type& normal)
{
    return direction - 2 * Dot(direction, normal) * normal;
}

Vector4::vector_type Vector4::Refract(const vector_type& direction, const vector_type& normal, const scalar_type theta)
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

Vector4::vector_type Vector4::FaceForward(const vector_type& direction, const vector_type& normal, const vector_type& normalRef)
{
    return Dot(normalRef, direction) < 0 ? normal : -normal;
}

Vector4::vector_type Vector4::Project(const vector_type& a, const vector_type& b)
{
    scalar_type dot = Dot(a, b);
    scalar_type mag = b.Magnitude();
    return (dot / mag) * b;
}

Vector4::scalar_type Vector4::Distance(const vector_type& a, const vector_type& b)
{
    return (a-b).Magnitude();
}

Vector4::scalar_type Vector4::SqrDistance(const vector_type& a, const vector_type& b)
{
    return (a - b).SqrMagnitude();
}

Vector4::vector_type Vector4::RotateX(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    vector_type r(
        v.x,
        v.y * c - v.z * s,
        v.y * s + v.z * c,
        v.w
    );
    return r;
}

Vector4::vector_type Vector4::RotateY(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    vector_type r(
        v.x * c + v.z * s,
        v.y,
        v.x * s + v.z * c,
        v.w
    );
    return r;
}

Vector4::vector_type Vector4::RotateZ(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    vector_type r(
        v.x * c + v.y * s,
        v.x * s + v.y * c,
        v.z,
        v.w
    );
    return r;
}

Vector4::vector_type Vector4::ClampMagnitude(const vector_type& v, const scalar_type length)
{
    return v.Normalized() * length;
}

Vector4::scalar_type Vector4::Magnitude() const
{
    return std::sqrtf(x * x + y * y + z * z + w * w);
}

Vector4::scalar_type Vector4::SqrMagnitude() const
{
    return x * x + y * y + z * z + w * w;
}

void Vector4::Normalize()
{
    scalar_type mag = InverseSqrt(SqrMagnitude());
    x /= mag;
    y /= mag;
    z /= mag;
    w /= mag;
}

void Vector4::SafeNormalize()
{
    scalar_type mag = Magnitude();
    x /= mag;
    y /= mag;
    z /= mag;
    w /= mag;
}

Vector4::vector_type Vector4::Normalized() const
{
    vector_type r = *this;
    r.Normalize();
    return r;
}

Vector4::vector_type Vector4::SafeNormalized() const
{
    vector_type r = *this;
    r.SafeNormalize();
    return r;
}

void Vector4::Splat(const scalar_type v)
{
    x = y = z = w = v;
}

Vector4::scalar_type Vector4::GetX() const
{
    return x;
}

Vector4::scalar_type Vector4::GetY() const
{
    return y;
}

Vector4::scalar_type Vector4::GetZ() const
{
    return z;
}

Vector4::scalar_type Vector4::GetW() const
{
    return w;
}

void Vector4::SetX(const scalar_type v)
{
    x = v;
}

void Vector4::SetY(const scalar_type v)
{
    y = v;
}

void Vector4::SetZ(const scalar_type v)
{
    z = v;
}

void Vector4::SetW(const scalar_type v)
{
    w = v;
}

void Vector4::GetAll(scalar_type v[MAX_COMPONENT]) const
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}
void Vector4::SetAll(const scalar_type v[MAX_COMPONENT])
{
    x = v[0];
    y = v[1];
    z = v[2];
    w = v[3];
}

Vector4::vector_type& Vector4::operator=(const vector_type& other)
{
    x = other.x;
    y = other.y;
    z = other.z;
    w = other.w;
    return *this;
}

Vector4::vector_type& Vector4::operator=(vector_type&& other)
{
    x = std::move(other.x);
    y = std::move(other.y);
    z = std::move(other.z);
    w = std::move(other.w);
    return *this;
}

Vector4::vector_type& Vector4::operator+=(const vector_type& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

Vector4::vector_type& Vector4::operator-=(const vector_type& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

Vector4::vector_type& Vector4::operator*=(const vector_type& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return *this;
}

Vector4::vector_type& Vector4::operator*=(const scalar_type other)
{
    x *= other;
    y *= other;
    z *= other;
    w *= other;
    return *this;
}

Vector4::vector_type& Vector4::operator/=(const vector_type& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return *this;
}

Vector4::vector_type& Vector4::operator/=(const scalar_type other)
{
    x /= other;
    y /= other;
    z /= other;
    w /= other;
    return *this;
}

Vector4::vector_type Vector4::operator+(const vector_type& other) const
{
    return vector_type(
        x + other.x,
        y + other.y,
        z + other.z,
        w + other.w
    );
}

Vector4::vector_type Vector4::operator-(const vector_type& other) const
{
    return vector_type(
        x - other.x,
        y - other.y,
        z - other.z,
        w - other.w
    );
}

Vector4::vector_type Vector4::operator*(const vector_type& other) const
{
    return vector_type(
        x * other.x,
        y * other.y,
        z * other.z,
        w * other.w
    );
}

Vector4::vector_type Vector4::operator*(const scalar_type other) const
{
    return vector_type(
        x * other,
        y * other,
        z * other,
        w * other
    );
}

Vector4::vector_type Vector4::operator/(const vector_type& other) const
{
    return vector_type(
        x / other.x,
        y / other.y,
        z / other.z,
        w / other.w
    );
}

Vector4::vector_type Vector4::operator/(const scalar_type other) const
{
    return vector_type(
        x / other,
        y / other,
        z / other,
        w / other
    );
}

bool Vector4::operator==(const vector_type& other) const
{
    return x == other.x && y == other.y && z == other.z && w == other.z;
}

bool Vector4::operator!=(const vector_type& other) const
{
    return x != other.x && y != other.y && z != other.z && w != other.z;
}

Vector4::vector_type Vector4::operator-() const
{
    return vector_type(-x, -y, -z, -w);
}

Vector4::scalar_type& Vector4::operator[](const size_t index)
{
    switch (index)
    {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    case 3: return w;
    default:
        break;
    }

    CriticalAssertMsgEx("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
    return x;
}

const Vector4::scalar_type& Vector4::operator[](const size_t index) const
{
    switch (index)
    {
    case 0: return x;
    case 1: return y;
    case 2: return z;
    case 3: return w;
    default:
        break;
    }

    CriticalAssertMsgEx("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
    return x;
}
} // namespace lf
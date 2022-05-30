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
#if defined(LF_ENABLE_SSE_1)
Vector::Vector() : mVector(vector_set(0.0f, 0.0f, 0.0f, 1.0f)) {}
Vector::Vector(const Vector& other) : mVector(other.mVector) {}
Vector::Vector(Vector&& other) : mVector(std::move(other.mVector)) {}
Vector::Vector(LazyTag) {} 
Vector::Vector(const scalar_type splat) : mVector(vector_set(splat, splat, splat, 1.0f)) {} 
Vector::Vector(const scalar_type x, const scalar_type y, const scalar_type z) : mVector(vector_set(x, y, z, 1.0f)) {}
Vector::Vector(const scalar_type x, const scalar_type y, const scalar_type z, const scalar_type w) : mVector(vector_set(x, y, z, w)) {}
Vector::Vector(const scalar_type components[4]) { SetAll(components); }
     
Vector::scalar_type Vector::Angle(const vector_type& a, const vector_type& b)
{
    return Rad2Deg(std::acosf(Clamp(Dot(a, b), scalar_type(-1.0f), (1.0f))));
}

    Vector::scalar_type Vector::Dot(const vector_type& a, const vector_type& b)
{
    internal_vector r = vector_dot(a.mVector, b.mVector, 0x7F);
    return vector_to_float(r);
}

Vector::vector_type Vector::Cross(const vector_type& a, const vector_type& b)
{
    return vector_type(vector_add(vector_sub(
        vector_mul(vector_cross_shuffle(a.mVector, 3, 0, 2, 1), vector_cross_shuffle(b.mVector, 3, 1, 0, 2)),
        vector_mul(vector_cross_shuffle(a.mVector, 3, 1, 0, 2), vector_cross_shuffle(b.mVector, 3, 0, 2, 1))
    ), vector_set(0.0f, 0.0f, 0.0f, 1.0f)));
}

Vector::vector_type Vector::RotateX(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    internal_aligned_buffer vec[4];
    vector_get(vec, v.mVector);
    vector_type r(
        vec[0],
        vec[1] * c - vec[2] * s,
        vec[1] * s + vec[2] * c,
        vec[3]
    );
    return r;
}

Vector::vector_type Vector::RotateY(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    internal_aligned_buffer vec[4];
    vector_get(vec, v.mVector);
    vector_type r(
        vec[0] * c + vec[2] * s,
        vec[1],
        vec[0] * s + vec[2] * c,
        vec[3]
    );
    return r;
}

Vector::vector_type Vector::RotateZ(const vector_type& v, const scalar_type angleDeg)
{
    scalar_type c = std::cosf(Deg2Rad(angleDeg));
    scalar_type s = std::sinf(Deg2Rad(angleDeg));

    internal_aligned_buffer vec[4];
    vector_get(vec, v.mVector);
    vector_type r(
        vec[0] * c + vec[1] * s,
        vec[0] * s + vec[1] * c,
        vec[2],
        vec[3]
    );
    return r;
}

Vector::scalar_type Vector::Magnitude() const
{
    return vector_to_float(vector_sqrt(vector_dot(mVector, mVector, 0x71)));
}

Vector::scalar_type Vector::SqrMagnitude() const
{
    return vector_to_float(vector_dot(mVector, mVector, 0x71));
}

void Vector::Normalize()
{
    scalar_type mag = InverseSqrt(SqrMagnitude());
    mVector = vector_mul(mVector, vector_set(mag, mag, mag, mag));
}

void Vector::SafeNormalize()
{
    scalar_type mag = Magnitude();
    mVector = vector_div(mVector, vector_set(mag, mag, mag, mag));
}

void Vector::Splat(const scalar_type v)
{
    mVector = vector_set(v, v, v, 1.0f);
}

Vector::scalar_type Vector::GetX() const
{
    return mVector.m128_f32[0];
}

Vector::scalar_type Vector::GetY() const
{
    return mVector.m128_f32[1];
}

Vector::scalar_type Vector::GetZ() const
{
    return mVector.m128_f32[2];
}

Vector::scalar_type Vector::GetW() const
{
    return mVector.m128_f32[3];
}

void Vector::SetX(const scalar_type v)
{
    mVector.m128_f32[0] = v;
}

void Vector::SetY(const scalar_type v)
{
    mVector.m128_f32[1] = v;
}

void Vector::SetZ(const scalar_type v)
{
    mVector.m128_f32[2] = v;
}

void Vector::SetW(const scalar_type v)
{
    mVector.m128_f32[3] = v;
}

void Vector::GetAll(scalar_type v[MAX_COMPONENT]) const
{
    memcpy(v, &mVector, sizeof(scalar_type) * MAX_COMPONENT);
}

void Vector::SetAll(const scalar_type inX, const scalar_type inY, const scalar_type inZ, const scalar_type inW)
{
    mVector = vector_set(inX, inY, inZ, inW);
}

void Vector::SetAll(const scalar_type v[MAX_COMPONENT])
{
    memcpy(&mVector, v, sizeof(scalar_type) * MAX_COMPONENT);
}

Vector::vector_type& Vector::operator=(const vector_type& other)
{
    mVector = other.mVector;
    return *this;
}

Vector::vector_type& Vector::operator=(vector_type&& other)
{
    mVector = std::move(other.mVector);
    return *this;
}

Vector::vector_type& Vector::operator+=(const vector_type& other)
{
    mVector = vector_add(mVector, other.mVector);
    return *this;
}

Vector::vector_type& Vector::operator-=(const vector_type& other)
{
    mVector = vector_sub(mVector, other.mVector);
    return *this;
}

Vector::vector_type& Vector::operator*=(const vector_type& other)
{
    mVector = vector_mul(mVector, other.mVector);
    return *this;
}

Vector::vector_type& Vector::operator*=(const scalar_type other)
{
    mVector = vector_mul(mVector, vector_set(other,other,other,other));
    return *this;
}

Vector::vector_type& Vector::operator/=(const vector_type& other)
{
    mVector = vector_div(mVector,  other.mVector);
    return *this;
}

Vector::vector_type& Vector::operator/=(const scalar_type other)
{
    mVector = vector_mul(mVector, vector_set(other, other, other, other));
    return *this;
}

Vector::vector_type Vector::operator+(const vector_type& other) const
{
    return vector_type(vector_add(mVector, other.mVector));
}

Vector::vector_type Vector::operator-(const vector_type& other) const
{
    return vector_type(vector_sub(mVector, other.mVector));
}

Vector::vector_type Vector::operator*(const vector_type& other) const
{
    return vector_type(vector_mul(mVector, other.mVector));
}

Vector::vector_type Vector::operator*(const scalar_type other) const
{
    return vector_type(vector_mul(mVector, vector_set(other,other,other,other)));
}

Vector::vector_type Vector::operator/(const vector_type& other) const
{
    return vector_type(vector_div(mVector, other.mVector));
}

Vector::vector_type Vector::operator/(const scalar_type other) const
{
    return vector_type(vector_div(mVector, vector_set(other, other, other, other)));
}

bool Vector::operator==(const vector_type& other) const
{
    return vector_cmp(mVector, other.mVector);
}

bool Vector::operator!=(const vector_type& other) const
{
    bool r = vector_cmp(mVector, other.mVector);
    return !r;
}

Vector::vector_type Vector::operator-() const
{
    return vector_type(vector_mul(mVector, vector_set(-1.0f, -1.0f, -1.0f, -1.0f)));
}

Vector::scalar_type& Vector::operator[](const size_t index)
{
    switch (index)
    {
    case 0: return mVector.m128_f32[0];
    case 1: return mVector.m128_f32[1];
    case 2: return mVector.m128_f32[2];
    case 3: return mVector.m128_f32[3];
    default:
        break;
    }

    CriticalAssertMsgEx("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
    return mVector.m128_f32[0];
}

const Vector::scalar_type& Vector::operator[](const size_t index) const
{
    switch (index)
    {
    case 0: return mVector.m128_f32[0];
    case 1: return mVector.m128_f32[1];
    case 2: return mVector.m128_f32[2];
    case 3: return mVector.m128_f32[3];
    default:
        break;
    }

    CriticalAssertMsgEx("Operator [] index out of bounds.", LF_ERROR_BAD_STATE, ERROR_API_CORE);
    return mVector.m128_f32[0];
}
#else 
Vector::Vector() : x(0.0f), y(0.0f), z(0.0f), w(1.0f) {}
Vector::Vector(const Vector& other) : x(other.x), y(other.y), z(other.z), w(other.w) {}
Vector::Vector(Vector&& other) : x(std::move(other.x)), y(std::move(other.y)), z(std::move(other.z)), w(std::move(other.w)) {}
Vector::Vector(LazyTag) {}
Vector::Vector(const scalar_type splat) : x(splat), y(splat), z(splat), w(splat) {}
Vector::Vector(const scalar_type inX, const scalar_type inY, const scalar_type inZ) : x(inX), y(inY), z(inZ), w(1.0f) {}
Vector::Vector(const scalar_type inX, const scalar_type inY, const scalar_type inZ, const scalar_type inW) : x(inX), y(inY), z(inZ), w(inW) {}

Vector::scalar_type Vector::Angle(const vector_type& a, const vector_type& b)
{
    // dot = Dot(a,b)
    // clamped = Clamp(dot,-1,1)
    // angleRadians = acos(clamped)
    // angle = Rad2Deg(angleRadians)

    return Rad2Deg(std::acosf(Clamp(Dot(a, b), scalar_type(-1.0f), (1.0f))));
}

Vector::scalar_type Vector::Dot(const vector_type& a, const vector_type& b)
{
    // component multiply 
    // sum components
    vector_type temp = a * b;
    return temp.x + temp.y + temp.z + temp.w;
}

Vector::vector_type Vector::Cross(const vector_type& a, const vector_type& b)
{
    return vector_type(
        a.y * b.z - b.y * a.z,
        a.z * b.x - b.z * a.x,
        a.x * b.y - b.x * a.y,
        1.0f
    );
}

Vector::vector_type Vector::RotateX(const vector_type& v, const scalar_type angleDeg)
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

Vector::vector_type Vector::RotateY(const vector_type& v, const scalar_type angleDeg)
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

Vector::vector_type Vector::RotateZ(const vector_type& v, const scalar_type angleDeg)
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

    

Vector::scalar_type Vector::Magnitude() const
{
    return std::sqrtf(x * x + y * y + z * z + w * w);
}

Vector::scalar_type Vector::SqrMagnitude() const
{
    return x * x + y * y + z * z + w * w;
}

void Vector::Normalize()
{
    scalar_type mag = InverseSqrt(SqrMagnitude());
    x /= mag;
    y /= mag;
    z /= mag;
    w /= mag;
}

void Vector::SafeNormalize()
{
    scalar_type mag = Magnitude();
    x /= mag;
    y /= mag;
    z /= mag;
    w /= mag;
}

Vector::vector_type Vector::Normalized() const
{
    vector_type r = *this;
    r.Normalize();
    return r;
}

Vector::vector_type Vector::SafeNormalized() const
{
    vector_type r = *this;
    r.SafeNormalize();
    return r;
}

void Vector::Splat(const scalar_type v)
{
    x = y = z = v;
    w = 1.0f;
}

Vector::scalar_type Vector::GetX() const
{
    return x;
}

Vector::scalar_type Vector::GetY() const
{
    return y;
}

Vector::scalar_type Vector::GetZ() const
{
    return z;
}

Vector::scalar_type Vector::GetW() const
{
    return w;
}

void Vector::SetX(const scalar_type v)
{
    x = v;
}

void Vector::SetY(const scalar_type v)
{
    y = v;
}

void Vector::SetZ(const scalar_type v)
{
    z = v;
}

void Vector::SetW(const scalar_type v)
{
    w = v;
}

void Vector::GetAll(scalar_type v[MAX_COMPONENT]) const
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
    v[3] = w;
}

void Vector::SetAll(const scalar_type inX, const scalar_type inY, const scalar_type inZ, const scalar_type inW)
{
    x = inX;
    y = inY;
    z = inZ;
    w = inW;
}

void Vector::SetAll(const scalar_type v[MAX_COMPONENT])
{
    x = v[0];
    y = v[1];
    z = v[2];
    w = v[3];
}

Vector::vector_type& Vector::operator=(const vector_type& other)
{
    x = other.x;
    y = other.y;
    z = other.z;
    w = other.w;
    return *this;
}

Vector::vector_type& Vector::operator=(vector_type&& other)
{
    x = std::move(other.x);
    y = std::move(other.y);
    z = std::move(other.z);
    w = std::move(other.w);
    return *this;
}

Vector::vector_type& Vector::operator+=(const vector_type& other)
{
    x += other.x;
    y += other.y;
    z += other.z;
    w += other.w;
    return *this;
}

Vector::vector_type& Vector::operator-=(const vector_type& other)
{
    x -= other.x;
    y -= other.y;
    z -= other.z;
    w -= other.w;
    return *this;
}

Vector::vector_type& Vector::operator*=(const vector_type& other)
{
    x *= other.x;
    y *= other.y;
    z *= other.z;
    w *= other.w;
    return *this;
}

Vector::vector_type& Vector::operator*=(const scalar_type other)
{
    x *= other;
    y *= other;
    z *= other;
    w *= other;
    return *this;
}

Vector::vector_type& Vector::operator/=(const vector_type& other)
{
    x /= other.x;
    y /= other.y;
    z /= other.z;
    w /= other.w;
    return *this;
}

Vector::vector_type& Vector::operator/=(const scalar_type other)
{
    x /= other;
    y /= other;
    z /= other;
    w /= other;
    return *this;
}

Vector::vector_type Vector::operator+(const vector_type& other) const
{
    return vector_type(
        x + other.x,
        y + other.y,
        z + other.z,
        w + other.w
    );
}

Vector::vector_type Vector::operator-(const vector_type& other) const
{
    return vector_type(
        x - other.x,
        y - other.y,
        z - other.z,
        w - other.w
    );
}

Vector::vector_type Vector::operator*(const vector_type& other) const
{
    return vector_type(
        x * other.x,
        y * other.y,
        z * other.z,
        w * other.w
    );
}

Vector::vector_type Vector::operator*(const scalar_type other) const
{
    return vector_type(
        x * other,
        y * other,
        z * other,
        w * other
    );
}

Vector::vector_type Vector::operator/(const vector_type& other) const
{
    return vector_type(
        x / other.x,
        y / other.y,
        z / other.z,
        w / other.w
    );
}

Vector::vector_type Vector::operator/(const scalar_type other) const
{
    return vector_type(
        x / other,
        y / other,
        z / other,
        w / other
    );
}

bool Vector::operator==(const vector_type& other) const
{
    return x == other.x && y == other.y && z == other.z && w == other.z;
}

bool Vector::operator!=(const vector_type& other) const
{
    return x != other.x && y != other.y && z != other.z && w != other.z;
}

Vector::vector_type Vector::operator-() const
{
    return vector_type(-x, -y, -z, -w);
}

Vector::scalar_type& Vector::operator[](const size_t index)
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

const Vector::scalar_type& Vector::operator[](const size_t index) const
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
#endif

Vector::vector_type Vector::Lerp(const vector_type& a, const vector_type& b, const scalar_type t)
{
    return a + (b - a) * t;
}

Vector::vector_type Vector::Slerp(const vector_type& a, const vector_type& b, const scalar_type t)
{
    scalar_type dot = Clamp(Dot(a, b), scalar_type(-1.0f), scalar_type(1.0f));
    scalar_type theta = std::acosf(dot) * t;
    vector_type relativeVec = b - a * t;
    relativeVec.Normalize();
    return ((a * std::cosf(theta)) + relativeVec * std::sinf(theta));
}

Vector::vector_type Vector::Reflect(const vector_type& direction, const vector_type& normal)
{
    return direction - 2 * Dot(direction, normal) * normal;
}

Vector::vector_type Vector::Refract(const vector_type& direction, const vector_type& normal, const scalar_type theta)
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

Vector::vector_type Vector::FaceForward(const vector_type& direction, const vector_type& normal, const vector_type& normalRef)
{
    return Dot(normalRef, direction) < 0 ? normal : -normal;
}

Vector::vector_type Vector::Project(const vector_type& a, const vector_type& b)
{
    scalar_type dot = Dot(a, b);
    scalar_type mag = b.Magnitude();
    return (dot / mag) * b;
}

Vector::scalar_type Vector::Distance(const vector_type& a, const vector_type& b)
{
    return (a - b).Magnitude();
}

Vector::scalar_type Vector::SqrDistance(const vector_type& a, const vector_type& b)
{
    return (a - b).SqrMagnitude();
}

Vector::vector_type Vector::ClampMagnitude(const vector_type& v, const scalar_type length)
{
    return v.Normalized() * length;
}

Vector::vector_type Vector::Normalized() const
{
    vector_type r = *this;
    r.Normalize();
    return r;
}

Vector::vector_type Vector::SafeNormalized() const
{
    vector_type r = *this;
    r.SafeNormalize();
    return r;
}
} // namespace lf


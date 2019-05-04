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
namespace lf
{
    Quaternion::Quaternion()
    {
        mComponents[0] = mComponents[1] = mComponents[2] = mComponents[3] = 0.0f;
    }
    Quaternion::Quaternion(const Quaternion& other)
    {
        memcpy(mComponents, other.mComponents, sizeof(mComponents));
    }
    Quaternion::Quaternion(Quaternion&& other)
    {
        memcpy(mComponents, other.mComponents, sizeof(mComponents));
        other.mComponents[0] = other.mComponents[1] = other.mComponents[2] = 0.0f;
        other.mComponents[3] = 1.0f;
    }
    Quaternion::Quaternion(LazyTag)
    {

    }
    Quaternion::Quaternion(Float32 x, Float32 y, Float32 z, Float32 w)
    {
        mComponents[0] = x;
        mComponents[1] = y;
        mComponents[2] = z;
        mComponents[3] = w;
    }

    Quaternion::Quaternion(Float32 eulerX, Float32 eulerY, Float32 eulerZ)
    {
        SetEulerAngles(eulerX, eulerY, eulerZ);
    }

    Quaternion::scalar_type Quaternion::Dot(const quaternion_type& a, const quaternion_type& b)
    {
        return a.mComponents[0] * b.mComponents[0] + a.mComponents[1] * b.mComponents[1] + a.mComponents[2] * b.mComponents[2] + a.mComponents[3] * b.mComponents[3];
    }

    Quaternion::quaternion_type Quaternion::Cross(const quaternion_type& a, const quaternion_type& b)
    {
        return quaternion_type(
            a.mComponents[3] * b.mComponents[3] - a.mComponents[0] * b.mComponents[0] - a.mComponents[1] * b.mComponents[1] - a.mComponents[2] * b.mComponents[2],
            a.mComponents[3] * b.mComponents[0] + a.mComponents[0] * b.mComponents[3] + a.mComponents[1] * b.mComponents[2] - a.mComponents[2] * b.mComponents[1],
            a.mComponents[3] * b.mComponents[1] + a.mComponents[1] * b.mComponents[3] + a.mComponents[2] * b.mComponents[0] - a.mComponents[0] * b.mComponents[2],
            a.mComponents[3] * b.mComponents[2] + a.mComponents[2] * b.mComponents[3] + a.mComponents[0] * b.mComponents[1] - a.mComponents[1] * b.mComponents[0]);
    }

    Quaternion::quaternion_type Quaternion::Lerp(const quaternion_type& a, const quaternion_type& b, const scalar_type t)
    {
        return a * (1.0f - t) + (b * a);
    }
    Quaternion::quaternion_type Quaternion::Slerp(const quaternion_type& a, const quaternion_type& b, const scalar_type t)
    {
        quaternion_type z = b;
        scalar_type cosTheta = Dot(a,b);
        if (cosTheta < 0.0f)
        {
            z = -b;
            cosTheta = -cosTheta;
        }

        if (ApproxEquals(cosTheta, 1.0f))
        {
            return quaternion_type(
                ::lf::Lerp(a.mComponents[0], z.mComponents[0], t),
                ::lf::Lerp(a.mComponents[1], z.mComponents[1], t),
                ::lf::Lerp(a.mComponents[2], z.mComponents[2], t),
                ::lf::Lerp(a.mComponents[3], z.mComponents[3], t)
            );
        }
        scalar_type angle = std::acosf(cosTheta);
        return (a * std::sinf(1.0f - t * angle) + z * std::sinf(t * angle)) / std::sinf(angle);
    }
    Quaternion::quaternion_type Quaternion::LookRotation(const vector_type& forward, const vector_type& up)
    {
        Quaternion q1 = RotationBetween(Vector::Forward, forward);
        Vector right = Vector::Cross(forward, up);
        Vector yaxis = Vector::Cross(right, forward);
        Vector upAxis = q1 * Vector::Up;
        Quaternion q2 = RotationBetween(upAxis, yaxis);
        return q2 * q1;
    }
    Quaternion::quaternion_type Quaternion::LookRotation(const vector_type& forward)
    {
        Quaternion q1 = RotationBetween(Vector::Forward, forward);
        Vector right = Vector::Cross(forward, Vector::Up);
        Vector yaxis = Vector::Cross(right, forward);
        Vector upAxis = q1 * Vector::Up;
        Quaternion q2 = RotationBetween(upAxis, yaxis);
        return q2 * q1;
    }
    // Quaternion::quaternion_type Quaternion::RotateTowards(const quaternion_type& from, const quaternion_type& to, const scalar_type degrees)
    // {
    // 
    // }
    Quaternion::quaternion_type Quaternion::RotationBetween(const vector_type& a, const vector_type& b)
    {
        Vector aNorm = a.Normalized();
        Vector bNorm = b.Normalized();

        Float32 dot = Vector::Dot(aNorm, bNorm);
        Vector rotationAxis = Vector(LAZY);

        if (ApproxEquals(dot, -1.0f))
        {
            rotationAxis = Vector::Cross(Vector::Forward, aNorm);
            if (rotationAxis.Magnitude() < 0.01f)
            {
                rotationAxis = Vector::Cross(Vector::Right, aNorm);
            }
            rotationAxis.Normalize();
            return AngleAxis(180.0f, rotationAxis);
        }
        rotationAxis = Vector::Cross(aNorm, bNorm);
        Float32 sqr = sqrtf((1.0f + dot) * 2.0f);
        Float32 inverse = 1 / sqr;

        Float32 components[4];
        rotationAxis.GetAll(components);

        return Quaternion::Quaternion(components[0] * inverse, components[1] * inverse, components[2] * inverse, sqr * 0.5f);
    }

    Quaternion::quaternion_type Quaternion::AngleAxis(scalar_type angle, const vector_type& vec)
    {
        angle = Deg2Rad(angle);
        scalar_type halfSign = sinf(angle * 0.5f);
        scalar_type components[4];
        vec.GetAll(components);
        return quaternion_type(components[0] * halfSign, components[1] * halfSign, components[2] * halfSign, cosf(angle * 0.5f));
    }

    Quaternion Quaternion::Conjugate() const
    {
        return Quaternion(-mComponents[0], -mComponents[1], -mComponents[2], mComponents[3]);
    }

    Quaternion Quaternion::Inverse() const
    {
        return Conjugate() / Dot(*this,*this);
    }

    Float32 Quaternion::Magnitude() const
    {
        return sqrtf(mComponents[0] * mComponents[0] + mComponents[1] * mComponents[1] + mComponents[2] * mComponents[2] + mComponents[3] * mComponents[3]);
    }

    Float32 Quaternion::SqrMagnitude() const
    {
        return mComponents[0] * mComponents[0] + mComponents[1] * mComponents[1] + mComponents[2] * mComponents[2] + mComponents[3] * mComponents[3];
    }

    void Quaternion::Normalize()
    {
        scalar_type length = Magnitude();
        if (ApproxEquals(length, 0.0f))
        {
            return;
        }

        mComponents[0] /= length;
        mComponents[1] /= length;
        mComponents[2] /= length;
        mComponents[3] /= length;
    }

    Quaternion Quaternion::Normalized() const
    {
        scalar_type length = Magnitude();
        if (ApproxEquals(length, 0.0f))
        {
            return Quaternion();
        }
        return Quaternion(mComponents[0] / length,
            mComponents[1] / length,
            mComponents[2] / length,
            mComponents[3] / length);
    }

    void Quaternion::Splat(const scalar_type scalar)
    {
        mComponents[0] = mComponents[1] = mComponents[2] = mComponents[3] = scalar;
    }

    void Quaternion::SetEulerAngles(Float32 x, Float32 y, Float32 z)
    {
        Float32 radX = Deg2Rad(x) * 0.5f;
        Float32 radY = Deg2Rad(y) * 0.5f;
        Float32 radZ = Deg2Rad(z) * 0.5f;

        quaternion_type qx(sinf(radX), 0.0f, 0.0f, cosf(radX));
        quaternion_type qy(0.0f, sinf(radY), 0.0f, cosf(radY));
        quaternion_type qz(0.0f, 0.0f, sinf(radZ), cosf(radZ));
        *this = qy * qx * qz;
    }

    Quaternion::vector_type Quaternion::GetEulerAngles() const
    {
        const Float32& x = mComponents[0];
        const Float32& y = mComponents[1];
        const Float32& z = mComponents[2];
        const Float32& w = mComponents[3];

        Float32 yy = y * y;
        Float32 t0 = 2.0f * (w * x + y * z);
        Float32 t1 = 1.0f - 2.0f * (x * x + yy);
        Float32 roll = Rad2Deg(std::atan2f(t0, t1));

        Float32 t2 = 2.0f * (w * y - z * x);
        t2 = t2 > 1.0f ? 1.0f : t2;
        t2 = t2 < -1.0f ? -1.0f : t2;
        Float32 pitch = Rad2Deg(std::asinf(t2));

        Float32 t3 = 2.0f * (w * z + x * y);
        Float32 t4 = 1.0f - 2.0f * (yy + z*z);
        Float32 yaw = Rad2Deg(std::atan2(t3, t4));
        return Vector(roll < 0.0f ? 360.0f + roll : roll, pitch < 0.0f ? 360.0f + pitch : pitch, yaw < 0.0f ? 360.0f + yaw : yaw);
    }

    Quaternion::scalar_type Quaternion::Pitch() const
    {
        return Rad2Deg(std::atan2f(2.0f * (mComponents[1] * mComponents[2] * mComponents[3] * mComponents[0]),
            mComponents[3] * mComponents[3] - mComponents[0] * mComponents[0] - mComponents[1] * mComponents[1] + mComponents[2] * mComponents[2]));
    }

    Quaternion::scalar_type Quaternion::Yaw() const
    {
        return Rad2Deg(std::asin(-2.0f * (mComponents[0] * mComponents[2] - mComponents[3] * mComponents[1])));
    }

    Quaternion::scalar_type Quaternion::Roll() const
    {
        return Rad2Deg(std::atan2f(2.0f * (mComponents[0] * mComponents[1] * mComponents[3] * mComponents[2]),
            mComponents[3] * mComponents[3] + mComponents[0] * mComponents[0] - mComponents[1] * mComponents[1] - mComponents[2] * mComponents[2]));
    }

    void Quaternion::SetAll(const Float32 components[4])
    {
        memcpy(mComponents, components, sizeof(mComponents));
    }

    void Quaternion::SetAll(const Float32 x, const Float32 y, const Float32 z, const Float32 w)
    {
        mComponents[0] = x;
        mComponents[1] = y;
        mComponents[2] = z;
        mComponents[3] = w;
    }


    void Quaternion::GetAll(Float32 components[4]) const
    {
        memcpy(components, mComponents, sizeof(mComponents));
    }

    Quaternion& Quaternion::operator=(const Quaternion& other)
    {
        memcpy(mComponents, other.mComponents, sizeof(mComponents));
        return *this;
    }

    Quaternion Quaternion::operator+(const Quaternion& other) const
    {
        return Quaternion(mComponents[0] + other.mComponents[0],
            mComponents[1] + other.mComponents[1],
            mComponents[2] + other.mComponents[2],
            mComponents[3] + other.mComponents[3]);
    }

    Quaternion Quaternion::operator-(const Quaternion& other) const
    {
        return Quaternion(mComponents[0] - other.mComponents[0],
            mComponents[1] - other.mComponents[1],
            mComponents[2] - other.mComponents[2],
            mComponents[3] - other.mComponents[3]);
    }

    Quaternion Quaternion::operator*(Float32 scalar) const
    {
        return Quaternion(mComponents[0] * scalar,
            mComponents[1] * scalar,
            mComponents[2] * scalar,
            mComponents[3] * scalar);
    }

    Quaternion Quaternion::operator/(Float32 scalar) const
    {
        return Quaternion(mComponents[0] / scalar,
            mComponents[1] / scalar,
            mComponents[2] / scalar,
            mComponents[3] / scalar);
    }

    Quaternion Quaternion::operator*(const Quaternion& other) const
    {
        return Quaternion(
             mComponents[0] * other.mComponents[3] + mComponents[1] * other.mComponents[2] - mComponents[2] * other.mComponents[1] + mComponents[3] * other.mComponents[0],
            -mComponents[0] * other.mComponents[2] + mComponents[1] * other.mComponents[3] + mComponents[2] * other.mComponents[0] + mComponents[3] * other.mComponents[1],
             mComponents[0] * other.mComponents[1] - mComponents[1] * other.mComponents[0] + mComponents[2] * other.mComponents[3] + mComponents[3] * other.mComponents[2],
            -mComponents[0] * other.mComponents[0] - mComponents[1] * other.mComponents[1] - mComponents[2] * other.mComponents[2] + mComponents[3] * other.mComponents[3]
        );
    }

    Quaternion::vector_type Quaternion::operator*(const vector_type& vec) const
    {
        scalar_type xx = mComponents[0] * mComponents[0];
        scalar_type yy = mComponents[1] * mComponents[1];
        scalar_type zz = mComponents[2] * mComponents[2];
        scalar_type ww = mComponents[3] * mComponents[3];
        scalar_type xy = mComponents[0] * mComponents[1];
        scalar_type xz = mComponents[0] * mComponents[2];
        scalar_type yz = mComponents[1] * mComponents[2];
        scalar_type wx = mComponents[3] * mComponents[0];
        scalar_type wy = mComponents[3] * mComponents[1];
        scalar_type wz = mComponents[3] * mComponents[2];

        scalar_type v[4];
        scalar_type r[4];
        vec.GetAll(v);

        r[0] = v[0] * (xx + ww - yy - zz) + v[1] * (2.0f * xy - 2.0f * wz) + v[2] * (2.0f * xz + 2.0f * wy);
        r[1] = v[0] * (2.0f * wz + 2.0f * xy) + v[1] * (ww - xx + yy - zz) + v[2] * (-2.0f * wx + 2.0f * yz);
        r[2] = v[0] * (-2.0f * wy + 2.0f * xz) + v[1] * (2 * wx + 2.0f * yz) + v[2] * (ww - xx - yy + zz);
        return vector_type(r[0], r[1], r[2], v[3]);
    }

    bool Quaternion::operator==(const Quaternion& other) const
    {
        return mComponents[0] == other.mComponents[0] && mComponents[1] == other.mComponents[1] && mComponents[2] == other.mComponents[2] && mComponents[3] == other.mComponents[3];
    }
    bool Quaternion::operator!=(const Quaternion& other) const
    {
        return mComponents[0] != other.mComponents[0] || mComponents[1] != other.mComponents[1] || mComponents[2] != other.mComponents[2] || mComponents[3] != other.mComponents[3];
    }
    Quaternion::scalar_type Quaternion::operator[](const size_t i) const
    {
        Assert(i < LF_ARRAY_SIZE(mComponents));
        return mComponents[i];
    }

}

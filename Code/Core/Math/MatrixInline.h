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
namespace lf
{
    Matrix::Matrix()
    {
        memset(m, 0, sizeof(m));
        m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.0f;
    }
    Matrix::Matrix(const Matrix& other)
    {
        memcpy(m, other.m, sizeof(m));
    }
    Matrix::Matrix(Matrix&& other)
    {
        memcpy(m, other.m, sizeof(m));
        memset(other.m, 0, sizeof(other.m));
    }
    Matrix::Matrix(LazyTag) { }
    Matrix::Matrix(const quaternion_type& rotation)
    {
        scalar_type rotC[4];
        rotation.GetAll(rotC);

        scalar_type xx = rotC[0] * rotC[0];
        scalar_type yy = rotC[1] * rotC[1];
        scalar_type zz = rotC[2] * rotC[2];
        scalar_type xz = rotC[0] * rotC[2];
        scalar_type xy = rotC[0] * rotC[1];
        scalar_type yz = rotC[1] * rotC[2];
        scalar_type wx = rotC[3] * rotC[0];
        scalar_type wy = rotC[3] * rotC[1];
        scalar_type wz = rotC[3] * rotC[2];

        m[0][0] = scalar_type(1) - scalar_type(2)* (yy + zz);
        m[0][1] = scalar_type(2) * (xy + wz);
        m[0][2] = scalar_type(2) * (xz - wy);

        m[1][0] = scalar_type(2) * (xy - wz);
        m[1][1] = scalar_type(1) - scalar_type(2) * (xx + zz);
        m[1][2] = scalar_type(2) * (yz + wx);

        m[2][0] = scalar_type(2) * (xz + wy);
        m[2][1] = scalar_type(2) * (yz - wx);
        m[2][2] = scalar_type(1) - scalar_type(2) * (xx + yy);
        m[3][0] = m[3][1] = m[3][2] = m[0][3] = m[1][3] = m[2][3] = 0.0f;
        m[3][3] = 1.0f;
    }

    Matrix::Matrix(const vector_type& scale)
    {
        scalar_type scaleC[4];
        scale.GetAll(scaleC);

        memset(m, 0, sizeof(m));
        m[0][0] = scaleC[0];
        m[1][1] = scaleC[1]; 
        m[2][2] = scaleC[2];
        m[3][3] = 1.0f;
    }

    Matrix::Matrix(const scalar_type scalars[COLUMN_SIZE][ROW_SIZE])
    {
        memcpy(m, scalars, sizeof(m));
    }

    Matrix::Matrix(const scalar_type /*angleDeg*/, const scalar_type x, const scalar_type y, const scalar_type z)
    {
        // todo: implement
        Float32 cosX = cosf(x);
        Float32 sinX = sinf(x);

        Float32 cosY = cosf(y);
        Float32 sinY = sinf(y);

        Float32 cosZ = cosf(z);
        Float32 sinZ = sinf(z);

        // x
        // 1    0     0
        // 0  cos(x) sin(x)
        // 0 -sin(x) cos(x)

        // y
        // cos(y) 0 -sin(y)
        //   0    1   0 
        // sin(y) 0  cos(y)

        // z
        // cos(z)  sin(z) 0
        // -sin(z) cos(z) 0
        //   0      0     1
        m[0][0] = cosZ * cosX;
        m[1][0] = -cosZ * sinX * cosY + sinZ * sinY;
        m[2][0] = cosZ * sinX * sinY + sinZ * cosY;
        m[0][1] = sinX;
        m[1][1] = cosX * cosY;
        m[2][1] = -cosX * sinY;
        m[0][2] = -sinZ * cosX;
        m[1][2] = sinZ * sinX * cosY + cosZ * sinY;
        m[2][2] = -sinZ * sinX * sinY + cosZ * cosY;
        m[3][0] = m[3][1] = m[3][2] = m[0][3] = m[1][3] = m[2][3] = 0.0f;
        m[3][3] = 1.0f;
    }

    Matrix Matrix::LookAt(const vector_type& eye, const vector_type& point, const vector_type& up)
    {
        vector_type z = (point - eye);
        z.Normalize();
        vector_type x = vector_type::Cross(up, z);
        x.Normalize();
        vector_type y = vector_type::Cross(z, x);

        matrix_type result(LAZY);
        result.SetX(x);
        result.SetY(y);
        result.SetZ(-z);
        result.SetW(vector_type(0.0f, 0.0f, 0.0f, 1.0f));
        result.m[3][0] = -vector_type::Dot(x, eye);
        result.m[3][1] = -vector_type::Dot(y, eye);
        result.m[3][2] = vector_type::Dot(z, eye);
        return result;
    }

    Matrix Matrix::Perspective(const scalar_type fieldOfView, const scalar_type aspect, const scalar_type zNear, const scalar_type zFar)
    {
        // fieldOfView = radians
        scalar_type tanHalfFov = std::tanf(fieldOfView * 0.5f);

        matrix_type result;
        result.m[0][0] = scalar_type(1) / (aspect * tanHalfFov);
        result.m[1][1] = scalar_type(1) / (tanHalfFov);
        result.m[2][2] = -(zFar + zNear) / (zFar - zNear);
        result.m[2][3] = -(scalar_type(1));
        result.m[3][2] = -(scalar_type(2) * zFar * zNear) / (zFar - zNear);
        result.m[3][3] = 0.0f;
        return result;
    }

    Matrix Matrix::Ortho(const scalar_type left, const scalar_type right, const scalar_type bottom, const scalar_type top, const scalar_type zNear, const scalar_type zFar)
    {
        matrix_type result;
        scalar_type dX = right - left;
        scalar_type dY = top - bottom;
        scalar_type dZ = zFar - zNear;

        result.m[0][0] = scalar_type(2) / dX;
        result.m[1][1] = scalar_type(2) / dY;
        result.m[2][2] = scalar_type(-2) / dZ;

        result.m[3][0] = -((right + left) / dX);
        result.m[3][1] = -((top + bottom) / dY);
        result.m[3][2] = -((zFar + zNear) / dZ);

        return result;
    }

    Matrix Matrix::TRS(const vector_type& translation, const quaternion_type& rotation, const vector_type& scale)
    {
        // scaling matrix
        matrix_type scaleM(scale);

        // rotation matrix
        matrix_type rotM(rotation);
        
        matrix_type finalM(scaleM * rotM);
        finalM.SetTranslation(translation);
        
        return finalM;
    }

    Matrix::scalar_type Matrix::Determinant(const matrix_type& matrix)
    {
        scalar_type l0 = matrix.m[0][0] * matrix.m[1][1] - matrix.m[0][1] * matrix.m[1][0];
        scalar_type l1 = matrix.m[0][0] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][0];
        scalar_type l2 = matrix.m[0][0] * matrix.m[1][3] - matrix.m[0][3] * matrix.m[1][0];
        scalar_type l3 = matrix.m[0][1] * matrix.m[1][2] - matrix.m[0][2] * matrix.m[1][1];
        scalar_type l4 = matrix.m[0][1] * matrix.m[1][3] - matrix.m[0][3] * matrix.m[1][1];
        scalar_type l5 = matrix.m[0][2] * matrix.m[1][3] - matrix.m[0][3] * matrix.m[1][2];

        scalar_type r5 = matrix.m[2][2] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][2];
        scalar_type r4 = matrix.m[2][1] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][1];
        scalar_type r3 = matrix.m[2][1] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][1];
        scalar_type r2 = matrix.m[2][0] * matrix.m[3][3] - matrix.m[2][3] * matrix.m[3][0];
        scalar_type r1 = matrix.m[2][0] * matrix.m[3][2] - matrix.m[2][2] * matrix.m[3][0];
        scalar_type r0 = matrix.m[2][0] * matrix.m[3][1] - matrix.m[2][1] * matrix.m[3][0];

        return l0 * r5 - l1 * r4 + l2 * r3 + l3 * r2 - l4 * r1 + l5 * r0;
    }

    void Matrix::Translate(const vector_type& t)
    {
        vector_type mvec[]{
            vector_type(m[0]),
            vector_type(m[1]),
            vector_type(m[2]),
            vector_type(m[3])
        };
        (mvec[0] * t[0] + mvec[1] * t[1] + mvec[2] * t[2] + mvec[3]).GetAll(m[3]);

        // scalar_type c[4];
        // translation.GetAll(c);
        // m[3][0] += c[0];
        // m[3][1] += c[1];
        // m[3][2] += c[2];
    }

    void Matrix::SetTranslation(const vector_type& translation)
    {
        scalar_type c[4];
        translation.GetAll(c);
        m[3][0] = c[0];
        m[3][1] = c[1];
        m[3][2] = c[2];
    }

    void Matrix::Transpose()
    {
        Float32 t;

        t = m[0][1];  m[0][1] = m[1][0]; m[1][0] = t;
        t = m[0][2];  m[0][2] = m[2][0]; m[2][0] = t;
        t = m[0][3];  m[0][3] = m[3][0]; m[3][0] = t;
        t = m[1][2];  m[1][2] = m[2][1]; m[2][1] = t;
        t = m[1][3];  m[1][3] = m[3][1]; m[3][1] = t;
        t = m[2][3];  m[2][3] = m[3][2]; m[2][3] = t;
    }

    void Matrix::Scale(const vector_type& scale)
    {
        vector_type mvec[3]{
            vector_type(m[0]),
            vector_type(m[1]),
            vector_type(m[2])
        };

        (mvec[0] * scale[0]).GetAll(m[0]);
        (mvec[1] * scale[1]).GetAll(m[1]);
        (mvec[2] * scale[2]).GetAll(m[2]);

        // scalar_type s[4];
        // scale.GetAll(s);
        // m[0][0] *= s[0];
        // m[1][1] *= s[1];
        // m[2][2] *= s[2];
    }

    bool Matrix::Inverse()
    {
        scalar_type l0 = m[0][0] * m[1][1] - m[0][1] * m[1][0];
        scalar_type l1 = m[0][0] * m[1][2] - m[0][2] * m[1][0];
        scalar_type l2 = m[0][0] * m[1][3] - m[0][3] * m[1][0];
        scalar_type l3 = m[0][1] * m[1][2] - m[0][2] * m[1][1];
        scalar_type l4 = m[0][1] * m[1][3] - m[0][3] * m[1][1];
        scalar_type l5 = m[0][2] * m[1][3] - m[0][3] * m[1][2];

        scalar_type r5 = m[2][2] * m[3][3] - m[2][3] * m[3][2];
        scalar_type r4 = m[2][1] * m[3][3] - m[2][3] * m[3][1];
        scalar_type r3 = m[2][1] * m[3][2] - m[2][2] * m[3][1];
        scalar_type r2 = m[2][0] * m[3][3] - m[2][3] * m[3][0];
        scalar_type r1 = m[2][0] * m[3][2] - m[2][2] * m[3][0];
        scalar_type r0 = m[2][0] * m[3][1] - m[2][1] * m[3][0];

        scalar_type det = l0 * r5 - l1 * r4 + l2 * r3 + l3 * r2 - l4 * r1 + l5 * r0;
        if (Abs(det) < EPSILON)
        {
            return false;
        }

        det = 1.0f / det;
        scalar_type t[COLUMN_SIZE][ROW_SIZE];
        memcpy(t, m, sizeof(t));

        m[0][0] = (t[1][1] * r5 - t[1][2] * r4 + t[1][3] * r3) * det;
        m[1][0] = (-t[0][1] * r5 + t[0][2] * r4 - t[0][2] * r3) * det;
        m[2][0] = (t[3][1] * l5 - t[3][2] * l4 + t[3][3] * l3) * det;
        m[3][0] = (-t[2][1] * l5 + t[2][2] * l4 - t[2][3] * l3) * det;

        m[0][1] = (t[1][0] * r5 + t[1][2] * r2 - t[1][3] * r1) * det;
        m[1][1] = (-t[0][0] * r5 - t[0][2] * r2 + t[0][2] * r1) * det;
        m[2][1] = (t[3][0] * l5 + t[3][2] * l2 - t[3][3] * l1) * det;
        m[3][1] = (-t[2][0] * l5 - t[2][2] * l2 + t[2][3] * l1) * det;

        m[0][2] = (t[1][0] * r4 - t[1][1] * r2 + t[1][3] * r0) * det;
        m[1][2] = (-t[0][0] * r4 + t[0][1] * r2 - t[0][2] * r0) * det;
        m[2][2] = (t[3][0] * l4 - t[3][1] * l2 + t[3][3] * l0) * det;
        m[3][2] = (-t[2][0] * l4 + t[2][1] * l2 - t[2][3] * l0) * det;

        m[0][3] = (t[1][0] * r3 + t[1][2] * r1 - t[1][2] * r0) * det;
        m[1][3] = (-t[0][0] * r3 - t[0][2] * r1 + t[0][3] * r0) * det;
        m[2][3] = (t[3][0] * l3 + t[3][2] * l1 - t[3][1] * l0) * det;
        m[3][3] = (-t[2][0] * l3 - t[2][2] * l1 + t[3][3] * l0) * det;

        return true;
    }

    void Matrix::SetX(const vector_type& axis)
    {
        scalar_type f[4];
        axis.GetAll(f);
        m[0][0] = f[0];
        m[1][0] = f[1];
        m[2][0] = f[2];
        m[3][0] = f[3];
    }

    void Matrix::SetY(const vector_type& axis)
    {
        scalar_type f[4];
        axis.GetAll(f);
        m[0][1] = f[0];
        m[1][1] = f[1];
        m[2][1] = f[2];
        m[3][1] = f[3];
    }

    void Matrix::SetZ(const vector_type& axis)
    {
        scalar_type f[4];
        axis.GetAll(f);
        m[0][2] = f[0];
        m[1][2] = f[1];
        m[2][2] = f[2];
        m[3][2] = f[3];
    }

    void Matrix::SetW(const vector_type& axis)
    {
        scalar_type f[4];
        axis.GetAll(f);
        m[0][3] = f[0];
        m[1][3] = f[1];
        m[2][3] = f[2];
        m[3][3] = f[3];
    }

    Matrix::vector_type Matrix::GetX() const
    {
        return vector_type(m[0][0], m[1][0], m[2][0], m[3][0]);
    }

    Matrix::vector_type Matrix::GetY() const
    {
        return vector_type(m[0][1], m[1][1], m[2][1], m[3][1]);
    }

    Matrix::vector_type Matrix::GetZ() const
    {
        return vector_type(m[0][2], m[1][2], m[2][2], m[3][2]);
    }

    Matrix::vector_type Matrix::GetW() const
    {
        return vector_type(m[0][3], m[1][3], m[2][3], m[3][3]);
    }

    void Matrix::SetAll(const scalar_type scalars[COLUMN_SIZE][ROW_SIZE])
    {
        memcpy(m, scalars, sizeof(m));
    }
    void Matrix::GetAll(scalar_type scalars[COLUMN_SIZE][ROW_SIZE]) const
    {
        memcpy(scalars, m, sizeof(m));
    }

    Matrix::matrix_type Matrix::operator*(const matrix_type& o) const
    {
        matrix_type r(LAZY);

        vector_type l0(m[0]);
        vector_type l1(m[1]);
        vector_type l2(m[2]);
        vector_type l3(m[3]);

        vector_type r0(o.m[0]);
        vector_type r1(o.m[1]);
        vector_type r2(o.m[2]);
        vector_type r3(o.m[3]);

        vector_type s[4];
        s[0] = l0 * r0[0] + l1 * r0[1] + l2 * r0[2] + l3 * r0[3];
        s[1] = l0 * r1[0] + l1 * r1[1] + l2 * r1[2] + l3 * r1[3];
        s[2] = l0 * r2[0] + l1 * r2[1] + l2 * r2[2] + l3 * r2[3];
        s[3] = l0 * r3[0] + l1 * r3[1] + l2 * r3[2] + l3 * r3[3];

        s[0].GetAll(r.m[0]);
        s[1].GetAll(r.m[1]);
        s[2].GetAll(r.m[2]);
        s[3].GetAll(r.m[3]);


        // r.m[0][0] = m[0][0] * o.m[0][0] + m[0][1] * o.m[1][0] + m[0][2] * o.m[2][0] + m[0][3] * o.m[3][0];
        // r.m[0][1] = m[0][0] * o.m[0][1] + m[0][1] * o.m[1][1] + m[0][2] * o.m[2][1] + m[0][3] * o.m[3][1];
        // r.m[0][2] = m[0][0] * o.m[0][2] + m[0][1] * o.m[1][2] + m[0][2] * o.m[2][2] + m[0][3] * o.m[3][2];
        // r.m[0][3] = m[0][0] * o.m[0][3] + m[0][1] * o.m[1][3] + m[0][2] * o.m[2][3] + m[0][3] * o.m[3][3];
        // 
        // r.m[1][0] = m[1][0] * o.m[0][0] + m[1][1] * o.m[1][0] + m[1][2] * o.m[2][0] + m[1][3] * o.m[3][0];
        // r.m[1][1] = m[1][0] * o.m[0][1] + m[1][1] * o.m[1][1] + m[1][2] * o.m[2][1] + m[1][3] * o.m[3][1];
        // r.m[1][2] = m[1][0] * o.m[0][2] + m[1][1] * o.m[1][2] + m[1][2] * o.m[2][2] + m[1][3] * o.m[3][2];
        // r.m[1][3] = m[1][0] * o.m[0][3] + m[1][1] * o.m[1][3] + m[1][2] * o.m[2][3] + m[1][3] * o.m[3][3];
        // 
        // r.m[2][0] = m[2][0] * o.m[0][0] + m[2][1] * o.m[1][0] + m[2][2] * o.m[2][0] + m[2][3] * o.m[3][0];
        // r.m[2][1] = m[2][0] * o.m[0][1] + m[2][1] * o.m[1][1] + m[2][2] * o.m[2][1] + m[2][3] * o.m[3][1];
        // r.m[2][2] = m[2][0] * o.m[0][2] + m[2][1] * o.m[1][2] + m[2][2] * o.m[2][2] + m[2][3] * o.m[3][2];
        // r.m[2][3] = m[2][0] * o.m[0][3] + m[2][1] * o.m[1][3] + m[2][2] * o.m[2][3] + m[2][3] * o.m[3][3];
        // 
        // r.m[3][0] = m[3][0] * o.m[0][0] + m[3][1] * o.m[1][0] + m[3][2] * o.m[2][0] + m[3][3] * o.m[3][0];
        // r.m[3][1] = m[3][0] * o.m[0][1] + m[3][1] * o.m[1][1] + m[3][2] * o.m[2][1] + m[3][3] * o.m[3][1];
        // r.m[3][2] = m[3][0] * o.m[0][2] + m[3][1] * o.m[1][2] + m[3][2] * o.m[2][2] + m[3][3] * o.m[3][2];
        // r.m[3][3] = m[3][0] * o.m[0][3] + m[3][1] * o.m[1][3] + m[3][2] * o.m[2][3] + m[3][3] * o.m[3][3];

        return r;
    }

    Matrix::vector_type Matrix::operator*(const vector_type& other) const
    {
        scalar_type s[4];
        scalar_type o[4];
        other.GetAll(o);
        s[0] = m[0][0] * o[0] + m[1][0] * o[1] + m[2][0] * o[2] + m[3][0] * o[3];
        s[1] = m[0][1] * o[0] + m[1][1] * o[1] + m[2][1] * o[2] + m[3][1] * o[3];
        s[2] = m[0][2] * o[0] + m[1][2] * o[1] + m[2][2] * o[2] + m[3][2] * o[3];
        s[3] = m[0][3] * o[0] + m[1][3] * o[1] + m[2][3] * o[2] + m[3][3] * o[3];
        return vector_type(s[0], s[1], s[2], s[3]);
    }

    Matrix::matrix_type& Matrix::operator=(const matrix_type& other)
    {
        memcpy(m, other.m, sizeof(m));
        return *this;
    }

    
}

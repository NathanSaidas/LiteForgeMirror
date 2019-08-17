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
#ifndef LF_CORE_MATRIX_H
#define LF_CORE_MATRIX_H

#include "Core/Common/Types.h"
#include "Core/Common/Assert.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Quaternion.h"

#include <math.h>

namespace lf
{
    //  0  1  2  3 | m11 m21 m31 m41
    //  4  5  6  7 | m12 m22 m32 m42
    //  8  9 10 11 | m13 m23 m33 m43
    // 12 13 14 15 | m14 m24 m34 m44
    //
    // ISROT
    //
    class LF_CORE_API Matrix
    {
    public:
        // enum Rotation { ROTATION };
        // enum Scale { SCALE };
        // enum Translation { TRANSLATION };

        static const size_t ROW_SIZE = 4;
        static const size_t COLUMN_SIZE = 4;
        typedef Matrix matrix_type;
        typedef Float32 scalar_type;
        typedef Vector vector_type;
        typedef Quaternion quaternion_type;

        LF_INLINE Matrix();
        LF_INLINE Matrix(const Matrix& other);
        LF_INLINE Matrix(Matrix&& other);
        LF_INLINE Matrix(LazyTag);
        LF_INLINE Matrix(const quaternion_type& rotation);
        LF_INLINE Matrix(const vector_type& scale);
        LF_INLINE Matrix(const scalar_type scalars[COLUMN_SIZE][ROW_SIZE]);
        LF_INLINE Matrix(const scalar_type angleDeg, const scalar_type x, const scalar_type y, const scalar_type z);
        LF_INLINE ~Matrix() {}

        LF_INLINE static Matrix LookAt(const vector_type& from, const vector_type& to, const vector_type& up);
        LF_INLINE static Matrix Perspective(const scalar_type fieldOfView, const scalar_type aspect, const scalar_type zNear, const scalar_type zFar);
        LF_INLINE static Matrix Ortho(const scalar_type left, const scalar_type right, const scalar_type bottom, const scalar_type top, const scalar_type zNear, const scalar_type zFar);
        LF_INLINE static Matrix TRS(const vector_type& translation, const quaternion_type& rotation, const vector_type& scale);
        LF_INLINE static scalar_type Determinant(const matrix_type& matrix);

        LF_INLINE void Translate(const vector_type& translation);
        LF_INLINE void SetTranslation(const vector_type& translation);
        LF_INLINE void Transpose();
        LF_INLINE void Scale(const vector_type& scale);
        LF_INLINE bool Inverse();
        
        LF_INLINE void SetX(const vector_type& axis);
        LF_INLINE void SetY(const vector_type& axis);
        LF_INLINE void SetZ(const vector_type& axis);
        LF_INLINE void SetW(const vector_type& axis);
        LF_INLINE vector_type GetX() const;
        LF_INLINE vector_type GetY() const;
        LF_INLINE vector_type GetZ() const;
        LF_INLINE vector_type GetW() const;

        LF_INLINE void SetAll(const scalar_type scalars[COLUMN_SIZE][ROW_SIZE]);
        LF_INLINE void GetAll(scalar_type scalars[COLUMN_SIZE][ROW_SIZE]) const;
        LF_INLINE scalar_type& Get(const size_t x, const size_t y) { return m[x][y]; }
        LF_INLINE const scalar_type& Get(const size_t x, const size_t y) const { return m[x][y]; }

        LF_INLINE matrix_type operator*(const matrix_type& other) const;
        LF_INLINE vector_type operator*(const vector_type& other) const;
        LF_INLINE matrix_type& operator=(const matrix_type& other);
        
        LF_INLINE scalar_type* operator[](const size_t x) { return m[x]; }
        LF_INLINE const scalar_type* operator[](const size_t x) const { return m[x]; }

        // -- Transformations
        // TransformVector
        //LF_INLINE Vector TransformVector(const Vector& vec) const
        //{
        //    Float32 components[4];
        //    vec.GetAll(components);
        //    Vector result = Vector(
        //        m[0]  * components[0] + m[1]  * components[1] + m[2]  * components[2] + m[3]  * components[3],
        //        m[4]  * components[0] + m[5]  * components[1] + m[6]  * components[2] + m[7]  * components[3],
        //        m[8]  * components[0] + m[9]  * components[1] + m[10] * components[2] + m[11] * components[3],
        //        m[12] * components[0] + m[13] * components[1] + m[14] * components[2] + m[15] * components[3]);
        //    Float32 resultW = result[3];
        //    if (resultW)
        //    {
        //        return result / resultW;
        //    }
        //    return result;
        //}
         
    private:
        Float32 m[COLUMN_SIZE][ROW_SIZE];
    };
}

#include "MatrixInline.h"

#endif // LF_CORE_MATRIX_H
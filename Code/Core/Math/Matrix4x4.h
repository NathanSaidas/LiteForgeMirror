#ifndef LF_CORE_MATRIX_4X4_H
#define LF_CORE_MATRIX_4X4_H

#include "Core/Math/Matrix.h"

namespace lf
{
    class Matrix4x4
    {
    public:
        LF_FORCE_INLINE Matrix4x4();
        LF_FORCE_INLINE Matrix4x4(const Matrix4x4& other);
        LF_FORCE_INLINE Matrix4x4(const Matrix& other);

        Float32 m[4][4];
    };

}

#include "Matrix4x4Inline.h"

#endif // LF_CORE_MATRIX_4X4_H

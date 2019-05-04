#ifndef LF_CORE_MATH_COMBINED_H
#define LF_CORE_MATH_COMBINED_H

#include "Core/Math/Matrix.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"

namespace lf
{
    
    LF_INLINE Quaternion CastQuat(Matrix& m)
    {
        Float32 x = m[0][0] - m[1][1] - m[2][2];
        Float32 y = m[1][1] - m[0][0] - m[2][2];
        Float32 z = m[2][2] - m[0][0] - m[1][1];
        Float32 w = m[0][0] + m[1][1] + m[2][2];

        size_t largestIndex = 0;
        Float32 largestValue = w;
        if (x > largestValue)
        {
            largestIndex = 1;
            largestValue = x;
        }
        if (y > largestValue)
        {
            largestIndex = 2;
            largestValue = y;
        }
        if (z > largestValue)
        {
            largestIndex = 3;
            largestValue = z;
        }

        largestValue = sqrt(largestValue + 1.0f) * 0.5f;
        Float32 mult = 0.25f / largestValue;

        Quaternion result;
        switch (largestIndex)
        {
        case 0:
            return Quaternion(  m[1][2] - m[2][1] * mult,
                                m[2][0] - m[0][2] * mult,
                                m[0][1] - m[1][0] * mult,
                                largestValue);
        case 1:
            return Quaternion(  largestValue,
                                m[0][1] - m[1][0] * mult,
                                m[2][0] - m[0][2] * mult,
                                m[1][2] - m[2][1] * mult);
        case 2:
            return Quaternion(  m[0][1] - m[1][0] * mult,
                                largestValue,
                                m[1][2] - m[2][1] * mult,
                                m[2][0] - m[0][2] * mult
                );
        case 3:
            return Quaternion(  m[2][0] - m[0][2] * mult,
                                m[1][2] - m[2][1] * mult,
                                largestValue,
                                m[0][1] - m[1][0] * mult
                );
        default:
            Assert("Invalid index.");
        }
        return Quaternion::Identity;
    }

    namespace math
    {
        LF_INLINE Vector2 Abs(const Vector2& vec);
        LF_INLINE Vector3 Abs(const Vector3& vec);
        LF_INLINE Vector4 Abs(const Vector4& vec);
        LF_INLINE Vector  Abs(const Vector& vec);
        LF_INLINE bool    AllLessEqual(const Vector2& a, const Vector2& b);
        LF_INLINE bool    AllLessEqual(const Vector3& a, const Vector3& b);
        LF_INLINE bool    AllLessEqual(const Vector4& a, const Vector4& b);
        LF_INLINE bool    AllLessEqual(const Vector& a, const Vector& b);
        LF_INLINE bool    AllGreaterEqual(const Vector2& a, const Vector2& b);
        LF_INLINE bool    AllGreaterEqual(const Vector3& a, const Vector3& b);
        LF_INLINE bool    AllGreaterEqual(const Vector4& a, const Vector4& b);
        LF_INLINE bool    AllGreaterEqual(const Vector& a, const Vector& b);
        LF_INLINE bool    ApproxEquals(const Vector2& a, const Vector2& b, const Vector2::scalar_type epsilon = 0.0000000001f);
        LF_INLINE bool    ApproxEquals(const Vector3& a, const Vector3& b, const Vector3::scalar_type epsilon = 0.0000000001f);
        LF_INLINE bool    ApproxEquals(const Vector4& a, const Vector4& b, const Vector4::scalar_type epsilon = 0.0000000001f);
        LF_INLINE bool    ApproxEquals(const Vector& a, const Vector& b, const Vector::scalar_type epsilon = 0.0000000001f);
    }


}

#include "MathCombinedInline.h"

#endif // LF_CORE_MATH_COMBINED_H

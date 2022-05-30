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
    Vector2 Abs(const Vector2& vec)
    {
        return Vector2(std::fabsf(vec.x), std::fabsf(vec.y));
    }
    Vector3 Abs(const Vector3& vec)
    {
        return Vector3(std::fabsf(vec.x), std::fabsf(vec.y), std::fabsf(vec.z));
    }
    Vector4 Abs(const Vector4& vec)
    {
        return Vector4(std::fabsf(vec.x), std::fabsf(vec.y), std::fabsf(vec.z), std::fabsf(vec.w));
    }
    Vector  Abs(const Vector& vec)
    {
#ifdef LF_ENABLE_SSE_1
        return Vector(vector_abs(vec.mVector));
#else 
        return Vector4(std::fabsf(vec.x), std::fabsf(vec.y), std::fabsf(vec.z), std::fabsf(vec.w));
#endif
    }

    bool AllLessEqual(const Vector2& a, const Vector2& b)
    {
        return a.x <= b.x && a.y <= b.y;
    }
    bool AllLessEqual(const Vector3& a, const Vector3& b)
    {
        return a.x <= b.x && a.y <= b.y && a.z <= b.z;
    }
    bool AllLessEqual(const Vector4& a, const Vector4& b)
    {
        return a.x <= b.x && a.y <= b.y && a.z <= b.z && a.w <= b.w;
    }

    bool AllLessEqual(const Vector& a, const Vector& b)
    {
#ifdef LF_ENABLE_SSE_1
        return vector_cmp(vector_less_equal(a.mVector, b.mVector), 
            vector_set( static_cast<Float32>(0xFFFFFFFF), 
                        static_cast<Float32>(0xFFFFFFFF), 
                        static_cast<Float32>(0xFFFFFFFF), 
                        static_cast<Float32>(0xFFFFFFFF)));
#else 
        return a.x <= b.x && a.y <= b.y && a.z <= b.z && a.w <= b.w;
#endif 
    }
    bool AllGreaterEqual(const Vector2& a, const Vector2& b)
    {
        return a.x >= b.x && a.y >= b.y;
    }
    bool AllGreaterEqual(const Vector3& a, const Vector3& b)
    {
        return a.x >= b.x && a.y >= b.y && a.z >= b.z;
    }
    bool AllGreaterEqual(const Vector4& a, const Vector4& b)
    {
        return a.x >= b.x && a.y >= b.y && a.z >= b.z && a.w >= b.z;
    }
    bool AllGreaterEqual(const Vector& a, const Vector& b)
    {
#ifdef LF_ENABLE_SSE_1
        return vector_cmp(vector_greater_equal(a.mVector, b.mVector),
            vector_set(static_cast<Float32>(0xFFFFFFFF),
                static_cast<Float32>(0xFFFFFFFF),
                static_cast<Float32>(0xFFFFFFFF),
                static_cast<Float32>(0xFFFFFFFF)));
#else 
        return a.x <= b.x && a.y <= b.y && a.z <= b.z && a.w <= b.w;
#endif 
    }

    bool ApproxEquals(const Vector2& a, const Vector2& b, const Vector2::scalar_type epsilon)
    {
        return ApproxEquals(a.x, b.x, epsilon) 
            && ApproxEquals(a.y, b.y, epsilon);
    }
    bool ApproxEquals(const Vector3& a, const Vector3& b, const Vector3::scalar_type epsilon)
    {
        return ApproxEquals(a.x, b.x, epsilon)
            && ApproxEquals(a.y, b.y, epsilon)
            && ApproxEquals(a.z, b.z, epsilon);
    }
    bool ApproxEquals(const Vector4& a, const Vector4& b, const Vector4::scalar_type epsilon)
    {
        return ApproxEquals(a.x, b.x, epsilon)
            && ApproxEquals(a.y, b.y, epsilon)
            && ApproxEquals(a.z, b.z, epsilon)
            && ApproxEquals(a.w, a.w, epsilon);
    }
    bool ApproxEquals(const Vector& a, const Vector& b, const Vector::scalar_type epsilon)
    {
#ifdef LF_ENABLE_SSE_1
        // Abs(a-b) <= epsilon
        return AllLessEqual(Abs(a - b), Vector(epsilon));
#endif
    }
} // namespace lf

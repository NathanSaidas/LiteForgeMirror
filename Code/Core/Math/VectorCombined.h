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
#ifndef LF_CORE_VECTOR_COMBINED_H
#define LF_CORE_VECTOR_COMBINED_H

#include "Core/Math/Vector.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Vector3.h"

namespace lf {
// Matrix > Quaternion & Vector
    
     
// Vector
// {
//    global:
//      vector Abs(vector a); 
//      bool ApproxEquals(vector a);
//      bool AllLessEqual(vector a);
//      bool AllGreaterEqual(vector a);
//
//    static:
//      scalar Angle(vector a, vector b)
//      scalar Dot(vector a, vector b)
//      vector Cross(vector a, vector b)
//      vector Lerp(vector a, vector b, scalar t)
//      vector Slerp(vector a, vector b, scalar t)
//      vector Reflect(vector direction, vector normal)
//      vector Refract(vector direction, vector normal)
//      vector FaceForward(vector direction, vector normal, vector normalRef)
//      vector Project(vector a, vector b)
//      scalar Distance(vector a, vector b)
//      scalar DistanceSqr(vector a, vector b)
//      vector RotateX(vector v, scalar angle)
//      vector RotateY(vector v, scalar angle)
//      vector RotateZ(vector v, scalar angle)
//      vector ClampMagnitude(vector v, scalar length)
//    instance:
//      scalar Magnitude()
//      scalar SqrMagnitude()
//      void   Normalize()
//      vector Normalized()
//      void   Splat(scalar value)
//      scalar GetX()
//      scalar GetY()
//      scalar GetZ()
//      scalar GetW()
//      void   SetX(scalar v)
//      void   SetY(scalar v)
//      void   SetZ(scalar v)
//      void   SetW(scalar v)
//      
//    operators:
//      vector operator=(vector other)
//      vector operator+=(vector other)
//      vector operator-=(vector other)
//      vector operator*=(vector other)
//      vector operator*=(scalar other)
//      vector operator/=(vector other)
//      vector operator/=(scalar other)
//      vector operator+(vector other)
//      vector operator-(vector other)
//      vector operator*(vector other)
//      vector operator*(scalar other)
//      vector operator/(vector other)
//      vector operator/(scalar other)
//      bool operator==(vector other)
//          
// }
//
// Quaternion
// {
//    global:
//      quaternion Abs(quaternion)
//      bool ApproxEquals(vector a)
//      bool AllLessEqual(vector a)
//      bool AllGreaterEqual(vector a)
//      euler CastEuler(quaternion q)
//      matrix CastMatrix(quaternion q)
//      
//    static:
//      scalar Dot(quaternion a, quaternion b)
//      quaternion Cross(quaternion a, quaternion b)
//      quaternion Lerp(quaternion a, quaternion b, scalar t)
//      quaternion Slerp(quaternion a, quaternion b, scalar t)
//      quaternion LookRotation(vector forward, vector up)
//      quaternion RotateTowards(quaternion from, quaternion to, scalar degrees)
//      quaternion RotationBetween(vector a, vector b)
//      quaternion AngleAxis(scalar_type degrees, vector axis)
//    instance:
//      quaternion Conjugate()
//      quaternion Inverse()
//      scalar Magnitude()
//      scalar SqrMagnitude()
//      void   Normalize()
//      vector Normalized()
//      void   Splat(scalar value)
//      
//    operators:
//      quaternion operator=(quaternion other)
//      quaternion operator*=(quaternion other)
//      quaternion operator*(quaternion other)
//      vector operator*(vector other)
//      bool operator==(quaternion other)
// 
// }
//
// Matrix
// {
//    global:
//      euler CastEuler(matrix m)
//      quaternion CastQuaternion(matrix m)
//    static:
//      matrix LookAt(vector from, vector to, vector up)
//      matrix Perspective(scalar fov, scalar aspect, scalar near, scalar far)
//      matrix Ortho(scalar left, scalar right, scalar bottom, scalar top, scalar near, scalar far)
//      matrix TRS(vector, quaterion, vector)
//      matrix TRS(vector, euler, vector)
//      scalar Determinant(matrix m)
//    instance:
//      vector TransformPoint(vector p)
//      vector TransformVector(vector v)
//      SetRow 
//      GetRow
//      GetColumn
//      void Translate(scalar x, scalar y, scalar z)
//      void Transpose()
//      void Inverse()
// 
//    operators:
//     
// }
//
// AABounds -- Axis Aligned Bounding Box
// {
//    instance:
//      vector ClosestPoint(vector p)
//      bool   Contains(vector p)
//      bool   Intersect(line l, out vector point)
//      bool   Intersect(bounds b, out vector point)
//      bool   IsDegenerate()
//      void   Insert(vector p)
//      void   Expand(scalar v)
//      vector GetMin()
//      vector GetMax()
//      vector GetCenter()
//      scalar Distance(vector p)
// }
// OBounds -- Oriented Bounding Box
// {
//    instance:
//      vector ClosestPoint(vector p)
//      bool   Contains(vector p)
//      bool   Intersect(line l, out vector point)
//      bool   Intersect(bounds b, out vector point)
//      bool   IsDegenerate()
//      void   Insert(vector p)
//      void   Expand(scalar v)
//      vector GetMin()
//      vector GetMax()
//      vector GetCenter()
//      vector GetForward()
//      
//      scalar Distance(vector p)
// }
// 
// Plane 
// {
//    instance:
// }
// Line
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
// 
//
LF_INLINE Vector3 ToVector3(const Vector& v)
{
    Float32 splat[4];
    v.GetAll(splat);
    return Vector3(splat[0], splat[1], splat[2]);
}

LF_INLINE Vector4 ToVector4(const Vector& v)
{
    Float32 splat[4];
    v.GetAll(splat);
    return Vector4(splat[0], splat[1], splat[2], splat[3]);
}
} // namespace lf

#endif
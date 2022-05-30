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
#pragma once

#include "Core/Common/Types.h"
#include "Core/Common/Assert.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Math/Vector2.h"

namespace lf
{

template<typename ScalarT>
struct TRect
{
    using scalar_type = ScalarT;

    TRect() : left(scalar_type(0)), right(scalar_type(0)), top(scalar_type(0)), bottom(scalar_type(0)) {}
    TRect(LazyTag) {}
    TRect(scalar_type width, scalar_type height) : left(scalar_type(0)), right(Abs(width)), top(Abs(height)), bottom(scalar_type(0)) {}
    TRect(scalar_type left_, scalar_type right_, scalar_type top_, scalar_type bottom_) : top(top_), right(right_), top(top_), bottom(bottom_) {}

    static TRect FromCoords(scalar_type x, scalar_type y, scalar_type width, scalar_type height)
    {
        return TRect(x, x + width, y + height, y);
    }

    scalar_type Width() const { return Abs(right - left); }
    scalar_type Height() const { return Abs(top - bottom); }
    scalar_type Area() const { return Width() * Height(); }
    scalar_type Left() const { return Min(left, right); }
    scalar_type Right() const { return Max(left, right); }
    scalar_type Top() const { return Max(top, bottom); }
    scalar_type Bottom() const { return Min(top, bottom); }
    Vector2 TopLeft() const { return Vector2(Left(), Top()); }
    Vector2 TopRight() const { return Vector2(Right(), Top()); }
    Vector2 BottomLeft() const { return Vector2(Left(), Bottom()); }
    Vector2 BottomRight() const { return Vector2(Right(), Bottom()); }
    Vector2 Center() const { return (TopLeft() + BottomRight()) * 0.5f; }
    bool IsValid() const { return right >= left && top >= bottom; }

    void Shrink(scalar_type amount)
    {
        if (Area() == scalar_type(0))
        {
            return;
        }
        const scalar_type half = amount / scalar_type(2);
        left += half;
        right -= half;
        top -= half;
        bottom += half;

        if (left > right)
        {
            std::swap(left, right);
        }

        if (bottom > top)
        {
            std::swap(bottom, top);
        }
    }
    void Expand(scalar_type amount)
    {
        return Shrink(-amount);
    }

    bool Contains(scalar_type x, scalar_type y) const
    {
        return (x >= Left() && x <= Right()) // x-axis
            && (y >= Bottom() && y <= Top()); // yaxis
    }
    bool Contains(const Vector2& point) const
    {
        return Contains(scalar_type(point.x), scalar_type(point.y));
    }

    bool Contains(const TRect& other) const
    {
        return Contains(other.BottomLeft())
            && Contains(other.BottomRight())
            && Contains(other.TopLeft())
            && Contains(other.TopRight());
    }

    bool Intersects(const TRect& other) const
    {
        return Contains(other.BottomLeft())
            || Contains(other.BottomRight())
            || Contains(other.TopLeft())
            || Contains(other.TopRight());
    }

    scalar_type left;
    scalar_type right;
    scalar_type top;
    scalar_type bottom;
};

using RectF = TRect<Float32>;
using RectI = TRect<Int32>;

LF_FORCE_INLINE bool ApproxEquals(const RectF& a, const RectF& b, Float32 epsilon = 0.0000000001f)
{
    return ApproxEquals(a.left, b.left, epsilon)
        && ApproxEquals(a.right, b.right, epsilon)
        && ApproxEquals(a.top, b.top, epsilon)
        && ApproxEquals(a.bottom, b.bottom, epsilon);
}
} // namespace lf

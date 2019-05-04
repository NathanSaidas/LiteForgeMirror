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
#ifndef LF_CORE_RECT_H
#define LF_CORE_RECT_H

#include "Core/Common/Types.h"
#include "Core/Math/Vector2.h"

namespace lf
{
    class Rect
	{
	public:
        LF_FORCE_INLINE Rect() : x(0.0f), y(0.0f), width(0.0f), height(0.0f) {}
        LF_FORCE_INLINE Rect(Float32 _x, Float32 _y, Float32 _width, Float32 _height) :
            x(_x), y(_y), width(_width), height(_height) 
        {}

        /** @return - Returns true if the point is in bounds. 
        * Box:
        *
        * y + height
        *    |
        *    |
        *    y
               x------ x + width
        */
        bool LF_FORCE_INLINE PointInBounds(Float32 _x, Float32 _y) const
        {
            return(_x >= x && _x <= x + width && _y >= y && _y <= y + height);
        }

        void Shrink(Float32 amount)
        {
            const Float32 half = amount * 0.5f;
            x += half;
            width -= half;
            y += half;
            height -= half;
        }

        Vector2 GetCenter() const
        {
            return Vector2(x + width * 0.5f, y + height *0.5f);
        }

        float GetArea() const
        {
            return width * height;
        }

		Float32 x;
		Float32 y;
		Float32 width;
		Float32 height;
	};

    namespace math
    {
        LF_FORCE_INLINE bool ApproxEquals(const Rect& a, const Rect& b, Float32 epsilon = 0.0000000001f)
        {
            return ApproxEquals(a.x, b.x, epsilon)
                && ApproxEquals(a.y, b.y, epsilon)
                && ApproxEquals(a.width, b.width, epsilon)
                && ApproxEquals(a.height, b.height, epsilon);
        }
    }
}

#endif // LF_CORE_RECT_H
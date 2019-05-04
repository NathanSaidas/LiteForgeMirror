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
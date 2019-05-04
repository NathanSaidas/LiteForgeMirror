#ifndef LF_CORE_COLOR_H
#define LF_CORE_COLOR_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Math/MathFunctions.h"

namespace lf
{
    class LF_CORE_API Color
	{
	public:
		Color() : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
		Color(Float32 red, Float32 green, Float32 blue, Float32 alpha) : r(red), g(green), b(blue), a(alpha) {}
        Color(UInt32 red, UInt32 green, UInt32 blue) : 
            r (static_cast<float>(red) / 255.0f), 
            g(static_cast<float>(green) / 255.0f),
            b(static_cast<float>(blue) / 255.0f),
            a(1.0f)
        {}
		~Color() {}

		// METHODS

		Color operator+(const Color& v) const
		{
			return Color(r + v.r, g + v.g, b + v.b, a + v.b);
		}

	    Color operator-(const Color& v) const
		{
			return Color(r - v.r, g - v.g, b - v.b, a - v.b);
		}

		Color operator*(Float32 scalar) const
		{
			return Color(r*scalar, g*scalar, b*scalar,a * scalar);
		}
        bool operator==(const Color& other) const
        {
            return r == other.r && g == other.g && b == other.b && a == other.a;
        }
        bool operator!=(const Color& other) const
        {
            return r != other.r || g != other.g || b != other.b || a != other.a;
        }

		// FUNCTIONS

		static Color Lerp(Color a, Color b, float t)
		{
			return a + (b - a) * t;
		}

		Float32 r;
		Float32 g;
		Float32 b;
		Float32 a;

        static const Color White;
        static const Color DodgerBlue;
        static const Color DeepPink;
        static const Color LightSkyBlue;
        static const Color LightSkyGray;
        // new 
        static const Color AbsoluteZero;
        static const Color AcidGreen;
        static const Color Aero;
        static const Color Crimson;
        static const Color Amber;
        static const Color ArmyGreen;
        static const Color Azure;
        static const Color Bone;

        static const Color Gray;
        static const Color DeepGray;
        static const Color GrayShadow;
	};

    LF_FORCE_INLINE bool ApproxEquals(const Color& a, const Color& b, Float32 epsilon = 0.0000000001f)
    {
        return ApproxEquals(a.r, b.r, epsilon)
            && ApproxEquals(a.g, b.g, epsilon)
            && ApproxEquals(a.b, b.b, epsilon)
            && ApproxEquals(a.a, b.a, epsilon);
    }

}

#endif // LF_CORE_COLOR_H
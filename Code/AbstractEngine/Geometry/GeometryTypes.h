// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Core/Math/Color.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector2.h"
#include "Core/Utility/StdVector.h"

namespace lf {
namespace Geometry
{
    using PositionT = Vector4;

    using ColorT = Color;
    using ColorQuantizedT = Color;

    using NormalT = Vector3;
    using NormalQuantizedT = Vector3;

    using TexCoordT = Vector2;
    using TexCoordQuantizedT = Vector2;

    enum VertexType
    {
        VT_POSITION,         // PositionT position
        VT_POSITION_COLOR,   // PositionT position, ColorT color
        VT_POSITION_NORMAL,  // PositionT position, NormalT normal
        VT_BASIC,            // PositionT position, NormalT normal, TexCoordT texCoord
        VT_FULL
    };

    struct FullVertexData
    {
        TVector<PositionT> mPositions;
        TVector<ColorT>    mColors;
        TVector<NormalT>   mNormals;
        TVector<TexCoordT> mTexCoords;
    };

    enum { CUBE_VERTEX_COUNT = 36 };
    enum { CUBE_INDEX_COUNT = 36 };
    LF_ABSTRACT_ENGINE_API void CreateCube(const Vector3& size, const Color& color, FullVertexData& outData, TVector<UInt16>& outIndicies, VertexType type, bool writeIndices);

    enum { PLANE_VERTEX_COUNT = 6 };
    enum { PLANE_INDEX_COUNT = 6 };
    LF_ABSTRACT_ENGINE_API void CreatePlane(const Vector3& size, const Color& color, FullVertexData& outData, TVector<UInt16>& outIndicies, VertexType type, bool writeIndices);

    enum { LINE_VERTEX_COUNT = 2};
    enum { LINE_INDEX_COUNT = 2};

    constexpr SizeT GridVertexCount(const SizeT numSegments) { return (numSegments + 1) * 4; }
    constexpr SizeT GridIndexCount(const SizeT numSegments) { return (numSegments + 1) * 4; }

    constexpr SizeT SphereVertexCount(const SizeT numRings, const SizeT numSectors) { return numRings * numSectors; }
    constexpr SizeT SphereIndexCount(const SizeT numRings, const SizeT numSectors) { return numRings * numSectors * 6; }

}
} // namespace lf

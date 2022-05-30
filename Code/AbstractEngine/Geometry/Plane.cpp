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
#include "AbstractEngine/PCH.h"
#include "GeometryTypes.h"
namespace lf {
void Geometry::CreatePlane(const Vector3& size, const Color& color, FullVertexData& outData, TVector<UInt16>& outIndicies, VertexType type, bool writeIndices)
{
    TVector<PositionT>& positions = outData.mPositions;
    TVector<ColorT>& colors = outData.mColors;
    TVector<NormalT>& normals = outData.mNormals;
    TVector<TexCoordT>& texCoords = outData.mTexCoords;

    // clear and resize
    positions.resize(0);
    positions.resize(6);

    const bool useNormals = type == VT_FULL || type == VT_BASIC || VT_POSITION_NORMAL;
    const bool useColors = type == VT_FULL || VT_POSITION_COLOR;
    const bool useTexCoords = type == VT_FULL || type == VT_BASIC;

    if (useNormals)
    {
        normals.resize(0);
        normals.resize(6);
    }

    if (useTexCoords)
    {
        texCoords.resize(0);
        texCoords.resize(6);
    }

    if (useColors)
    {
        colors.resize(0);
        colors.resize(6);
    }

    if (writeIndices)
    {
        outIndicies.resize(0);
        outIndicies.resize(6);
    }

    // define some constants:
    const Float32 halfWidth = size.x * 0.5f;
    const Float32 halfHeight = size.y * 0.5f;

    const SizeT F_TOP_LEFT = 0;
    const SizeT F_TOP_RIGHT = 1;
    const SizeT F_BOTTOM_RIGHT = 2;
    const SizeT F_BOTTOM_LEFT = 3;

    const Vector4 vertexPositions[4] =
    {
        //Front Face
        Vector4(-halfWidth,  halfHeight, 0.0f), //TopLeft
        Vector4(halfWidth,  halfHeight, 0.0f), //TopRight
        Vector4(halfWidth, -halfHeight, 0.0f), //Bottom Right
        Vector4(-halfWidth, -halfHeight, 0.0f), //Bottom Left
    };

    const Vector2 TEX_TOP_LEFT(0.0f, 0.0f);
    const Vector2 TEX_TOP_RIGHT(1.0f, 0.0f);
    const Vector2 TEX_BOTTOM_LEFT(0.0f, 1.0f);
    const Vector2 TEX_BOTTOM_RIGHT(1.0f, 1.0f);

    //Front
    positions[0] = vertexPositions[F_TOP_LEFT];
    positions[1] = vertexPositions[F_BOTTOM_RIGHT];
    positions[2] = vertexPositions[F_BOTTOM_LEFT];
    positions[3] = vertexPositions[F_TOP_LEFT];
    positions[4] = vertexPositions[F_TOP_RIGHT];
    positions[5] = vertexPositions[F_BOTTOM_RIGHT];


    if (useNormals)
    {
        //Front
        normals[0] = normals[1] = normals[2] = normals[3] = normals[4] = normals[5] = Vector3::Forward * -1.0f;
    }

    if (writeIndices && useColors)
    {
        for (SizeT i = 0; i < 6; ++i)
        {
            colors[i] = color;
            outIndicies[i] = static_cast<UInt16>(i);
        }
    }
    else if (writeIndices)
    {
        for (SizeT i = 0; i < 6; ++i)
        {
            outIndicies[i] = static_cast<UInt16>(i);
        }
    }
    else if (useColors)
    {
        for (SizeT i = 0; i < 6; ++i)
        {
            colors[i] = color;
        }
    }

    if (useTexCoords)
    {
        for (SizeT i = 0; i < 6; i += 6)
        {
            texCoords[i + 0] = TEX_TOP_LEFT;
            texCoords[i + 1] = TEX_BOTTOM_RIGHT;
            texCoords[i + 2] = TEX_BOTTOM_LEFT;
            texCoords[i + 3] = TEX_TOP_LEFT;
            texCoords[i + 4] = TEX_TOP_RIGHT;
            texCoords[i + 5] = TEX_BOTTOM_RIGHT;
        }
    }
}
}
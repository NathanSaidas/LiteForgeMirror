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


void Geometry::CreateCube(const Vector3& size, const Color& color, FullVertexData& outData, TVector<UInt16>& outIndicies, VertexType type, bool writeIndices)
{
    TVector<PositionT>& positions = outData.mPositions;
    TVector<ColorT>& colors = outData.mColors;
    TVector<NormalT>& normals = outData.mNormals;
    TVector<TexCoordT>& texCoords = outData.mTexCoords;

    // clear and resize
    positions.resize(0);
    positions.resize(36);

    const bool useNormals = type == VT_FULL || type == VT_BASIC || VT_POSITION_NORMAL;
    const bool useColors = type == VT_FULL || VT_POSITION_COLOR;
    const bool useTexCoords = type == VT_FULL || type == VT_BASIC;

    if (useNormals)
    {
        normals.resize(0);
        normals.resize(36);
    }

    if (useTexCoords)
    {
        texCoords.resize(0);
        texCoords.resize(36);
    }

    if (useColors)
    {
        colors.resize(0);
        colors.resize(36);
    }

    if (writeIndices)
    {
        outIndicies.resize(0);
        outIndicies.resize(36);
    }


    // define some constants:
    const Float32 halfWidth = size.x * 0.5f;
    const Float32 halfHeight = size.y * 0.5f;
    const Float32 halfDepth = size.z * 0.5f;

    const SizeT F_TOP_LEFT = 0;
    const SizeT F_TOP_RIGHT = 1;
    const SizeT F_BOTTOM_RIGHT = 2;
    const SizeT F_BOTTOM_LEFT = 3;
    const SizeT B_TOP_LEFT = 4;
    const SizeT B_TOP_RIGHT = 5;
    const SizeT B_BOTTOM_RIGHT = 6;
    const SizeT B_BOTTOM_LEFT = 7;

    const Vector4 vertexPositions[8] =
    {
        //Front Face
        Vector4(-halfWidth,  halfHeight, -halfDepth), //TopLeft
        Vector4(halfWidth,  halfHeight, -halfDepth), //TopRight
        Vector4(halfWidth, -halfHeight, -halfDepth), //Bottom Right
        Vector4(-halfWidth, -halfHeight, -halfDepth), //Bottom Left

        //Back Face
        Vector4(halfWidth,  halfHeight,  halfDepth), //TopLeft
        Vector4(-halfWidth,  halfHeight,  halfDepth), //TopRight
        Vector4(-halfWidth, -halfHeight,  halfDepth), //Bottom Right
        Vector4(halfWidth, -halfHeight,  halfDepth)  //Bottom Left
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

    //Back
    positions[6] = vertexPositions[B_TOP_LEFT];
    positions[7] = vertexPositions[B_BOTTOM_RIGHT];
    positions[8] = vertexPositions[B_BOTTOM_LEFT];
    positions[9] = vertexPositions[B_TOP_LEFT];
    positions[10] = vertexPositions[B_TOP_RIGHT];
    positions[11] = vertexPositions[B_BOTTOM_RIGHT];

    //Left
    positions[12] = vertexPositions[B_TOP_RIGHT];
    positions[13] = vertexPositions[F_BOTTOM_LEFT];
    positions[14] = vertexPositions[B_BOTTOM_RIGHT];
    positions[15] = vertexPositions[B_TOP_RIGHT];
    positions[16] = vertexPositions[F_TOP_LEFT];
    positions[17] = vertexPositions[F_BOTTOM_LEFT];

    //Right
    positions[18] = vertexPositions[F_TOP_RIGHT];
    positions[19] = vertexPositions[B_BOTTOM_LEFT];
    positions[20] = vertexPositions[F_BOTTOM_RIGHT];
    positions[21] = vertexPositions[F_TOP_RIGHT];
    positions[22] = vertexPositions[B_TOP_LEFT];
    positions[23] = vertexPositions[B_BOTTOM_LEFT];

    //Top
    positions[24] = vertexPositions[B_TOP_RIGHT];
    positions[25] = vertexPositions[F_TOP_RIGHT];
    positions[26] = vertexPositions[F_TOP_LEFT];
    positions[27] = vertexPositions[B_TOP_RIGHT];
    positions[28] = vertexPositions[B_TOP_LEFT];
    positions[29] = vertexPositions[F_TOP_RIGHT];

    //Bottom
    positions[30] = vertexPositions[B_BOTTOM_LEFT];
    positions[31] = vertexPositions[F_BOTTOM_LEFT];
    positions[32] = vertexPositions[F_BOTTOM_RIGHT];
    positions[33] = vertexPositions[B_BOTTOM_LEFT];
    positions[34] = vertexPositions[B_BOTTOM_RIGHT];
    positions[35] = vertexPositions[F_BOTTOM_LEFT];

    if (useNormals)
    {
        //Front
        normals[0] = normals[1] = normals[2] = normals[3] = normals[4] = normals[5] = Vector3::Forward * -1.0f;
        //Back
        normals[6] = normals[7] = normals[8] = normals[9] = normals[10] = normals[11] = Vector3::Forward;
        //Left
        normals[12] = normals[13] = normals[14] = normals[15] = normals[16] = normals[17] = Vector3::Right * -1.0f;
        //Rigth
        normals[18] = normals[19] = normals[20] = normals[21] = normals[22] = normals[23] = Vector3::Right;
        //Top
        normals[24] = normals[25] = normals[26] = normals[27] = normals[28] = normals[29] = Vector3::Up;
        //Bottom
        normals[30] = normals[31] = normals[32] = normals[33] = normals[34] = normals[35] = Vector3::Up * -1.0f;
    }

    if (writeIndices && useColors)
    {
        for (SizeT i = 0; i < 36; ++i)
        {
            colors[i] = color;
            outIndicies[i] = static_cast<UInt16>(i);
        }
    }
    else if (writeIndices)
    {
        for (SizeT i = 0; i < 36; ++i)
        {
            outIndicies[i] = static_cast<UInt16>(i);
        }
    }
    else if (useColors)
    {
        for (SizeT i = 0; i < 36; ++i)
        {
            colors[i] = color;
        }
    }

    if (useTexCoords)
    {
        for (SizeT i = 0; i < 36; i += 6)
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
} // namespace lf
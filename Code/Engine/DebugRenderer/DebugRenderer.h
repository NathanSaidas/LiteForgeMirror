#pragma once
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
#include "Core/Common/API.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Color.h"
#include "Core/Math/Quaternion.h"
#include "Core/Math/Matrix.h"
#include "Core/Memory/SmartPointer.h"
#include "AbstractEngine/Gfx/GfxRenderer.h"

namespace lf
{
DECLARE_PTR(GfxDevice);

class LF_ENGINE_API DebugRenderer
{
public:
    virtual ~DebugRenderer() { }

    static DebugRenderer* Create(const GfxDevicePtr& device);

    virtual void DrawBounds(const Vector& center, const Vector& size, const Color& color, Float32 persistence) = 0;
    // virtual void DrawPlane(const Vector& center, const Vector& size, const Quaternion& rotation, const Color& color, Float32 persistence) = 0;
    // virtual void DrawSphere(const Vector& center, Float32 radius, const Color& color, Float32 persistence) = 0;
    // virtual void DrawLine(const Vector& start, const Vector& end, const Color& color, Float32 persistence) = 0;
    // virtual void DrawWireBounds(const Vector& center, const Vector& size, const Color& color, Float32 persistence) = 0;
    // virtual void DrawWirePlane(const Vector& center, const Vector& size, const Quaternion& rotation, const Color& color, Float32 persistence) = 0;
    // virtual void DrawWireSphere(const Vector& center, Float32 radius, const Color& color, Float32 persistence) = 0;
    // virtual void DrawWireGrid(const Vector& center, Float32 size, SizeT numSegments, const Color& color, Float32 persistence) = 0;
    // 
    virtual void Set3DProjection(const Matrix& projection) = 0;
    virtual void Set3DView(const Matrix& view) = 0;
private:
    
};

} // namespace lf
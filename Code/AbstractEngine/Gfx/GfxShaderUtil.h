// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "AbstractEngine/Gfx/GfxTypes.h"

namespace lf {
class AssetPath;

namespace Gfx {

    // ** Compute the shader hash such that
    // Hash(path) + HashArray(defines) + Hash(shaderType.AsString())
    LF_ABSTRACT_ENGINE_API ShaderHash ComputeHash(ShaderType shaderType, const AssetPath& path, const TVector<Token>& defines);
    // ** Compute the path of a shader such that
    // Path + _ + Hash + _ + ShaderType + _ + API
    // eg; Engine//Test/Shaders/ExampleShader_0x88838239_vertex_DX11
    LF_ABSTRACT_ENGINE_API String ComputePath(ShaderType shaderType, GraphicsApi api, const AssetPath& path, ShaderHash hash);

} // namespace Gfx
} // namespace lf
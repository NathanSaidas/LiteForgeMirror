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

#include "AbstractEngine/PCH.h"
#include "GfxShaderUtil.h"
#include "Runtime/Asset/AssetPath.h"

namespace lf {

Gfx::ShaderHash Gfx::ComputeHash(Gfx::ShaderType shaderType, const AssetPath& path, const TVector<Token>& defines)
{
    SizeT hashSize = 0;
    Gfx::ShaderHash hash = FNV::HashStreamedString(0, path.CStr(), hashSize);
    for (const Token& define : defines)
    {
        hash = FNV::HashStreamedString(hash, define.CStr(), hashSize);
    }
    hash = FNV::HashStreamedString(hash, Gfx::TShaderType::GetString(shaderType), hashSize);
    return hash;
}

String Gfx::ComputePath(Gfx::ShaderType shaderType, Gfx::GraphicsApi api, const AssetPath& path, Gfx::ShaderHash hash)
{
    const char* SHADER_EXTENSION[] =
    {
        "vs",
        "ps"
    };
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(SHADER_EXTENSION) == ENUM_SIZE(Gfx::ShaderType));
    const char* API[] =
    {
        "Generic",
        "DX11",
        "Dx12"
    };
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(API) == ENUM_SIZE(Gfx::GraphicsApi));

    String newPath(path.CStr());

    SizeT ext = newPath.Find(".");
    if (Valid(ext))
    {
        newPath = newPath.SubString(0, ext);
    }

    newPath += "_";
    newPath += ToHexString(hash);
    newPath += "_";
    newPath += SHADER_EXTENSION[EnumValue(shaderType)];
    newPath += "_";
    newPath += API[EnumValue(api)];
    return newPath;
}

} // namespace lf
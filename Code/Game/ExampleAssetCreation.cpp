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
#include "Core/Test/Test.h"
#include "Runtime/Asset/AssetMgr.h"
#include "AbstractEngine/Gfx/GfxVertexInputFormat.h"


namespace lf {

// This will create a vertex format
REGISTER_TEST(CreateAsset_GfxVertexFormatDesc_Test, "Graphics")
{
    AssetMgr& assetMgr = GetAssetMgr();
    
    auto testPath = AssetPath("engine//testing/example_assets/GfxVertexInputFormat");

    auto vertexFormat = MakeConvertibleAtomicPtr<GfxVertexInputFormat>();
    vertexFormat->Append(Gfx::ShaderAttribFormat::SAF_VECTOR4, Token("SV_VERTEX"), Token("Position"), 0);
    vertexFormat->Append(Gfx::ShaderAttribFormat::SAF_VECTOR4, Token("COLOR"), Token("Color"), 1);
    vertexFormat->Append(Gfx::ShaderAttribFormat::SAF_VECTOR2, Token("TEXCOORD"), Token("TexCoord"), 2);

    auto op = assetMgr.Create(testPath, vertexFormat, nullptr);
    TEST(assetMgr.Wait(op));
}

}
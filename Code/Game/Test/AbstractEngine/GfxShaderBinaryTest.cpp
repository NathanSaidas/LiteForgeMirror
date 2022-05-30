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
#include "Core/Memory/MemoryBuffer.h"
#include "Runtime/Asset/AssetOp.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "Runtime/Async/PromiseImpl.h"
#include "AbstractEngine/Gfx/GfxShader.h"
#include "AbstractEngine/Gfx/GfxShaderText.h"
#include "AbstractEngine/Gfx/GfxShaderBinary.h"
#include "AbstractEngine/Gfx/GfxShaderUtil.h"
#include "AbstractEngine/Gfx/GfxMaterial.h"

namespace lf {

DECLARE_ASSET(GfxShader);
DECLARE_ASSET(GfxShaderBinaryInfo);
DECLARE_ASSET(GfxShaderBinaryData);

using ShaderOpSuccessCallback = TCallback<void, Gfx::ResourcePtr>;
using ShaderOpFailedCallback = TCallback<void, String>;
using ShaderOpPromise = PromiseImpl<ShaderOpSuccessCallback, ShaderOpFailedCallback>;

static const char* TEST_SHADER_CODE = ""
    "float4 VSMain() {"
    "   return float4(1,1,1,1);"
    "}";
static const char* TEST_SHADER_PATH = "Engine//Test/AbstractEngine/TestShaderBinary";
static const ByteT TEST_SHADER_BYTE_CODE[] = {
 34, 179, 112, 233, 110,  15, 156, 165, 122,  43, 136,  33,  70,   7,  52,  93, 210, 163, 160,  89,  30, 255, 204,  21,  42,  27,
184, 145, 246, 247, 100, 205, 130, 147, 208, 201, 206, 239, 252, 133, 218,  11, 232,   1, 166, 231, 148,  61,  50, 131,   0,
 57, 126, 223,  44, 245, 138, 251,  24, 113,  86, 215, 196, 173, 226, 115,  48, 169,  46, 207,  92, 101,  58, 235,  72, 225,
  6, 199, 244,  29, 146,  99,  96,  25, 222, 191, 140, 213, 234, 219, 120,  81, 182, 183,  36, 141,  66,  83, 144, 137 };


REGISTER_TEST(GfxShaderBinaryTestSetup, "AbstractEngine.Gfx", TestFlags::TF_SETUP, 100)
{
    const AssetPath shaderPath(String(TEST_SHADER_PATH) + ".lob");
    const AssetPath shaderTextPath(String(TEST_SHADER_PATH) + ".hlsl");
    const TVector<Token> defines = { Token("RED"), Token("FORWARD"), Token("LIGHT4") };
    const Gfx::ShaderHash hash = Gfx::ComputeHash(Gfx::ShaderType::VERTEX, shaderPath, defines);
    String basePath = Gfx::ComputePath(Gfx::ShaderType::VERTEX, Gfx::GraphicsApi::DX11, shaderPath, hash);
    const AssetPath infoPath(basePath + ".shaderinfo");
    const AssetPath dataPath(basePath + ".shaderdata");

    if (auto type = GetAssetMgr().FindType(dataPath))
    {
        GetAssetMgr().Wait(GetAssetMgr().Delete(type));
    }

    if (auto type = GetAssetMgr().FindType(infoPath))
    {
        GetAssetMgr().Wait(GetAssetMgr().Delete(type));
    }

    if (auto type = GetAssetMgr().FindType(shaderTextPath))
    {
        GetAssetMgr().Wait(GetAssetMgr().Delete(type));
    }

    if (auto type = GetAssetMgr().FindType(shaderPath))
    {
        GetAssetMgr().Wait(GetAssetMgr().Delete(type));
    }

    GetAssetMgr().Wait(GetAssetMgr().SaveDomain(shaderPath.GetDomain()));
    GetAssetMgr().Wait(GetAssetMgr().SaveDomainCache(shaderPath.GetDomain()));

}

// Test to show the flow of creating/deleting shaders.
// Ensure they can be created/deleted.
REGISTER_TEST(GfxShaderBinaryCreateDeleteTest, "AbstractEngine.Gfx", TestFlags::TF_DISABLED)
{
    const AssetPath shaderPath(String(TEST_SHADER_PATH) + ".lob");
    const AssetPath shaderTextPath(String(TEST_SHADER_PATH) + ".hlsl");
    const TVector<Token> defines = { Token("RED"), Token("FORWARD"), Token("LIGHT4") };
    const Gfx::ShaderHash hash = Gfx::ComputeHash(Gfx::ShaderType::VERTEX, shaderPath, defines);

    auto shaderText = GetAssetMgr().CreateEditable<GfxShaderText>();
    shaderText->SetText(TEST_SHADER_CODE);

    TEST_CRITICAL(GetAssetMgr().Wait(GetAssetMgr().Create(shaderTextPath, shaderText, nullptr)));

    GfxShaderTextAsset shaderTextAsset(shaderTextPath, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    TEST_CRITICAL(shaderTextAsset.IsLoaded());

    auto shader = GetAssetMgr().CreateEditable<GfxShader>();
    shader->SetText(Gfx::GraphicsApi::DX11, shaderTextAsset);

    TEST_CRITICAL(GetAssetMgr().Wait(GetAssetMgr().Create(shaderPath, shader, nullptr)));

    GfxShaderAsset shaderAsset(shaderPath, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    TEST_CRITICAL(shaderAsset.IsLoaded());

    auto info = GetAssetMgr().CreateEditable<GfxShaderBinaryInfo>();
    info->SetShaderType(Gfx::ShaderType::VERTEX);
    info->SetShader(shaderAsset);
    info->SetDefines(defines);
    info->SetHash(hash);


    auto data = GetAssetMgr().CreateEditable<GfxShaderBinaryData>();
    data->SetBuffer(TEST_SHADER_BYTE_CODE, LF_ARRAY_SIZE(TEST_SHADER_BYTE_CODE));

    String basePath = Gfx::ComputePath(Gfx::ShaderType::VERTEX, Gfx::GraphicsApi::DX11, shaderPath, hash);
    const AssetPath infoPath(basePath + ".shaderinfo");
    const AssetPath dataPath(basePath + ".shaderdata");

    TEST_CRITICAL(GetAssetMgr().Wait(GetAssetMgr().Create(infoPath, info, nullptr)));
    TEST_CRITICAL(GetAssetMgr().Wait(GetAssetMgr().Create(dataPath, data, nullptr)));

    // TODO: Why does it not load?
    GfxShaderBinaryInfoAsset infoAsset(infoPath, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    TEST_CRITICAL(infoAsset.IsLoaded());
    GfxShaderBinaryDataAsset dataAsset(dataPath, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    TEST_CRITICAL(dataAsset.IsLoaded());

    // auto material = GetAssetMgr().CreateEditable<GfxMaterial>();
    // material->SetShader(Gfx::ShaderType::VERTEX, shaderAsset);
    // material->SetDefines(defines);

    // When we create the asset, Create Shader Cache data
    // When we import the asset, Create Shader Cache data
    // When we update the asset, Create Shader Cache data
    // When we load the domain,  Create Shader Cache data
    // 
    //
}

// Test the flow to load binary data.
REGISTER_TEST(GfxShaderBinaryLoadTest, "AbstractEngine.Gfx")
{
    // GetAssetMgr().Create(path, object, nullptr);
        
}

    

} // namespace lf
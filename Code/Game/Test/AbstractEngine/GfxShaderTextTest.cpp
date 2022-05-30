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
#include "Runtime/Asset/AssetMgr.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxShaderText.h"
namespace lf {

static const char* TEST_SHADER_CODE = ""
    "float4 VSMain() {"
    "   return float4(1,1,1,1);"
    "}";
static const char* TEST_SHADER_PATH = "Engine//Test/AbstractEngine/TestShader.hlsl";

REGISTER_TEST(GfxShaderTextSetupTest, "AbstractEngine.Gfx", TestFlags::TF_SETUP, 100)
{
    const String shaderCode(TEST_SHADER_CODE, COPY_ON_WRITE);
    const String shaderPath(TEST_SHADER_PATH, COPY_ON_WRITE);
    const AssetPath shaderAssetPath(shaderPath);
    auto type = GetAssetMgr().FindType(shaderAssetPath);
    if (type != nullptr)
    {
        // This did not update the type map.. maybe even the cache?
        auto op = GetAssetMgr().Delete(type);
        GetAssetMgr().Wait(op);
        TEST_CRITICAL(op->IsSuccess());
        TEST_CRITICAL(!GetAssetMgr().FindType(shaderAssetPath));

        op = GetAssetMgr().SaveDomain(shaderAssetPath.GetDomain());
        GetAssetMgr().Wait(op);
        TEST(op->IsSuccess());

        op = GetAssetMgr().SaveDomainCache(shaderAssetPath.GetDomain());
        GetAssetMgr().Wait(op);
        TEST(op->IsSuccess());
    }
}

REGISTER_TEST(GfxShaderTextAssetTest, "AbstractEngine.Gfx")
{
    const String shaderCode(TEST_SHADER_CODE, COPY_ON_WRITE);
    const String shaderPath(TEST_SHADER_PATH, COPY_ON_WRITE);
    const AssetPath shaderAssetPath(shaderPath);
    TEST_CRITICAL(GetAssetMgr().FindType(shaderAssetPath) == nullptr);

    {
        auto asset = GetAssetMgr().CreateEditable<GfxShaderText>();
        asset->SetText(shaderCode);
        TEST_CRITICAL(!asset->GetText().CopyOnWrite());

        auto op = GetAssetMgr().Create(shaderAssetPath, asset, nullptr);
        GetAssetMgr().Wait(op);
        TEST_CRITICAL(op->IsSuccess());

        TAssetType<GfxShaderText> assetType(shaderAssetPath);
        TEST(assetType != NULL_PTR);

        MemoryBuffer sourceBuffer;
        TEST(GetAssetMgr().GetSourceData(assetType.GetType(), sourceBuffer));

        MemoryBuffer cacheBuffer;
        TEST(GetAssetMgr().GetCacheData(assetType.GetType(), cacheBuffer));

        String sourceText(sourceBuffer.GetSize() - 1, reinterpret_cast<const char*>(sourceBuffer.GetData()), COPY_ON_WRITE);
        String cacheText(cacheBuffer.GetSize() - 1, reinterpret_cast<const char*>(cacheBuffer.GetData()), COPY_ON_WRITE);

        TEST(sourceText == shaderCode);
        TEST(cacheText == shaderCode);

        TAsset<GfxShaderText> assetObject(shaderAssetPath, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);

        TEST(assetObject->GetText() == shaderCode);
    }
    


}

} // namespace lf
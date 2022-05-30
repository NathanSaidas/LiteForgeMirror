
#include "GraphicsServiceBase.h"
#include "Runtime/Asset/AssetMgr.h"
#include "Runtime/Asset/AssetOp.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxShaderText.h"
#include "AbstractEngine/Gfx/GfxTextureBinary.h"
#include "Engine/DX12/DX12GfxShaderCompiler.h"

namespace lf {

DECLARE_ASSET(GfxShaderText);

DEFINE_ABSTRACT_CLASS(lf::GraphicsServiceBase) { NO_REFLECTION; }

static AssetTypeInfoCPtr GetOrImportType(const String& assetPath)
{
    const AssetPath path(assetPath);
    AssetTypeInfoCPtr type = GetAssetMgr().FindType(path);
    if (!type)
    {
        auto importOp = GetAssetMgr().Import(path);
        if (!GetAssetMgr().Wait(importOp) || importOp->IsFailed())
        {
            return AssetTypeInfoCPtr();
        }
        type = GetAssetMgr().FindType(path);
    }

    if (type)
    {
        GetAssetMgr().Wait(GetAssetMgr().UpdateCacheData(type));
    }

    return type;
}

GraphicsServiceBase::~GraphicsServiceBase()
{

}

DebugAssetProviderPtr GraphicsServiceBase::CreateDebugAssetProvider()
{
    return DebugAssetProviderPtr(LFNew<DebugAssetProviderImpl>(
        DebugAssetProviderImpl::GetShaderTextCallback::Make(this, &GraphicsServiceBase::GetShaderText),
        DebugAssetProviderImpl::GetShaderBinaryCallback::Make(this, &GraphicsServiceBase::GetShaderBinary),
        DebugAssetProviderImpl::GetTextureCallback::Make(this, &GraphicsServiceBase::GetTexture)));
}

String GraphicsServiceBase::GetShaderText(const String& assetPath) const
{
    AssetTypeInfoCPtr type = GetOrImportType(assetPath);
    GfxShaderTextAsset shader(type, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    if (!shader && !shader.IsLoaded())
    {
        return String();
    }

    return shader->GetText();
}

String GraphicsServiceBase::GetShaderAggregateText(const TVector<String>& paths)
{
    String text;
    for (const String& path : paths)
    {
        text += GetShaderText(path);
    }
    return text;
}


bool GraphicsServiceBase::GetShaderBinary(Gfx::ShaderType shaderType, const String& text, const TVector<Token>& defines, MemoryBuffer& outBuffer) const
{
    DX12GfxShaderCompiler compiler;
    return compiler.Compile(shaderType, text, defines, outBuffer);
}

GfxTextureBinaryAsset GraphicsServiceBase::GetTexture(const String& assetPath) const
{
    AssetTypeInfoCPtr type = GetOrImportType(assetPath);

    GfxTextureBinaryAsset texture(type, AssetLoadFlags::LF_RECURSIVE_PROPERTIES);
    if (!texture && !texture.IsLoaded())
    {
        return GfxTextureBinaryAsset();
    }

    return texture;
}

} // namespace lf
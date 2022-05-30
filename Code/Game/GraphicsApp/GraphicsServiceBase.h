#pragma once

#include "Runtime/Service/Service.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxRenderer.h"


namespace lf {
DECLARE_ASSET(GfxTextureBinary);

class GraphicsServiceBase : public Service
{
    DECLARE_CLASS(GraphicsServiceBase, Service);
public:
    virtual ~GraphicsServiceBase();

protected:
    class DebugAssetProviderImpl : public DebugAssetProvider
    {
    public:
        using GetShaderTextCallback = TCallback<String, const String&>;
        using GetShaderBinaryCallback = TCallback<bool, Gfx::ShaderType, const String&, const TVector<Token>&, MemoryBuffer&>;
        using GetTextureCallback = TCallback<GfxTextureBinaryAsset, const String&>;

        DebugAssetProviderImpl(
            const GetShaderTextCallback& getShaderText,
            const GetShaderBinaryCallback& getShaderBinary,
            const GetTextureCallback& getTexture
        )
            : DebugAssetProvider()
            , mGetShaderText(getShaderText)
            , mGetShaderBinary(getShaderBinary)
            , mGetTexture(getTexture)
        {}

        String GetShaderText(const String& assetName)
        {
            return mGetShaderText.Invoke(assetName);
        }
        bool GetShaderBinary(Gfx::ShaderType shaderType, const String& text, const TVector<Token>& defines, MemoryBuffer& outputBuffer)
        {
            return mGetShaderBinary.Invoke(shaderType, text, defines, outputBuffer);
        }
        GfxTextureBinaryAsset GetTexture(const String& assetName)
        {
            return mGetTexture.Invoke(assetName);
        }

        GetShaderTextCallback mGetShaderText;
        GetShaderBinaryCallback mGetShaderBinary;
        GetTextureCallback mGetTexture;
    };
    DebugAssetProviderPtr CreateDebugAssetProvider();

    String GetShaderText(const String& assetPath) const;
    String GetShaderAggregateText(const TVector<String>& paths);
    bool GetShaderBinary(Gfx::ShaderType shaderType, const String& text, const TVector<Token>& defines, MemoryBuffer& outBuffer) const;
    GfxTextureBinaryAsset GetTexture(const String& assetPath) const;
};

} // 
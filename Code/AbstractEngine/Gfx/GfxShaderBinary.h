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

#include "Core/Memory/MemoryBuffer.h"
#include "AbstractEngine/Gfx/GfxBase.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
// TODO: We might be able to move these includes to the cpp if we provide constructor/destructor...
#include "AbstractEngine/Gfx/GfxShader.h"
#include "AbstractEngine/Gfx/GfxShaderText.h"

namespace lf {

DECLARE_ASSET_TYPE(GfxShader);
DECLARE_ASSET_TYPE(GfxShaderText);
DECLARE_ASSET(GfxShaderBinaryInfo);
DECLARE_ASSET(GfxShaderBinaryData);
DECLARE_ASSET_TYPE(GfxShaderBinaryData);

namespace Gfx {
struct ShaderBinary
{
    GfxShaderBinaryInfoAsset     mInfo;
    GfxShaderBinaryDataAssetType mData;
    Gfx::ResourcePtr             mResourceHandle;
};
} // namespace Gfx

class LF_ABSTRACT_ENGINE_API GfxShaderBinaryInfo : public AssetObject
{
    DECLARE_CLASS(GfxShaderBinaryInfo, AssetObject);
public:
    void Serialize(Stream& s) override;

    Gfx::ShaderType GetShaderType() const { return mShaderType; }
    Gfx::GraphicsApi GetApi() const { return mApi;  }
    const GfxShaderTextAssetType& GetShaderText() const { return mShaderText; }
    const GfxShaderAssetType& GetShader() const { return mShader; }
    const TVector<Token>& GetDefines() const { return mDefines; }
    const Gfx::ShaderHash GetHash() const { return mHash; }

    void SetShaderType(Gfx::ShaderType value) { mShaderType = value; }
    void SetApi(Gfx::GraphicsApi value) { mApi = value; }
    void SetShaderText(const GfxShaderTextAssetType& value) { mShaderText = value; }
    void SetShader(const GfxShaderAssetType& value) { mShader = value; }
    void SetDefines(const TVector<Token>& value) { mDefines = value; }
    void SetHash(Gfx::ShaderHash value) { mHash = value; }

protected:
    void OnClone(const Object& o) override;
private:
    Gfx::TShaderType mShaderType;
    Gfx::TGraphicsApi mApi;
    GfxShaderTextAssetType mShaderText;
    GfxShaderAssetType mShader;
    TVector<Token> mDefines;
    Gfx::ShaderHash mHash;
};

class LF_ABSTRACT_ENGINE_API GfxShaderBinaryData : public AssetObject
{
    DECLARE_CLASS(GfxShaderBinaryData, AssetObject);
public:
    void Serialize(Stream& s) override;

    const MemoryBuffer& GetBuffer() const { return mBuffer; }
    void SetBuffer(const MemoryBuffer& value) { mBuffer.Copy(value); }
    void SetBuffer(MemoryBuffer&& value) { mBuffer = std::move(value); }
    void SetBuffer(const void* value, SizeT byteCount);

protected:
    void OnClone(const Object& o) override;
private:
    MemoryBuffer mBuffer;
};

class LF_ABSTRACT_ENGINE_API GfxShaderBinaryBundle
{
public:
    void Serialize(Stream& s);

    void Set(Gfx::GraphicsApi api, const GfxShaderBinaryInfoAsset& info, const GfxShaderBinaryDataAsset& data)
    {
        CriticalAssert(ValidEnum(api));

        mInfo[EnumValue(api)] = info;
        mData[EnumValue(api)] = data;
    }

    const GfxShaderBinaryInfoAsset& GetInfo(Gfx::GraphicsApi api) const
    {
        CriticalAssert(ValidEnum(api));

        return mInfo[EnumValue(api)];
    }

    const GfxShaderBinaryDataAsset& GetData(Gfx::GraphicsApi api) const
    {
        CriticalAssert(ValidEnum(api));
        
        return mData[EnumValue(api)];
    }
private:
    GfxShaderBinaryInfoAsset mInfo[ENUM_SIZE(Gfx::GraphicsApi)];
    GfxShaderBinaryDataAsset mData[ENUM_SIZE(Gfx::GraphicsApi)];
};

LF_INLINE Stream& operator<<(Stream& s, GfxShaderBinaryBundle& o)
{
    o.Serialize(s);
    return s;
}

}
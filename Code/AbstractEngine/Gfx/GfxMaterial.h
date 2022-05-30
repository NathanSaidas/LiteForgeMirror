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
#include "AbstractEngine/Gfx/GfxResourceObject.h"

namespace lf {
#if defined(LF_GFX_ENGINE_REWORK)
DECLARE_ASSET(GfxShader);
DECLARE_ASSET_TYPE(GfxShader);
class GfxMaterial;
class Matrix;

// ********************************************************************
// ********************************************************************
class LF_ABSTRACT_ENGINE_API GfxMaterialProperty
{
public:

private:
    Gfx::TShaderAttribFormat mType;

    Token mName;
    int   mTexture;    // GfxTextureAsset
    int   mTexturePtr; // GfxTexturePtr
    UInt8 mBuffer[64]; // Support size for Matrix

};

class LF_ABSTRACT_ENGINE_API GfxMaterial : public GfxObject
{
    DECLARE_CLASS(GfxMaterial, GfxObject);
public:
    using PointerConvertible = void*;
    GfxMaterial();
    virtual ~GfxMaterial();

    // Derived GfxObjects cannot define custom properties, as we could have conflicts?
    // But we could also during caching create the different types...
    // eg
    // pbr_default/dx11.lob
    // pbr_default/dx12.lob
    // pbr_default/vulcan.lob
    // pbr_default/opengl.lob
    void Serialize(Stream& s) final;
    // ********************************************************************
    // Validate shaders, Generate Text, Compile Shaders, Create PSO
    // ********************************************************************
    virtual void Commit();

    void SetDefines(const TVector<Token>& defines) { mDefines = defines; }
    const TVector<Token>& GetDefines() const { return mDefines; }

    // Blend State:
    bool                    GetBlendEnabled() const { return mBlendEnabled; }
    Gfx::BlendType          GetBlendSrc() const { return mBlendSrc; }
    Gfx::BlendType          GetBlendDest() const { return mBlendDest; }
    Gfx::BlendType          GetBlendSrcAlpha() const { return mBlendSrcAlpha; }
    Gfx::BlendType          GetBlendDestAlpha() const { return mBlendDestAlpha; }
    Gfx::BlendOp            GetBlendOp() const { return mBlendOp; }
    Gfx::BlendOp            GetBlendAlphaOp() const { return mBlendAlphaOp; }
    void                    SetBlendEnabled(bool value) { mBlendEnabled = value; }
    void                    SetBlendSrc(Gfx::BlendType value) { mBlendSrc = value; }
    void                    SetBlendDest(Gfx::BlendType value) { mBlendDest = value; }
    void                    SetBlendSrcAlpha(Gfx::BlendType value) { mBlendSrcAlpha = value; }
    void                    SetBlendDestAlpha(Gfx::BlendType value) { mBlendDestAlpha = value; }
    void                    SetBlendOp(Gfx::BlendOp value) { mBlendOp = value; }
    void                    SetBlendAlphaOp(Gfx::BlendOp value) { mBlendAlphaOp = value; }

    // Raster State:
    bool                    GetRasterWireframe() const { return mRasterWireframe; }
    bool                    GetRasterMSAA() const { return mRasterMSAA; }
    bool                    GetRasterLineAA() const { return mRasterLineAA; }
    Gfx::CullMode           GetRasterCullMode() const { return mRasterCullMode; }
    Gfx::CullFace           GetRasterCullFace() const { return mRasterCullFace; }
    void                    SetRasterWireframe(bool value) { mRasterWireframe = value; }
    void                    SetRasterMSAA(bool value) { mRasterMSAA = value; }
    void                    SetRasterLineAA(bool value) { mRasterLineAA = value; }
    void                    SetRasterCullMode(Gfx::CullMode value) { mRasterCullMode = value; }
    void                    SetRasterCullFace(Gfx::CullFace value) { mRasterCullFace = value; }

    // Depth State:
    bool                    GetDepthEnabled() const { return mDepthEnabled; }
    bool                    GetDepthWrite() const { return mDepthWrite; }
    Gfx::TDepthFunc         GetDepthFunc() const { return mDepthFunc; }
    void                    SetDepthEnabled(bool value) { mDepthEnabled = value; }
    void                    SetDepthWrite(bool value) { mDepthWrite = value; }
    void                    SetDepthFunc(Gfx::DepthFunc value) { mDepthFunc = value; }

    Gfx::TRenderMode        GetRenderMode() const { return mRenderMode; }

    // Vertex/IA
    bool                    GetVertexMultiBuffer() const { return mVertexMultiBuffer; }
    void                    SetVertexMultiBuffer(bool value) { mVertexMultiBuffer = value; }
        
    void SetShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader);
    const GfxShaderAsset& GetShader(Gfx::ShaderType shaderType) const;
    void SetBinary(Gfx::ShaderType shaderType, const GfxShaderBinaryBundle& bundle);
    const GfxShaderBinaryBundle& GetBinary(Gfx::ShaderType shaderType) const;


    bool SetProperty(const Token& name, Float32 value);
    bool SetProperty(const Token& name, Int32 value);
    bool SetProperty(const Token& name, UInt32 value);
    bool SetProperty(const Token& name, const Vector2& value);
    bool SetProperty(const Token& name, const Vector3& value);
    bool SetProperty(const Token& name, const Vector4& value);
    bool SetProperty(const Token& name, const Color& value);
    bool SetProperty(const Token& name, const Matrix& value);
    bool SetProperty(Gfx::MaterialPropertyId id, Float32 value);
    bool SetProperty(Gfx::MaterialPropertyId id, Int32 value);
    bool SetProperty(Gfx::MaterialPropertyId id, UInt32 value);
    bool SetProperty(Gfx::MaterialPropertyId id, const Vector2& value);
    bool SetProperty(Gfx::MaterialPropertyId id, const Vector3& value);
    bool SetProperty(Gfx::MaterialPropertyId id, const Vector4& value);
    bool SetProperty(Gfx::MaterialPropertyId id, const Color& value);
    bool SetProperty(Gfx::MaterialPropertyId id, const Matrix& value);


    Gfx::MaterialPropertyId FindProperty(const Token& name) const;
private:
    
    TVector<Token>           mDefines;

    // Blend State:
    bool                    mBlendEnabled;
    Gfx::TBlendType         mBlendSrc;
    Gfx::TBlendType         mBlendDest;
    Gfx::TBlendType         mBlendSrcAlpha;
    Gfx::TBlendType         mBlendDestAlpha;
    Gfx::TBlendOp           mBlendOp;
    Gfx::TBlendOp           mBlendAlphaOp;

    // Raster State:
    bool                    mRasterWireframe;
    bool                    mRasterMSAA;
    bool                    mRasterLineAA;
    Gfx::TCullMode          mRasterCullMode;
    Gfx::TCullFace          mRasterCullFace;

    // Depth State:
    bool                    mDepthEnabled;
    bool                    mDepthWrite;
    Gfx::TDepthFunc         mDepthFunc;

    Gfx::TRenderMode        mRenderMode;

    // Whether the IA uses multiple buffers or 1
    bool                    mVertexMultiBuffer;

    GfxShaderAsset          mShaders[ENUM_SIZE(Gfx::ShaderType)];
    GfxShaderBinaryBundle   mBundles[ENUM_SIZE(Gfx::ShaderType)];
};

#endif


class LF_ABSTRACT_ENGINE_API GfxMaterial : public GfxResourceObject
{
    DECLARE_CLASS(GfxMaterial, GfxResourceObject);
public:

};

} // namespace lf
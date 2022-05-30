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
#include "AbstractEngine/PCH.h"
#include "GfxMaterial.h"


namespace lf {
#if defined(LF_GFX_ENGINE_REWORK)

DEFINE_ABSTRACT_CLASS(lf::GfxMaterialAdapter) { NO_REFLECTION; }

GfxMaterialAdapter::GfxMaterialAdapter()
: Super()
{}
GfxMaterialAdapter::~GfxMaterialAdapter()
{}

GfxMaterial::GfxMaterial()
: Super()
, mDefines()
// Blend State
, mBlendEnabled(true)
, mBlendSrc(Gfx::BlendType::SRC_ALPHA)
, mBlendDest(Gfx::BlendType::ONE_MINUS_SRC_ALPHA)
, mBlendSrcAlpha(Gfx::BlendType::ZERO)
, mBlendDestAlpha(Gfx::BlendType::ZERO)
, mBlendOp(Gfx::BlendOp::ADD)
, mBlendAlphaOp(Gfx::BlendOp::ADD)
// Raster State
, mRasterWireframe(false)
, mRasterMSAA(false)
, mRasterLineAA(false)
, mRasterCullMode(Gfx::CullMode::COUNTER_CLOCK_WISE)
, mRasterCullFace(Gfx::CullFace::BACK)
// Depth State
, mDepthEnabled(false)
, mDepthWrite(false)
, mDepthFunc(Gfx::DepthFunc::LESS)
// 
, mRenderMode(Gfx::RenderMode::TRIANGLES)
// Vertex/IA
, mVertexMultiBuffer(false)
{
}
GfxMaterial::~GfxMaterial()
{

}

void GfxMaterial::Serialize(Stream& s)
{
    Super::Serialize(s);

    SERIALIZE_ARRAY(s, mDefines, "");
    // Blend State
    SERIALIZE(s, mBlendEnabled, "");
    SERIALIZE(s, mBlendSrc, "");
    SERIALIZE(s, mBlendDest, "");
    SERIALIZE(s, mBlendSrcAlpha, "");
    SERIALIZE(s, mBlendDestAlpha, "");
    SERIALIZE(s, mBlendOp , "");
    SERIALIZE(s, mBlendAlphaOp, "");

    // Raster State
    SERIALIZE(s, mRasterWireframe, "");
    SERIALIZE(s, mRasterMSAA, "");
    SERIALIZE(s, mRasterLineAA, "");
    SERIALIZE(s, mRasterCullMode, "");
    SERIALIZE(s, mRasterCullFace, "");

    // Depth State
    SERIALIZE(s, mDepthEnabled, "");
    SERIALIZE(s, mDepthWrite, "");
    SERIALIZE(s, mDepthFunc, "");

    SERIALIZE(s, mRenderMode, "");

    // Vertex
    SERIALIZE(s, mVertexMultiBuffer, "");

    const char* SHADER_STRINGS[] = { "VertexShader", "PixelShader" };
    const char* SHADER_BUNDLE_STRINGS[] = { "VertexShaderBundle", "PixelShaderBundle" };
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(SHADER_STRINGS) == ENUM_SIZE(Gfx::ShaderType));
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(SHADER_BUNDLE_STRINGS) == ENUM_SIZE(Gfx::ShaderType));
    for (SizeT i = 0; i < ENUM_SIZE(Gfx::ShaderType); ++i)
    {
        SERIALIZE_NAMED(s, SHADER_STRINGS[i], mShaders[i], "");
        SERIALIZE_STRUCT_NAMED(s, SHADER_BUNDLE_STRINGS[i], mBundles[i], "");
    }
}

void GfxMaterial::Commit()
{

}

void GfxMaterial::SetShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader)
{
    CriticalAssert(ValidEnum(shaderType));
    mShaders[EnumValue(shaderType)] = shader;
}
const GfxShaderAsset& GfxMaterial::GetShader(Gfx::ShaderType shaderType) const
{
    CriticalAssert(ValidEnum(shaderType));
    return mShaders[EnumValue(shaderType)];
}

void GfxMaterial::SetBinary(Gfx::ShaderType shaderType, const GfxShaderBinaryBundle& bundle)
{
    CriticalAssert(ValidEnum(shaderType));
    mBundles[EnumValue(shaderType)] = bundle;
}
const GfxShaderBinaryBundle& GfxMaterial::GetBinary(Gfx::ShaderType shaderType) const
{
    CriticalAssert(ValidEnum(shaderType));
    return mBundles[EnumValue(shaderType)];
}

bool GfxMaterial::SetProperty(const Token& name, Float32 value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_FLOAT);
}
bool GfxMaterial::SetProperty(const Token& name, Int32 value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_INT);
}
bool GfxMaterial::SetProperty(const Token& name, UInt32 value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_UINT);
}
bool GfxMaterial::SetProperty(const Token& name, const Vector2& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR2);
}
bool GfxMaterial::SetProperty(const Token& name, const Vector3& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR3);
}
bool GfxMaterial::SetProperty(const Token& name, const Vector4& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR4);
}
bool GfxMaterial::SetProperty(const Token& name, const Color& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR4);
}
bool GfxMaterial::SetProperty(const Token& name, const Matrix& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(name, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_MATRIX_4x4);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, Float32 value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_FLOAT);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, Int32 value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_INT);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, UInt32 value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_UINT);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, const Vector2& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR2);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, const Vector3& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR3);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, const Vector4& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR4);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, const Color& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_VECTOR4);
}
bool GfxMaterial::SetProperty(Gfx::MaterialPropertyId id, const Matrix& value)
{
    return GetAdapterAs<GfxMaterialAdapter>()->SetProperty(id, &value, sizeof(value), Gfx::ShaderAttribFormat::SAF_MATRIX_4x4);
}

Gfx::MaterialPropertyId GfxMaterial::FindProperty(const Token& name) const
{
    return GetAdapterAs<GfxMaterialAdapter>()->FindProperty(name);
}
#endif

DEFINE_CLASS(lf::GfxMaterial) { NO_REFLECTION; }

} // namespace lf
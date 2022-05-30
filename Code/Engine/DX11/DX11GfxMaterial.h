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
#include "AbstractEngine/Gfx/GfxMaterial.h"
#include "Core/Utility/APIResult.h"
#include "AbstractEngine/Gfx/GfxMaterialProperty.h"
#include "Engine/DX11/DX11Common.h"

namespace lf {
#if defined(LF_DIRECTX11)


DECLARE_MANAGED_CPTR(AssetTypeInfo);


class LF_ENGINE_API DX11GfxMaterialAdapter : public GfxMaterialAdapter
{
    DECLARE_CLASS(DX11GfxMaterialAdapter, GfxMaterialAdapter);
public:
    struct PSO
    {
        // Shader bytecode(vertex, pixel, domain, hull, and geometry shaders)
        ComPtr<ID3D11VertexShader>    mVertexShader;
        ComPtr<ID3D11PixelShader>     mPixelShader;

        // Vertex format input layout
        ComPtr<ID3D11InputLayout>     mInputLayout;

        // Primitive topology type(point, line, triangle, or patch)
        D3D11_PRIMITIVE_TOPOLOGY      mTopology;

        // Blend state
        ComPtr<ID3D11BlendState>      mBlendState;
        // Enable()
        // static const Float32 BLEND_MASK[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        // deviceContext->OMSetBlendState(mState, BLEND_MASK, 0xFFFFFFFF);

        // Rasterizer state
        ComPtr<ID3D11RasterizerState> mRasterState;
        // Enable()
        // deviceContext->RSSetState(mState);

        // Depth - stencil state
        ComPtr<ID3D11DepthStencilState> mDepthState;
        
        // Enable()
        // deviceContext->OMSetDepthStencilState(mState, 1); // TODO: Figure out why 1?
        // deviceContext->OMSetRenderTargets(1, &view, mView);

        // Multisample description

        ComPtr<ID3D11Buffer>            mConstantBuffer;
        TVector<ByteT>                   mConstantCPUBuffer;

        bool                            mUseDepth;
    };

    DX11GfxMaterialAdapter();
    virtual ~DX11GfxMaterialAdapter() = default;
    void OnInitialize(GfxDependencyContext& context) override;
    void OnShutdown() override;

    bool CompileShader(TVector<ByteT>& outByteCode, const GfxMaterial& material, Gfx::ShaderType shaderType) override;

    APIResult<bool> CreatePSO(const GfxMaterial& material, const Gfx::PipelineStateDesc& desc);
    APIResult<bool> UploadProperties();

    APIResult<bool> SetProperty(const Token& propertyName, const void* data, SizeT dataSize, Gfx::ShaderAttribFormat type) override;
    APIResult<bool> SetProperty(Gfx::MaterialPropertyId propertyID, const void* data, SizeT dataSize, Gfx::ShaderAttribFormat type) override;

    Gfx::MaterialPropertyId FindProperty(const Token& propertyName) const override;

    PSO& GetPipelineState() { return mPSO; }
private:
    APIResult<bool> LoadShadersFromSource(const GfxMaterial& material, Gfx::PipelineStateDesc& shaderDesc);

    APIResult<bool> CompileShader(const char* version, const String& text, TVector<ByteT>& outByteCode, const AssetTypeInfoCPtr& type);
    APIResult<bool> CreateBlendState(const GfxMaterial& material);
    APIResult<bool> CreateDepthState(const GfxMaterial& material);
    APIResult<bool> CreateRasterState(const GfxMaterial& material);
    APIResult<bool> CreateVertexFormat(const GfxMaterial& material, const Gfx::VertexFormat& format, const TVector<ByteT>& vertexByteCode);
    APIResult<bool> CreateConstantBuffer();


    

    PSO mPSO;
    ComPtr<ID3D11Device> mDevice;
    ComPtr<ID3D11DeviceContext> mDeviceContext;
    

    Gfx::MaterialPropertyContainer mPropertyContainer;

};

class LF_ENGINE_API DX11GfxMaterial : public GfxMaterial
{
    DECLARE_CLASS(DX11GfxMaterial, GfxMaterial);
public:
    DX11GfxMaterial();
    ~DX11GfxMaterial();

    void Commit() override;
private:
};
#endif

} // namespace lf 

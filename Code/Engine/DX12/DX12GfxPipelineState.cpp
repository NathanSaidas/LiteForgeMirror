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
#include "Engine/PCH.h"
#include "DX12GfxPipelineState.h"
#include "Core/Reflection/DynamicCast.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Error.h"
#include "AbstractEngine/Gfx/GfxShader.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include "Engine/DX12/DX12Util.h"
#include <d3dx12.h>

namespace lf {
 
DEFINE_CLASS(lf::DX12GfxPipelineState) { NO_REFLECTION; }

static const CD3DX12_STATIC_SAMPLER_DESC STATIC_SAMPLERS[] = 
{
    // PointWrap
    CD3DX12_STATIC_SAMPLER_DESC(
        0,                                  // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,     // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,    // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,    // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP),   // addressW

    // PointClamp
    CD3DX12_STATIC_SAMPLER_DESC(
        1,                                  // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,     // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP),  // addressW

    // LinearWrap
    CD3DX12_STATIC_SAMPLER_DESC(
        2,                                  // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,    // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,    // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP),   // addressW

    // LinearClamp
    CD3DX12_STATIC_SAMPLER_DESC(
        3,                                  // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP),  // addressW

    // AnisotropicWrap
    CD3DX12_STATIC_SAMPLER_DESC(
        4,                                  // shaderRegister
        D3D12_FILTER_ANISOTROPIC,           // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,    // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,    // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,    // addressW
        0.0f,                               // mipLODBias
        8),                                 // maxAnisotropy

    // AnisotropicClamp
    CD3DX12_STATIC_SAMPLER_DESC(
        5,                                  // shaderRegister
        D3D12_FILTER_ANISOTROPIC,           // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,   // addressW
        0.0f,                               // mipLODBias
        8)                                  // maxAnisotropy
};


DX12GfxPipelineState::DX12GfxPipelineState()
: Super()
{
}

void DX12GfxPipelineState::Release()
{
    mPSO.Reset();

    Super::Release();
    SetGPUReady(false);
    Invalidate();
}

void DX12GfxPipelineState::Commit(GfxDevice& device, GfxCommandContext&)
{
    if (!AllowChanges())
    {
        return;
    }

    auto& shaderParams = GetShaderParams();
    SizeT textureCount = 0;
    for (const Gfx::ShaderParam& param : shaderParams)
    {
        if (!param.IsValid())
        {
            return;
        }

        if (param.GetType() == Gfx::ShaderParamType::SPT_TEXTURE_2D)
        {
            ++textureCount;
        }
    }


    ComPtr<ID3D12Device> dx12 = GetDX12Device(device);

    TStackVector<CD3DX12_DESCRIPTOR_RANGE1, 8> textures;
    TStackVector<CD3DX12_ROOT_PARAMETER1, 8> slotRootParameters;
    D3D12_VERSIONED_ROOT_SIGNATURE_DESC rootDesc = {};
    rootDesc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
    rootDesc.Desc_1_1.NumParameters = 0;
    rootDesc.Desc_1_1.pParameters = nullptr;

    
    if (!shaderParams.empty())
    {
        textures.reserve(textureCount);
        slotRootParameters.reserve(shaderParams.size());

        for (const Gfx::ShaderParam& param : shaderParams)
        {
            switch (param.GetType())
            {
                case Gfx::ShaderParamType::SPT_TEXTURE_2D:
                {
                    textures.push_back(CD3DX12_DESCRIPTOR_RANGE1());
                    textures.back().Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, static_cast<UINT>(param.GetRegister()));
                    CriticalAssert(textures.size() <= (textures.capacity()));

                    slotRootParameters.push_back(CD3DX12_ROOT_PARAMETER1());
                    slotRootParameters.back().InitAsDescriptorTable(1, &textures.back(), Gfx::ToDX12(param.GetVisibility()));

                } break;
                case Gfx::ShaderParamType::SPT_CONSTANT_BUFFER:
                {
                    slotRootParameters.push_back(CD3DX12_ROOT_PARAMETER1());
                    slotRootParameters.back().InitAsConstantBufferView(static_cast<UINT>(param.GetRegister()), 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, Gfx::ToDX12(param.GetVisibility()));
                } break;
                case Gfx::ShaderParamType::SPT_STRUCTURED_BUFFER:
                {
                    slotRootParameters.push_back(CD3DX12_ROOT_PARAMETER1());
                    slotRootParameters.back().InitAsShaderResourceView(static_cast<UINT>(param.GetRegister()), 0, D3D12_ROOT_DESCRIPTOR_FLAG_NONE, Gfx::ToDX12(param.GetVisibility()));
                } break;
                default:
                    CriticalAssertMsg("InvalidEnum");
                    break;
            }
        }
        rootDesc.Desc_1_1.NumParameters = static_cast<UINT>(slotRootParameters.size());
        rootDesc.Desc_1_1.pParameters = slotRootParameters.data();
    }

    rootDesc.Desc_1_1.NumStaticSamplers = LF_ARRAY_SIZE(STATIC_SAMPLERS);
    rootDesc.Desc_1_1.pStaticSamplers = STATIC_SAMPLERS;
    rootDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3D10Blob> rootBlob;
    ComPtr<ID3D10Blob> rootErrorBlob;

    HRESULT hr = D3D12SerializeVersionedRootSignature(&rootDesc, rootBlob.ReleaseAndGetAddressOf(), rootErrorBlob.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        if (rootErrorBlob)
        {
            const String errorMsg(static_cast<SizeT>(rootErrorBlob->GetBufferSize()), reinterpret_cast<const char*>(rootErrorBlob->GetBufferPointer()), COPY_ON_WRITE);
            LF_DEBUG_BREAK;
        }
        SetGPUReady(false);
        Invalidate();
        return;
    }

    hr = dx12->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(), IID_PPV_ARGS(&mRootSignature));
    if (FAILED(hr))
    {
        SetGPUReady(false);
        Invalidate();
        return;
    }

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    // unused;
    psoDesc.DS.BytecodeLength = 0;
    psoDesc.DS.pShaderBytecode = nullptr;
    psoDesc.HS.BytecodeLength = 0;
    psoDesc.HS.pShaderBytecode = nullptr;
    psoDesc.GS.BytecodeLength = 0;
    psoDesc.GS.pShaderBytecode = nullptr;
    psoDesc.StreamOutput = Gfx::DEFAULT_STREAM_OUTPUT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.NumRenderTargets = 1;
    psoDesc.DSVFormat = DXGI_FORMAT_UNKNOWN; //  DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;
    psoDesc.IBStripCutValue = D3D12_INDEX_BUFFER_STRIP_CUT_VALUE_DISABLED;
    psoDesc.NodeMask = 0;
    psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    psoDesc.CachedPSO.CachedBlobSizeInBytes = 0;
    psoDesc.CachedPSO.pCachedBlob = nullptr;

    // used
    psoDesc.BlendState = Gfx::ToDX12(GetBlendState());
    psoDesc.RasterizerState = Gfx::ToDX12(GetRasterState());
    psoDesc.DepthStencilState = Gfx::ToDX12(GetDepthStencilState());
    psoDesc.VS = Gfx::ToDX12(GetShaderByteCode(Gfx::ShaderType::VERTEX));
    psoDesc.PS = Gfx::ToDX12(GetShaderByteCode(Gfx::ShaderType::PIXEL));
    psoDesc.PrimitiveTopologyType = Gfx::ToTopologyType(GetRenderMode());
    psoDesc.RTVFormats[0] = Gfx::DX12Value(GetRenderTargetFormat());
    TStackVector<D3D12_INPUT_ELEMENT_DESC, 8> layoutDesc;
    ToDX12(GetInputLayout(), layoutDesc);
    psoDesc.InputLayout.NumElements = static_cast<UINT>(layoutDesc.size());
    psoDesc.InputLayout.pInputElementDescs = layoutDesc.data();
    psoDesc.pRootSignature = mRootSignature.Get();

    hr = dx12->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO));
    if (FAILED(hr))
    {
        SetGPUReady(false);
        Invalidate();
        return;
    }

    SetGPUReady(true);
}

// void DX12GfxMaterial::Serialize(Stream& s)
// {
//     Super::Serialize(s);
// 
//     const char* const* strings = Gfx::TShaderType::GetPrettyStrings();
//     for (SizeT i = 0; i < ENUM_SIZE(Gfx::ShaderType); ++i)
//     {
//         String propName(strings[i]);
//         propName += "Shader";
//         SERIALIZE_NAMED(s, propName.CStr(), mShaders[i], "");
//     }
// }

// void DX12GfxMaterial::Commit()
// {
//     DX12GfxMaterialAdapter* adapter = GetAdapterAs<DX12GfxMaterialAdapter>();
// 
//     TVector<ByteT> vertex;
//     TVector<ByteT> pixel;
// 
//     if(
//        adapter->CompileShader(vertex, *this, Gfx::ShaderType::VERTEX)
//     && adapter->CompileShader(pixel, *this, Gfx::ShaderType::PIXEL)
//     )
//     {
//         Gfx::PipelineStateDesc desc;
//         desc.mByteCode[EnumValue(Gfx::ShaderType::VERTEX)] = vertex;
//         desc.mByteCode[EnumValue(Gfx::ShaderType::PIXEL)] = pixel;
//         Assert(adapter->CreatePSO(*this, desc));
//     }
//     else
//     {
//         AssertMsg("Failed to create GfxMaterial");
//     }
// }




} // namespace lf
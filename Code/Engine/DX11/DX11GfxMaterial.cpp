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
#include "DX11GfxMaterial.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Utility/Error.h"
#include "Core/Utility/Log.h"
#include "Runtime/Asset/AssetTypeInfo.h"
#include "AbstractEngine/Gfx/GfxShader.h"
#include "Engine/DX11/DX11GfxDependencyContext.h"

namespace lf {
#if defined(LF_DIRECTX11)

// TODO:
// Implement the CreatePSO function
// -- Load Vertex Shader from Cache
// -- Load Pixel Shader from Cache
// -- Load Vertex Format from Material Cache
// Implement Bind PSO Function Bind( GraphicsCommandList& )
//      => Bind ( DX11GraphicsCommandList.GetDeviceContext() )
//      => Bind ( DX12GraphicsCommandList.GetCommandList() )

DEFINE_CLASS(lf::DX11GfxMaterialAdapter) { NO_REFLECTION; }
DEFINE_CLASS(lf::DX11GfxMaterial) { NO_REFLECTION; }

DX11GfxMaterialAdapter::DX11GfxMaterialAdapter()
: Super()
, mPSO()
, mDevice()
, mDeviceContext()
, mPropertyContainer()
{
}

void DX11GfxMaterialAdapter::OnInitialize(GfxDependencyContext& context)
{
    DX11GfxDependencyContext* dx11 = DynamicCast<DX11GfxDependencyContext>(&context);
    mDevice = dx11->GetDevice();
    mDeviceContext = dx11->GetDeviceContext();

}
void DX11GfxMaterialAdapter::OnShutdown()
{

}
bool DX11GfxMaterialAdapter::CompileShader(TVector<ByteT>& outByteCode, const GfxMaterial& material, Gfx::ShaderType shaderType)
{
    (outByteCode);
    (material);
    (shaderType);
    return false;
}

APIResult<bool> DX11GfxMaterialAdapter::CreatePSO(const GfxMaterial& material, const Gfx::PipelineStateDesc& desc)
{
    gSysLog.Warning(LogMessage("TODO: (CreateDepthState) Support loading from cache, current this will compile shaders from source!"));
    (desc);

    // Load Vertex Shader:
    APIResult<bool> result(false);

    Gfx::PipelineStateDesc shaderDesc;
    result = LoadShadersFromSource(material, shaderDesc);
    if (!result)
    {
        return result;
    }
    // Create Shaders:
    Gfx::PipelineStateDesc* targetDesc = &shaderDesc;

    HRESULT hr = mDevice->CreateVertexShader(
        targetDesc->mByteCode[EnumValue(Gfx::ShaderType::VERTEX)].data(), 
        targetDesc->mByteCode[EnumValue(Gfx::ShaderType::VERTEX)].size(), 
        NULL, 
        mPSO.mVertexShader.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "Failed to create Vertex Shader", material.GetAssetPath().CStr());
    }
    
    hr = mDevice->CreatePixelShader(
        targetDesc->mByteCode[EnumValue(Gfx::ShaderType::PIXEL)].data(),
        targetDesc->mByteCode[EnumValue(Gfx::ShaderType::PIXEL)].size(),
        NULL,
        mPSO.mPixelShader.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "Failed to create Pixel Shader", material.GetAssetPath().CStr());
    }

    result = CreateBlendState(material);
    if (!result)
    {
        return result;
    }
    
    result = CreateDepthState(material);
    if (!result)
    {
        return result;
    }
    result = CreateRasterState(material);
    if (!result)
    {
        return result;
    }
    result = CreateVertexFormat(material, targetDesc->mVertexFormat, targetDesc->mByteCode[EnumValue(Gfx::ShaderType::VERTEX)]);
    if (!result)
    {
        return result;
    }
    result = CreateConstantBuffer();
    if (!result)
    {
        return result;
    }

    mPSO.mTopology = Gfx::DX11Value(material.GetRenderMode());

    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::UploadProperties()
{
    D3D11_MAPPED_SUBRESOURCE propertyData;
    HRESULT result = mDeviceContext->Map(mPSO.mConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &propertyData);
    if (FAILED(result))
    {
        return ReportError(false, OperationFailureError, "Failed to upload properties (API Error ID3D11DeviceContext::Map)", "<NONE>");
    }

    memcpy(propertyData.pData, mPSO.mConstantCPUBuffer.data(), mPSO.mConstantCPUBuffer.size());
    mDeviceContext->Unmap(mPSO.mConstantBuffer.Get(), 0);
    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::SetProperty(const Token& propertyName, const void* data, SizeT dataSize, Gfx::ShaderAttribFormat type)
{
    TUnsafePtr<Gfx::MaterialProperty> property = mPropertyContainer.FindProperty(propertyName);
    if (!property)
    {
        return APIResult<bool>(false);
    }

    if (ValidEnum(type) && static_cast<Gfx::ShaderAttribFormat>(property->mType) != type)
    {
        return ReportError(false, InvalidArgumentError, "type", "Types must match!");
    }

    SizeT size = static_cast<SizeT>(property->mSize);
    if (dataSize > size)
    {
        return ReportError(false, InvalidArgumentError, "dataSize", "Argument exceeds the size of the property.");
    }
    SizeT offset = static_cast<SizeT>(property->mOffset);

    SizeT left = offset;
    SizeT right = offset + dataSize;
    SizeT bufferSize = mPSO.mConstantCPUBuffer.size();
    if (left >= bufferSize || right > bufferSize)
    {
        return ReportError(false, OperationFailureError, "Failed to set property, index out of bounds.", property->mName.CStr());
    }

    ByteT* cpuBuffer = mPSO.mConstantCPUBuffer.data();
    memcpy(&cpuBuffer[offset], data, dataSize);
    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::SetProperty(Gfx::MaterialPropertyId propertyID, const void* data, SizeT dataSize, Gfx::ShaderAttribFormat type)
{
    TUnsafePtr<Gfx::MaterialProperty> property = mPropertyContainer.FindProperty(propertyID);
    if (!property)
    {
        return APIResult<bool>(false);
    }

    if (ValidEnum(type) && static_cast<Gfx::ShaderAttribFormat>(property->mType) != type)
    {
        return ReportError(false, InvalidArgumentError, "type", "Types must match!");
    }

    SizeT size = static_cast<SizeT>(property->mSize);
    if (dataSize > size)
    {
        return ReportError(false, InvalidArgumentError, "dataSize", "Argument exceeds the size of the property.");
    }
    SizeT offset = static_cast<SizeT>(property->mOffset);

    SizeT left = offset;
    SizeT right = offset + dataSize;
    SizeT bufferSize = mPSO.mConstantCPUBuffer.size();
    if (left >= bufferSize || right > bufferSize)
    {
        return ReportError(false, OperationFailureError, "Failed to set property, index out of bounds.", property->mName.CStr());
    }

    ByteT* cpuBuffer = mPSO.mConstantCPUBuffer.data();
    memcpy(&cpuBuffer[offset], data, dataSize);
    return APIResult<bool>(true);
}

Gfx::MaterialPropertyId DX11GfxMaterialAdapter::FindProperty(const Token& propertyName) const
{
    return mPropertyContainer.FindPropertyID(propertyName);
}

APIResult<bool> DX11GfxMaterialAdapter::LoadShadersFromSource(const GfxMaterial& material, Gfx::PipelineStateDesc& desc)
{
    (desc);

    // Fetch and validate shaders.
    const GfxShaderAsset& vertexShader = material.GetShader(Gfx::ShaderType::VERTEX);
    if (!vertexShader)
    {
        return ReportError(false, InvalidArgumentError, "material", "Material doesn't have vertex shader!", material.GetAssetPath().CStr());
    }
    if (vertexShader.GetType() && !vertexShader.IsLoaded())
    {
        return ReportError(false, InvalidArgumentError, "material.VertexShader", "Cannot load shaders from source when the asset is not yet loaded.", vertexShader.GetType()->GetPath().CStr());
    }
    const GfxShaderAsset& pixelShader = material.GetShader(Gfx::ShaderType::PIXEL);
    if (!pixelShader)
    {
        return ReportError(false, InvalidArgumentError, "material", "Material doesn't have pixel shader!", material.GetAssetPath().CStr());
    }
    if (pixelShader.GetType() && !pixelShader.IsLoaded())
    {
        return ReportError(false, InvalidArgumentError, "material.PixelShader", "Cannot load shaders from source when the asset is not yet loaded.", pixelShader.GetType()->GetPath().CStr());
    }

    // Generate shader text
    Gfx::ShaderTextInfo vertexText;
    Gfx::ShaderTextInfo pixelText;
    if (!vertexShader->GenerateText(vertexText, Gfx::ShaderType::VERTEX, material.GetDefines()))
    {
        return ReportError(false, OperationFailureError, "Failed to generate vertex shader text.", vertexShader.GetType()->GetPath().CStr());;
    }
    if (!pixelShader->GenerateText(pixelText, Gfx::ShaderType::PIXEL, material.GetDefines()))
    {
        return ReportError(false, OperationFailureError, "Failed to generate pixel shader text.", pixelShader.GetType()->GetPath().CStr());
    }

    if (vertexText.mVertexFormat.empty())
    {
        return ReportError(false, OperationFailureError, "Vertex shader doesn't have vertex format.", vertexShader.GetType()->GetPath().CStr());
    }

    // Compute Vertex Format
    APIResult<bool> result(true);
    for (const auto& formatInfo : vertexText.mVertexFormat)
    {
        result = desc.mVertexFormat.Add(Gfx::GetShaderAttribFormat(formatInfo.mTypeName), formatInfo.mSemantic, formatInfo.mName, 0);
        if (!result)
        {
            return result;
        }
        if (material.GetVertexMultiBuffer())
        {
            desc.mVertexFormat.PushInputSlot();
        }
    }

    // Compute Properties
    SizeT offset = 0;
    for (const auto& propertyInfo : vertexText.mProperties)
    {
        Gfx::ShaderAttribFormat format = Gfx::GetShaderAttribFormat(Token(propertyInfo.mTypeName));
        switch (format)
        {
            case Gfx::ShaderAttribFormat::SAF_FLOAT:
            case Gfx::ShaderAttribFormat::SAF_INT:
            case Gfx::ShaderAttribFormat::SAF_UINT:
            case Gfx::ShaderAttribFormat::SAF_VECTOR2:
            case Gfx::ShaderAttribFormat::SAF_VECTOR3:
            case Gfx::ShaderAttribFormat::SAF_VECTOR4:
            case Gfx::ShaderAttribFormat::SAF_MATRIX_3X3:
            case Gfx::ShaderAttribFormat::SAF_MATRIX_4x4:
            {
                SizeT propertySize = Gfx::SHADER_ATTRIB_FORMAT_TO_SIZE[EnumValue(format)];
                mPropertyContainer.AddProperty(
                    Token(propertyInfo.mName),
                    static_cast<UInt8>(format),
                    static_cast<UInt8>(propertySize),
                    static_cast<UInt16>(offset));
                offset += propertySize;
            } break;
            case Gfx::ShaderAttribFormat::SAF_TEXTURE:
            {
                mPropertyContainer.AddTextureAsset(
                    Token(propertyInfo.mName),
                    static_cast<UInt8>(format),
                    0,
                    static_cast<UInt32>(propertyInfo.mIndex),
                    TAssetType<GfxObject>()); // TODO Update to GfxTexture
            } break;
            case Gfx::ShaderAttribFormat::SAF_SAMPLER:
            {
                // We skip this...
            } break;
            default:
            {
                mPropertyContainer.Clear();
                return ReportError(false, OperationFailureError, "Vertex shader has unknown property type.", vertexShader.GetType()->GetPath().CStr());
            } break;
        }
    }

    result = CompileShader("vs_5_0", vertexText.mText, desc.mByteCode[EnumValue(Gfx::ShaderType::VERTEX)], AssetTypeInfoCPtr(vertexShader.GetType()));
    if (!result)
    {
        return result;
    }
    result = CompileShader("ps_5_0", pixelText.mText, desc.mByteCode[EnumValue(Gfx::ShaderType::PIXEL)], AssetTypeInfoCPtr(pixelShader.GetType()));
    if (!result)
    {
        return result;
    }
    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::CompileShader(const char* version, const String& text, TVector<ByteT>& outByteCode, const AssetTypeInfoCPtr& type)
{
    ComPtr<ID3D10Blob> errorMessage;
    ComPtr<ID3D10Blob> shaderBuffer;
    HRESULT result = D3DCompile(
                        text.CStr(), text.Size(),
                        NULL, NULL, NULL, "main",
                        version,
                        D3DCOMPILE_ENABLE_STRICTNESS,
                        0,
                        &shaderBuffer,
                        &errorMessage);
    if (FAILED(result))
    {
        return ReportError(false, ShaderCompilationError, static_cast<const char*>(errorMessage->GetBufferPointer()), type->GetPath().CStr());
    }
    outByteCode.resize(shaderBuffer->GetBufferSize());
    memcpy(outByteCode.data(), shaderBuffer->GetBufferPointer(), outByteCode.size());
    return APIResult<bool>(true);
}
APIResult<bool> DX11GfxMaterialAdapter::CreateBlendState(const GfxMaterial& material)
{
    HRESULT hr = S_OK;
    D3D11_BLEND_DESC desc;
    ZeroMemory(&desc, sizeof(desc));

    desc.AlphaToCoverageEnable = FALSE; // TODO: Check out stuff on foliage. This might be useful.
    desc.IndependentBlendEnable = FALSE; // Single render target.
    desc.RenderTarget[0].BlendEnable = static_cast<BOOL>(material.GetBlendEnabled());
    desc.RenderTarget[0].SrcBlend = DX11Value(material.GetBlendSrc());
    desc.RenderTarget[0].DestBlend = DX11Value(material.GetBlendDest());
    desc.RenderTarget[0].BlendOp = DX11Value(material.GetBlendOp());
    desc.RenderTarget[0].SrcBlendAlpha = DX11Value(material.GetBlendSrcAlpha());
    desc.RenderTarget[0].DestBlendAlpha = DX11Value(material.GetBlendDestAlpha());
    desc.RenderTarget[0].BlendOpAlpha = DX11Value(material.GetBlendAlphaOp());
    desc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

    for (SizeT i = 1; i < LF_ARRAY_SIZE(desc.RenderTarget); ++i)
    {
        desc.RenderTarget[i] = desc.RenderTarget[0];
    }

    hr = mDevice->CreateBlendState(&desc, mPSO.mBlendState.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "Failed to create blend state.", material.GetAssetPath().CStr());
    }

    gGfxLog.Info(LogMessage("CreateBlendState ") << material.GetAssetPath().CStr() << "\n"
        << "BlendEnabled=" << (material.GetBlendEnabled() ? "true":"false")
        << "BlendSrc=" << Gfx::TBlendType::GetString(material.GetBlendSrc()));

    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::CreateDepthState(const GfxMaterial& material)
{
    HRESULT hr = S_OK;
    

    D3D11_DEPTH_STENCIL_DESC stateDesc;
    ZeroMemory(&stateDesc, sizeof(stateDesc));

    stateDesc.DepthEnable = static_cast<BOOL>(material.GetDepthEnabled());
    stateDesc.DepthWriteMask = material.GetDepthWrite() ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    stateDesc.DepthFunc = DX11Value(material.GetDepthFunc());
    stateDesc.StencilEnable = FALSE;
    stateDesc.StencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
    stateDesc.StencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
    stateDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    stateDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    stateDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    stateDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
    stateDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
    stateDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
    stateDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
    stateDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;

    hr = mDevice->CreateDepthStencilState(&stateDesc, &mPSO.mDepthState);
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "Failed to create Depth State (Depth State) for material.", material.GetAssetPath().CStr());
    }

    mPSO.mUseDepth = material.GetDepthEnabled();


    // gGfxLog.Info(LogMessage("CreateDepthState ") << material.GetAssetPath().CStr() << "\n"
    //     << "DepthEnabled=" << (material.GetDepthEnabled() ? "true" : "false"));

    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::CreateRasterState(const GfxMaterial& material)
{
    HRESULT hr = S_OK;
    D3D11_RASTERIZER_DESC rasterDesc;
    rasterDesc.FillMode = material.GetRasterWireframe() ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    rasterDesc.CullMode = DX11Value(material.GetRasterCullFace());
    gSysLog.Warning(LogMessage("TODO: (CreateRasterState) use material value for bool FrontCounterClockwise."));
    rasterDesc.FrontCounterClockwise = static_cast<BOOL>(false);
    rasterDesc.DepthBias = 0;
    rasterDesc.DepthBiasClamp = 0.0f;
    rasterDesc.DepthClipEnable = TRUE;
    rasterDesc.MultisampleEnable = material.GetRasterMSAA();
    rasterDesc.ScissorEnable = TRUE;
    rasterDesc.SlopeScaledDepthBias = 0.0f;
    rasterDesc.AntialiasedLineEnable = material.GetRasterLineAA();

    hr = mDevice->CreateRasterizerState(&rasterDesc, mPSO.mRasterState.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "Failed to create Raster State for material.", material.GetAssetPath().CStr());
    }
    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::CreateVertexFormat(const GfxMaterial& material, const Gfx::VertexFormat& format, const TVector<ByteT>& vertexByteCode)
{
    // Convert to D3D11_INPUT_ELEMENT_DESC
    TStackVector<D3D11_INPUT_ELEMENT_DESC, 8> desc;
    desc.resize(format.GetElements().size());

    const Gfx::GfxVertexElement* inData = format.GetElements().data();
    D3D11_INPUT_ELEMENT_DESC* outData = desc.data();
    for (size_t i = 0, size = desc.size(); i < size; ++i)
    {
        const Gfx::GfxVertexElement& inCurrent = inData[i];
        D3D11_INPUT_ELEMENT_DESC& outCurrent = outData[i];
        outCurrent.SemanticName = inCurrent.mSemantic.CStr();
        outCurrent.SemanticIndex = static_cast<UINT>(inCurrent.mIndex);
        outCurrent.Format = Gfx::SHADER_ATTRIB_FORMAT_TO_DXGI[static_cast<SizeT>(inCurrent.mFormat)];
        outCurrent.AlignedByteOffset = static_cast<UINT>(inCurrent.mByteOffset);
        outCurrent.InputSlot = static_cast<UINT>(inCurrent.mInputSlot);
        outCurrent.InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
        outCurrent.InstanceDataStepRate = 0;
    }

    HRESULT hr = mDevice->CreateInputLayout(outData, static_cast<UINT>(desc.size()), vertexByteCode.data(), vertexByteCode.size(), mPSO.mInputLayout.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "Failed to create Vertex Format (Input Layout)", material.GetAssetPath().CStr());
    }

    return APIResult<bool>(true);
}

APIResult<bool> DX11GfxMaterialAdapter::CreateConstantBuffer()
{
    D3D11_BUFFER_DESC desc;
    desc.ByteWidth = static_cast<UINT>(NextMultiple(mPropertyContainer.GetPropertyBufferSize(), 16));
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    desc.MiscFlags = 0;
    desc.StructureByteStride = 0;

    HRESULT hr = mDevice->CreateBuffer(&desc, nullptr, mPSO.mConstantBuffer.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return ReportError(false, OperationFailureError, "Failed to create constant buffer for material. (DX11Device::CreateBuffer)", "<NONE>");
    }

    mPSO.mConstantCPUBuffer.resize(desc.ByteWidth);
    return APIResult<bool>(true);
}

DX11GfxMaterial::DX11GfxMaterial()
: Super()
{

}
DX11GfxMaterial::~DX11GfxMaterial()
{

}

void DX11GfxMaterial::Commit()
{
    GetAdapterAs<DX11GfxMaterialAdapter>()->CreatePSO(*this, Gfx::PipelineStateDesc());
}
#endif

} // namespace lf
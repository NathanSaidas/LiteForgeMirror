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
#include "DX11GfxShader.h"
#include "Core/Utility/Log.h"
#include "Engine/DX11/DX11GfxDependencyContext.h"

namespace lf {
#if defined(LF_DIRECTX11)

DEFINE_CLASS(lf::DX11GfxShaderAdapter) { NO_REFLECTION; }
DEFINE_CLASS(lf::DX11GfxShader) { NO_REFLECTION; }

DX11GfxShaderAdapter::DX11GfxShaderAdapter()
: Super()
{
}
void DX11GfxShaderAdapter::OnInitialize(GfxDependencyContext& context)
{
    DX11GfxDependencyContext* dx11 = DynamicCast<DX11GfxDependencyContext>(&context);
    if (dx11)
    {
        mDevice = dx11->GetDevice();
        mDeviceContext = dx11->GetDeviceContext();
    }
}
void DX11GfxShaderAdapter::OnShutdown()
{
    mDevice = ComPtr<ID3D11Device>();
    mDeviceContext = ComPtr<ID3D11DeviceContext>();


}

static const char* GetShaderVersion(Gfx::ShaderType shaderType)
{
    switch (shaderType)
    {
    case Gfx::ShaderType::VERTEX: return "vs_5_0";
    case Gfx::ShaderType::PIXEL: return "ps_5_0";
    default:
        break;
    }
    return "";
}

APIResult<bool> DX11GfxShaderAdapter::CreateShaderFromText(Gfx::ShaderType shaderType, const String& text)
{
    if (InvalidEnum(shaderType))
    {
        return ReportError(false, InvalidArgumentError, "Invalid shader type supplied.", "shaderType");
    }

    if (text.Empty())
    {
        return ReportError(false, InvalidArgumentError, "Shader cannot compile empty text.", "text");
    }

    const SizeT shaderIndex = EnumValue(shaderType);

    // Release current shaders:
    if (mShader)
    {
        mShader->Release();
        mShader = ComPtr<IUnknown>();
    }

    // Compile the shader
    const char* ENTRY_POINT = "main";
    ComPtr<ID3D10Blob> errorMessage;
    ComPtr<ID3D10Blob> shaderBuffer;
    
    HRESULT result = D3D10CompileShader(text.CStr(), text.Size(),
        NULL,
        NULL,
        NULL,
        ENTRY_POINT,
        GetShaderVersion(shaderType),
        D3D10_SHADER_ENABLE_STRICTNESS,
        shaderBuffer.ReleaseAndGetAddressOf(),
        errorMessage.ReleaseAndGetAddressOf());

    if (FAILED(result))
    {
        if (errorMessage)
        {
            gGfxLog.Error(LogMessage("Failed to compile ") << Gfx::TShaderType(shaderType).GetString() << " Shader \"" << GetShaderName() << "\""
                << reinterpret_cast<char*>(errorMessage->GetBufferPointer()));
        }
        else
        {
            gGfxLog.Error(LogMessage("Failed to compile ") << Gfx::TShaderType(shaderType).GetString() << " Shader \"" << GetShaderName() << "\"");
        }
        return ReportError(false, OperationFailureError, "Shader failed to compile.", "text");
    }

    // Initialize internal state
    mByteCode.resize(shaderBuffer->GetBufferSize());
    memcpy(mByteCode.data(), shaderBuffer->GetBufferPointer(), shaderBuffer->GetBufferSize());
    mShaderType = shaderType;

    // Create the shader
    return CreateShader();
}

APIResult<bool> DX11GfxShaderAdapter::CreateShaderFromBytes(Gfx::ShaderType shaderType, const ByteT* memory, SizeT numBytes)
{
    if (InvalidEnum(shaderType))
    {
        return ReportError(false, InvalidArgumentError, "Invalid shader type supplied.", "DX11GfxShaderAdapter::CreateShaderFromBytes shaderType");
    }

    if (memory == nullptr || numBytes == 0)
    {
        return ReportError(false, ArgumentNullError, "DX11GfxShaderAdapter::CreateShaderFromBytes memory");
    }

    // Initialize internal state
    mByteCode.resize(numBytes);
    memcpy(mByteCode.data(), memory, numBytes);
    mShaderType = shaderType;

    return CreateShader();
}

APIResult<bool> DX11GfxShaderAdapter::CreateShader()
{
    if (mByteCode.empty())
    {
        return ReportError(false, OperationFailureError, "Missing byte code.", "DX11GfxShaderAdapter::mByteCode");
    }

    HRESULT result;
    switch (mShaderType)
    {
    case Gfx::ShaderType::VERTEX:
    {
        ComPtr<ID3D11VertexShader> shader;

        result = mDevice->CreateVertexShader(mByteCode.data(), mByteCode.size(), NULL, shader.ReleaseAndGetAddressOf());
        if (FAILED(result))
        {
            gGfxLog.Error(LogMessage("Failed to create Vertex Shader handle."));
            shader = ComPtr<ID3D11VertexShader>();
            return ReportError(false, OperationFailureError, "Shader failed to create handle.", "API - internal");
        }

#if defined(LF_DEBUG)
        String name = GetShaderName();
        shader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.Size()), name.CStr());
#endif
        mShader = shader;
    } break;
    case Gfx::ShaderType::PIXEL:
    {
        ComPtr<ID3D11PixelShader> shader;

        result = mDevice->CreatePixelShader(mByteCode.data(), mByteCode.size(), NULL, shader.ReleaseAndGetAddressOf());
        if (FAILED(result))
        {
            gGfxLog.Error(LogMessage("Failed to create Vertex Shader handle."));
            shader = ComPtr<ID3D11PixelShader>();
            return ReportError(false, OperationFailureError, "Shader failed to create handle.", "API - internal");
        }

#if defined(LF_DEBUG)
        String name = GetShaderName();
        shader->SetPrivateData(WKPDID_D3DDebugObjectName, static_cast<UINT>(name.Size()), name.CStr());
#endif
        mShader = shader;

    } break;
    default:
        CriticalAssertMsg("DX11GfxShaderAdapter::Compile was supplied an invalid shader type.");
        break;

    }
    return APIResult<bool>(true);
}

String DX11GfxShaderAdapter::GetShaderName() const
{
    return String();
}


DX11GfxShader::DX11GfxShader()
: Super()
{
}

bool DX11GfxShader::GenerateText(String& outText, Gfx::ShaderType shaderType, const TVector<Token>& defines) const
{
    if (!mParsedFile.GetParseError().Empty())
    {
        return false;
    }
    mParsedFile.GenerateText(outText, shaderType, defines);
    return true;
}
bool DX11GfxShader::GenerateText(Gfx::ShaderTextInfo& outText, Gfx::ShaderType shaderType, const TVector<Token>& defines) const
{
    if (!mParsedFile.GetParseError().Empty())
    {
        return false;
    }
    mParsedFile.GenerateText(outText, shaderType, defines);
    return true;
}

APIResult<bool> DX11GfxShader::Compile(Gfx::ShaderType shaderType, const String& text)
{
    return GetAdapterAs<DX11GfxShaderAdapter>()->CreateShaderFromText(shaderType, text);
}

APIResult<bool> DX11GfxShader::LoadFromBinary(Gfx::ShaderType shaderType, const TVector<ByteT>& buffer)
{
    return LoadFromBinary(shaderType, buffer.data(), buffer.size());
}

APIResult<bool> DX11GfxShader::LoadFromBinary(Gfx::ShaderType shaderType, const ByteT* memory, SizeT numBytes)
{
    (shaderType);
    (memory);
    (numBytes);
    return APIResult<bool>(false);
}

void DX11GfxShader::SetText(const String& value)
{
    mText = value;
    CriticalAssert(!mText.CopyOnWrite());
    mParsedFile.ParseText(mText);
}
const String& DX11GfxShader::GetText() const
{
    return mText;
}
#endif
}
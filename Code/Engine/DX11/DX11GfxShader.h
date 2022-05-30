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
#include "AbstractEngine/Gfx/GfxShader.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "Engine/DX11/DX11Common.h"

namespace lf {

#if defined(LF_DIRECTX11)
class LF_ENGINE_API DX11GfxShaderAdapter : public GfxShaderAdapter
{
    DECLARE_CLASS(DX11GfxShaderAdapter, GfxShaderAdapter);
public:
    DX11GfxShaderAdapter();
    virtual ~DX11GfxShaderAdapter() = default;

    void OnInitialize(GfxDependencyContext& context) override;
    void OnShutdown() override;

    APIResult<bool> CreateShaderFromText(Gfx::ShaderType shaderType, const String& text);
    APIResult<bool> CreateShaderFromBytes(Gfx::ShaderType shaderType, const ByteT* memory, SizeT numBytes);

private:
    APIResult<bool> CreateShader();
    String GetShaderName() const;

    ComPtr<IUnknown> mShader;
    TVector<ByteT> mByteCode;
    Gfx::ShaderType mShaderType;

    ComPtr<ID3D11Device>        mDevice;
    ComPtr<ID3D11DeviceContext> mDeviceContext;
};

class DX11GfxShader : public GfxShader
{
    DECLARE_CLASS(DX11GfxShader, GfxShader);
public:
    DX11GfxShader();
    virtual ~DX11GfxShader() = default;

    bool GenerateText(String& outText, Gfx::ShaderType shaderType, const TVector<Token>& defines) const override;
    bool GenerateText(Gfx::ShaderTextInfo& outText, Gfx::ShaderType shaderType, const TVector<Token>& defines) const override;

    APIResult<bool> Compile(Gfx::ShaderType shaderType, const String& text) override;
    APIResult<bool> LoadFromBinary(Gfx::ShaderType shaderType, const TVector<ByteT>& buffer) override;
    APIResult<bool> LoadFromBinary(Gfx::ShaderType shaderType, const ByteT* memory, SizeT numBytes) override;

    void SetText(const String& value) override;
    const String& GetText() const override;
private:

    String          mText;
    GfxShaderFile   mParsedFile;
};
#endif
}
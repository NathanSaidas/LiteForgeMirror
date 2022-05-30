
#include "Engine/PCH.h"
#include "DX12GfxShaderCompiler.h"
#include "Engine/DX12/DX12Common.h"
#include <dxcapi.h>

namespace lf {

bool DX12GfxShaderCompiler::Compile(Gfx::ShaderType shaderType, const String& text, const TVector<Token>& defines, MemoryBuffer& outBuffer)
{
    ComPtr<IDxcLibrary> library;
    HRESULT hr = DxcCreateInstance(CLSID_DxcLibrary, IID_PPV_ARGS(&library));
    if (FAILED(hr))
    {
        return false;
    }

    ComPtr<IDxcCompiler> compiler;
    hr = DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler));
    if (FAILED(hr))
    {
        return false;
    }

    UINT32 codePage = CP_UTF8;
    ComPtr<IDxcBlobEncoding> sourceBlob;
    hr = library->CreateBlobWithEncodingOnHeapCopy(text.CStr(), static_cast<UINT32>(text.Size()), codePage, sourceBlob.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return false;
    }

    TVector<WString> wsDefines;
    TVector<DxcDefine> dxDefines;
    wsDefines.resize(defines.size());
    dxDefines.resize(defines.size());
    for (SizeT i = 0; i < defines.size(); ++i)
    {
        wsDefines[i] = StrConvert(String(defines[i].Size(), defines[i].CStr(), COPY_ON_WRITE));
        dxDefines[i].Name = wsDefines[i].CStr();
        dxDefines[i].Value = L"";
    }

    const wchar_t* ENTRY_POINTS[] = {
        L"VSMain",
        L"PSMain"
    };
    const wchar_t* PROFILES[] = {
        L"vs_6_0",
        L"ps_6_0"
    };

    ComPtr<IDxcOperationResult> result;
    hr = compiler->Compile(
        sourceBlob.Get(),
        L"Shader.hlsl",
        ENTRY_POINTS[EnumValue(shaderType)],
        PROFILES[EnumValue(shaderType)],
        NULL, 0,
        dxDefines.data(), static_cast<UINT32>(dxDefines.size()),
        NULL,
        result.ReleaseAndGetAddressOf());

    if (SUCCEEDED(hr))
    {
        result->GetStatus(&hr);
    }
    if (FAILED(hr))
    {
        if (result)
        {
            ComPtr<IDxcBlobEncoding> errorsBlob;
            hr = result->GetErrorBuffer(&errorsBlob);
            if (SUCCEEDED(hr) && errorsBlob)
            {
                BOOL encodingKnown;
                UINT32 encoding;
                hr = errorsBlob->GetEncoding(&encodingKnown, &encoding);
                if (SUCCEEDED(hr))
                {
                    if (encodingKnown && encoding == CP_UTF8)
                    {
                        String errorText(static_cast<SizeT>(errorsBlob->GetBufferSize()), reinterpret_cast<const char*>(errorsBlob->GetBufferPointer()), COPY_ON_WRITE);
                        LF_DEBUG_BREAK;
                    }
                }
                // errorMessage from here...
            }
        }
        return false;
    }

    ComPtr<IDxcBlob> code;
    hr = result->GetResult(code.ReleaseAndGetAddressOf());
    if (FAILED(hr))
    {
        return false;
    }

    outBuffer.Allocate(code->GetBufferSize(), 1);
    outBuffer.SetSize(code->GetBufferSize());
    memcpy(outBuffer.GetData(), code->GetBufferPointer(), code->GetBufferSize());
    return true;
}

} // namespace lf
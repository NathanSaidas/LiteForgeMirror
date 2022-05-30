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
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/SpinLock.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "Engine/DX11/DX11Common.h"
#include "Engine/DX11/DX11GfxFactory.h"

namespace lf {
#if defined(LF_DIRECTX11)

DECLARE_ATOMIC_WPTR(GfxObject);
DECLARE_ATOMIC_PTR(GfxAdapter);
DECLARE_ATOMIC_WPTR(GfxMaterialAdapter);
DECLARE_ATOMIC_PTR(GfxMaterialAdapter);
DECLARE_ASSET_TYPE(GfxObject);
DECLARE_ASSET(GfxShaderText);
DECLARE_ASSET(GfxShaderBinaryInfo);
DECLARE_ASSET(GfxShaderBinaryData);
DECLARE_ASSET_TYPE(GfxShaderBinaryData);


class LF_ENGINE_API DX11GfxDevice : public GfxDevice
{
    DECLARE_CLASS(DX11GfxDevice, GfxDevice);
public:
    using Super = GfxDevice;

    DX11GfxDevice();
    virtual ~DX11GfxDevice();

    // Service API:
    APIResult<ServiceResult::Value> OnStart() override;
    APIResult<ServiceResult::Value> OnFrameUpdate() override;
    APIResult<ServiceResult::Value> OnShutdown(ServiceShutdownMode mode) override;

    // GfxDevice API:

    bool Initialize(GfxDeviceFlagsBitfield flags) override;
    void Shutdown() override;

    void Update() override;


    APIResult<GfxWindowAdapterAtomicPtr> CreateWindow(const String& title, SizeT width, SizeT height, const String& classID = EMPTY_STRING) override;

    bool IsDebug() const override;

    APIResult<GfxObjectAtomicPtr> CreateObject(const Type* baseType) override;

    APIResult<bool> CreateAdapter(GfxObject* object) override;
    APIResult<GfxAdapterAtomicPtr> CreateAdapter(const Type* baseType);

    APIResult<Gfx::ResourcePtr> CreateVertexBuffer(SizeT numElements, SizeT stride, Gfx::BufferUsage usage, const void* initialData = nullptr, SizeT initialDataSize = 0) override;
    APIResult<Gfx::ResourcePtr> CreateIndexBuffer(SizeT numElements, Gfx::IndexStride stride, Gfx::BufferUsage usage, const void* initialData = nullptr, SizeT initialDataSize = 0) override;

    APIResult<bool> CopyVertexBuffer(Gfx::Resource* vertexBuffer, SizeT numElements, SizeT stride, const void* vertexData) override;
    APIResult<bool> CopyIndexBuffer(Gfx::Resource* indexBuffer, SizeT numElements, Gfx::IndexStride stride, const void* indexData) override;

    void BeginFrame(const GfxWindowAdapterAtomicWPtr& window) override;
    void EndFrame() override;
    
    void BindPipelineState(GfxMaterialAdapter* adapter) override;
    void UnbindPipelineState() override;

    void BindVertexBuffer(Gfx::Resource* vertexBuffer) override;
    void UnbindVertexBuffer() override;

    void BindIndexBuffer(Gfx::Resource* indexBuffer);
    void UnbindIndexBuffer();

    void SetViewport(Float32 x, Float32 y, Float32 width, Float32 height, Float32 minDepth, Float32 maxDepth) override;
    void SetScissorRect(Float32 x, Float32 y, Float32 width, Float32 height) override;

    void ClearColor(const Color& color) override;
    void ClearDepth() override;
    void SwapBuffers() override;

    void Draw(SizeT vertexCount);
    void DrawIndexed(SizeT indexCount);

    bool  QueryMappedTypes(const Type* baseType, const Type** implementation, const Type** adapter) override;

    APIResult<Gfx::ResourcePtr> CompileAndLoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines) override;
    APIResult<Gfx::ResourcePtr> LoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines) override;

    // ShaderOpPromise CompileAndLoadShader()
private:


    DX11GfxFactory mFactory;

    ComPtr<ID3D11Device>        mDevice;
    ComPtr<ID3D11DeviceContext> mDeviceContext;

    GfxWindowAdapterAtomicPtr   mCurrentWindow;
    GfxMaterialAdapterAtomicPtr mCurrentMaterial;

    struct ShaderInfo
    {
        GfxShaderBinaryInfoAsset     mInfo;
        GfxShaderBinaryDataAssetType mData;
        Gfx::ResourcePtr             mResourceHandle;
    };
    using ShaderInfoMap = TMap<Gfx::ShaderHash, ShaderInfo>;
    using ShaderMap = TMap<AssetPath, ShaderInfoMap>;

    ShaderInfo* CompileShaderInfo(Gfx::ShaderHash hash, Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines);
    String GenerateShaderText(Gfx::ShaderType shaderType, const GfxShaderTextAsset& text, const TVector<Token>& defines);
    bool CompileBinary(Gfx::ShaderType shaderType, const String& text, MemoryBuffer& buffer);
    Gfx::ResourcePtr LoadShader(ShaderInfo& shaderInfo);
    ShaderMap mShaders;

};

#endif

} // namespace lf

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
#include "AbstractEngine/Gfx/GfxCommandContext.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
class Color;
class DX12GfxResourceHeap;
DECLARE_ATOMIC_PTR(GfxUploadBuffer);

class LF_ENGINE_API DX12GfxCommandContext : public GfxCommandContext
{
    DECLARE_CLASS(DX12GfxCommandContext, GfxCommandContext);
public:
    DX12GfxCommandContext();

    bool Initialize(GfxDependencyContext& context) override;
    void Release() override;

    void BeginRecord(Gfx::FrameCountType currentFrame) override;
    void EndRecord() override;
    void SetRenderTarget(GfxSwapChain* target, SizeT frame) override;
    void BindRenderTarget(GfxRenderTexture* target) override;
    void UnbindRenderTarget(GfxRenderTexture* target) override;
    void SetPresentSwapChainState(GfxSwapChain* target, SizeT frame) override;
    void SetPipelineState(const GfxPipelineState* state) override;
    void CopyDataImpl(GfxUploadBufferAtomicPtr& buffer, Gfx::UploadBufferType uploadBufferType, const ByteT* data, SizeT dataByteCount) override;

    void SetViewport(const ViewportF& viewport) override;
    void SetScissorRect(const RectI& rect) override;
    void ClearColor(GfxSwapChain* target, SizeT frame, const Color& color) override;
    void ClearDepth(float value) override;
    void ClearColor(const GfxRenderTexture* texture, const Color& color) override;

    // NOTE: The index referred to here is the 'root paramter' index. (It's based off the pipeline state)
    void SetTexture(Gfx::ShaderParamID index, const GfxTexture* texture) override;
    // NOTE: The index referred to here is the 'root paramter' index. (It's based off the pipeline state)
    void SetConstantBuffer(Gfx::ShaderParamID index, const GfxUploadBuffer* constantBuffer) override;
    void SetStructureBuffer(Gfx::ShaderParamID index, const GfxUploadBuffer* structureBuffer) override;
    void SetVertexBuffer(const GfxVertexBuffer* vertexBuffer) override;
    void SetVertexBuffers(SizeT startIndex, SizeT numBuffers, const GfxVertexBuffer** vertexBuffers) override;
    void SetIndexBuffer(const GfxIndexBuffer* indexBuffer) override;
    void SetTopology(Gfx::RenderMode topology) override;
    void Draw(SizeT vertexCount, SizeT vertexOffset = 0) override;
    void DrawIndexed(SizeT indexCount, SizeT indexOffset = 0, SizeT vertexOffset = 0) override;


    ComPtr<ID3D12GraphicsCommandList> CommandList() { return mCommandList; }
    ComPtr<ID3D12CommandAllocator> Allocator() { return mAllocator; }
private:
    GfxDevice* mDevice;
    ComPtr<ID3D12GraphicsCommandList> mCommandList;
    ComPtr<ID3D12CommandAllocator> mAllocator;
    Gfx::FrameCountType mCurrentFrame;
    DX12GfxResourceHeap* mResourceHeap;

    const GfxPipelineState* mCurrentPSO;

};

} // namespace lf
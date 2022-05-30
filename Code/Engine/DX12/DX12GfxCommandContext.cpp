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
#include "Engine/PCH.h"
#include "DX12GfxCommandContext.h"
#include "Core/Reflection/DynamicCast.h"
#include "Core/Math/Color.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include "Engine/DX12/DX12GfxTexture.h"
#include "Engine/DX12/DX12GfxRenderTexture.h"
#include "Engine/DX12/DX12GfxVertexBuffer.h"
#include "Engine/DX12/DX12GfxIndexBuffer.h"
#include "Engine/DX12/DX12GfxUploadBuffer.h"
#include "Engine/DX12/DX12GfxSwapChain.h"
#include "Engine/DX12/DX12GfxPipelineState.h"
#include "Engine/DX12/DX12GfxResourceHeap.h"
#include <d3dx12.h>

namespace lf {

DEFINE_CLASS(lf::DX12GfxCommandContext) { NO_REFLECTION; }

DX12GfxCommandContext::DX12GfxCommandContext()
: Super()
, mDevice(nullptr)
, mCommandList()
, mAllocator()
, mCurrentFrame(0)
, mResourceHeap(nullptr)
{}

bool DX12GfxCommandContext::Initialize(GfxDependencyContext& context)
{
    if (!Super::Initialize(context))
    {
        return false;
    }
    DX12GfxDependencyContext* dx12 = DynamicCast<DX12GfxDependencyContext>(&context);
    if (!dx12)
    {
        return false;
    }

    ComPtr<ID3D12Device> device = dx12->GetDevice();
    if (!device)
    {
        return false;
    }

    HRESULT hr = device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mAllocator));
    if (FAILED(hr))
    {
        return false;
    }

    hr = device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList));
    mResourceHeap = dx12->GetResourceHeap();
    mCommandList->Close();
    mDevice = context.GetGfxDevice();

    return true;
}
void DX12GfxCommandContext::Release()
{
    mAllocator.Reset();
    mCommandList.Reset();
}

void DX12GfxCommandContext::BeginRecord(Gfx::FrameCountType currentFrame)
{
    Assert(SUCCEEDED(mAllocator->Reset()));
    Assert(SUCCEEDED(mCommandList->Reset(mAllocator.Get(), nullptr)));
    mCurrentFrame = currentFrame;

    ID3D12DescriptorHeap* heaps[] = { mResourceHeap->GetHeap() };
    mCommandList->SetDescriptorHeaps(1, heaps);
}

void DX12GfxCommandContext::EndRecord()
{
    Assert(SUCCEEDED(mCommandList->Close()));
}

void DX12GfxCommandContext::SetRenderTarget(GfxSwapChain* target, SizeT frame)
{
    DX12GfxSwapChain* dx12SwapChain = DynamicCast<DX12GfxSwapChain>(target);
    Assert(dx12SwapChain != nullptr);

    if (dx12SwapChain->IsPresent())
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dx12SwapChain->GetRenderTarget(frame).Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandList->ResourceBarrier(1, &barrier);
        dx12SwapChain->BindForRenderTarget();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = dx12SwapChain->GetRTVDescriptor(frame);

    mCommandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);
}

void DX12GfxCommandContext::BindRenderTarget(GfxRenderTexture* target)
{
    DX12GfxRenderTexture* dx12RenderTexture = DynamicCast<DX12GfxRenderTexture>(target);
    Assert(dx12RenderTexture != nullptr);

    if (dx12RenderTexture->IsTexture())
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dx12RenderTexture->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        mCommandList->ResourceBarrier(1, &barrier);
        dx12RenderTexture->BindAsRenderTarget();
    }

    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = dx12RenderTexture->GetRTV();
    mCommandList->OMSetRenderTargets(1, &rtvDescriptor, false, nullptr);
}

void DX12GfxCommandContext::UnbindRenderTarget(GfxRenderTexture* target)
{
    DX12GfxRenderTexture* dx12RenderTexture = DynamicCast<DX12GfxRenderTexture>(target);
    Assert(dx12RenderTexture != nullptr);

    if (dx12RenderTexture->IsRenderTarget())
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dx12RenderTexture->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        mCommandList->ResourceBarrier(1, &barrier);
        dx12RenderTexture->BindAsTexture();
    }
}

void DX12GfxCommandContext::SetPresentSwapChainState(GfxSwapChain* target, SizeT frame)
{
    DX12GfxSwapChain* dx12SwapChain = DynamicCast<DX12GfxSwapChain>(target);
    Assert(dx12SwapChain != nullptr);

    if (dx12SwapChain->IsRenderTarget())
    {
        D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(dx12SwapChain->GetRenderTarget(frame).Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        mCommandList->ResourceBarrier(1, &barrier);
        dx12SwapChain->BindForPresent();
    }
    target->SetDirty(true);
}

void DX12GfxCommandContext::SetPipelineState(const GfxPipelineState* state)
{
    const DX12GfxPipelineState* dx12State = DynamicCast<DX12GfxPipelineState>(state);
    if (!dx12State)
    {
        return;
    }
    Assert(dx12State->IsGPUReady());

    mCurrentPSO = state;
    mCommandList->SetPipelineState(dx12State->GetPSO().Get());
    mCommandList->SetGraphicsRootSignature(dx12State->GetRootSignature().Get());
}

void DX12GfxCommandContext::CopyDataImpl(GfxUploadBufferAtomicPtr& buffer, Gfx::UploadBufferType uploadBufferType, const ByteT* data, SizeT dataByteCount)
{
    const bool gpuBound = buffer && buffer->IsMapped() && Valid(buffer->GetLastBoundFrame()) && buffer->GetLastBoundFrame() <= mDevice->GetLastCompletedFrame();

    if (!buffer || gpuBound)
    {
        switch (uploadBufferType)
        {
            case Gfx::UploadBufferType::Constant:
            {
                if (buffer)
                {
                    mDevice->ReleaseConstantBuffer(buffer);
                }
                buffer = mDevice->CreateConstantBuffer(dataByteCount);
            } break;
            case Gfx::UploadBufferType::Structured:
            {
                if (buffer)
                {
                    mDevice->ReleaseStructureBuffer(buffer);
                }
                buffer = mDevice->CreateStructureBuffer(dataByteCount);
            } break;
            default:
                CriticalAssertMsg("DX12GfxCommandContext::CopyDataImpl invalid Gfx::UploadBufferType");
                break;
        }
        
    }

    if (!buffer->IsMapped())
    {
        buffer->Commit(*mDevice, *this);
    }
    buffer->CopyData(data, dataByteCount);
}

void DX12GfxCommandContext::SetViewport(const ViewportF& viewport)
{
    // Its flipped for us
    D3D12_VIEWPORT dx12Viewport;
    dx12Viewport.TopLeftX = viewport.left;
    dx12Viewport.TopLeftY = viewport.bottom;
    dx12Viewport.Width = Abs(viewport.right - viewport.left);
    dx12Viewport.Height = Abs(viewport.top - viewport.bottom);
    dx12Viewport.MinDepth = viewport.near;
    dx12Viewport.MaxDepth = viewport.far;
    mCommandList->RSSetViewports(1, &dx12Viewport);
}

void DX12GfxCommandContext::SetScissorRect(const RectI& rect)
{
    // Flipped
    D3D12_RECT scissor;
    scissor.left = rect.left;
    scissor.right = rect.right;
    scissor.top = rect.bottom;
    scissor.bottom = rect.top;

    mCommandList->RSSetScissorRects(1, &scissor);
}

void DX12GfxCommandContext::ClearColor(GfxSwapChain* target, SizeT frame, const Color& color)
{
    DX12GfxSwapChain* dx12SwapChain = DynamicCast<DX12GfxSwapChain>(target);
    Assert(dx12SwapChain != nullptr);
    D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = dx12SwapChain->GetRTVDescriptor(frame);
    Assert(rtvDescriptor.ptr != 0);
    if (rtvDescriptor.ptr != 0)
    {
        FLOAT colorValue[] = { color.r, color.g, color.b, color.a };
        D3D12_RECT clearRect;
        clearRect.left = 0;
        clearRect.right = static_cast<LONG>(dx12SwapChain->GetWidth());
        clearRect.top = 0;
        clearRect.bottom = static_cast<LONG>(dx12SwapChain->GetHeight());
        mCommandList->ClearRenderTargetView(rtvDescriptor, colorValue, 1, &clearRect);
    }
}

void DX12GfxCommandContext::ClearDepth(float value)
{
    (value);
}

void DX12GfxCommandContext::ClearColor(const GfxRenderTexture* texture, const Color& color)
{
    const DX12GfxRenderTexture* dx12RenderTexture = DynamicCast<DX12GfxRenderTexture>(texture);
    Assert(dx12RenderTexture != nullptr);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = dx12RenderTexture->GetRTV();
    FLOAT colorValue[] = { color.r, color.g, color.b, color.a };
    D3D12_RECT clearRect;
    clearRect.left = 0;
    clearRect.right = static_cast<LONG>(dx12RenderTexture->GetWidth());
    clearRect.top = 0;
    clearRect.bottom = static_cast<LONG>(dx12RenderTexture->GetHeight());
    mCommandList->ClearRenderTargetView(rtv, colorValue, 1, &clearRect);
}

void DX12GfxCommandContext::SetTexture(Gfx::ShaderParamID index, const GfxTexture* texture)
{
    if (!texture || !index.IsValid())
    {
        return;
    }

    if (!index.IsTexture2D())
    {
        ReportBugMsg("GfxCommandContext::SetTexture invalid argument 'index'. The index must point to a texture resource slot.");
        return;
    }
    const DX12GfxTexture* dx12Texture = DynamicCast<DX12GfxTexture>(texture);
    const DX12GfxRenderTexture* dx12RenderTexture = DynamicCast<DX12GfxRenderTexture>(texture);
    Assert(dx12Texture != nullptr || dx12RenderTexture != nullptr);

    if (dx12Texture)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE textureGPUHandle;
        if (dx12Texture->Bind(mCurrentFrame, textureGPUHandle))
        {
            mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(index.mID), textureGPUHandle);
        }
    }
    else if(dx12RenderTexture)
    {
        D3D12_GPU_DESCRIPTOR_HANDLE textureGPUHandle;
        if (dx12RenderTexture->Bind(mCurrentFrame, textureGPUHandle))
        {
            mCommandList->SetGraphicsRootDescriptorTable(static_cast<UINT>(index.mID), textureGPUHandle);
        }
    }

    
}

void DX12GfxCommandContext::SetConstantBuffer(Gfx::ShaderParamID index, const GfxUploadBuffer* constantBuffer)
{
    if (!constantBuffer || !index.IsValid())
    {
        return;
    }
    if (!constantBuffer->IsConstantBuffer())
    {
        ReportBugMsg("GfxCommandContext::SetConstantBuffer invalid argument 'constantBuffer'. The uploadBuffer supplied must be a constant buffer");
        return;
    }
    if (!index.IsConstantBuffer())
    {
        ReportBugMsg("GfxCommandContext::SetConstantBuffer invalid argument 'index'. The index must point to a constant buffer slot.");
        return;
    }

    const DX12GfxUploadBuffer* dx12Buffer = DynamicCast<DX12GfxUploadBuffer>(constantBuffer);
    Assert(dx12Buffer != nullptr);

    D3D12_GPU_VIRTUAL_ADDRESS bufferHandle;
    if (dx12Buffer->Bind(mCurrentFrame, bufferHandle))
    {
        mCommandList->SetGraphicsRootConstantBufferView(static_cast<UINT>(index.mID), bufferHandle);
    }
}

void DX12GfxCommandContext::SetStructureBuffer(Gfx::ShaderParamID index, const GfxUploadBuffer* structuredBuffer)
{
    if (!structuredBuffer || !index.IsValid())
    {
        return;
    }
    if (!structuredBuffer->IsStructuredBuffer())
    {
        ReportBugMsg("GfxCommandContext::SetStructureBuffer invalid argument 'structuredBuffer'. The uploadBuffer supplied must be a structured buffer");
        return;
    }

    if (!index.IsStructuredBuffer())
    {
        ReportBugMsg("GfxCommandContext::SetConstantBuffer invalid argument 'index'. The index must point to a structured buffer slot.");
        return;
    }

    const DX12GfxUploadBuffer* dx12Buffer = DynamicCast<DX12GfxUploadBuffer>(structuredBuffer);
    Assert(dx12Buffer != nullptr);

    D3D12_GPU_VIRTUAL_ADDRESS bufferHandle;
    if (dx12Buffer->Bind(mCurrentFrame, bufferHandle))
    {
        mCommandList->SetGraphicsRootShaderResourceView(static_cast<UINT>(index.mID), bufferHandle);
    }
}

void DX12GfxCommandContext::SetVertexBuffer(const GfxVertexBuffer* vertexBuffer)
{
    const GfxVertexBuffer* buffers[] = { vertexBuffer };
    SetVertexBuffers(0, 1, buffers);
}

void DX12GfxCommandContext::SetVertexBuffers(SizeT startIndex, SizeT numBuffers, const GfxVertexBuffer** vertexBuffers)
{
    TStackVector<D3D12_VERTEX_BUFFER_VIEW, 8> dx12Buffers;
    for (SizeT i = 0; i < numBuffers; ++i)
    {
        const DX12GfxVertexBuffer* dx12Buffer = DynamicCast<DX12GfxVertexBuffer>(vertexBuffers[i]);
        if (!dx12Buffer)
        {
            return;
        }
        Assert(vertexBuffers[i]->IsGPUReady());
        const D3D12_VERTEX_BUFFER_VIEW& view = dx12Buffer->GetView();
        Assert(view.BufferLocation != 0);
        Assert(view.SizeInBytes != 0);
        Assert(view.StrideInBytes != 0);
        dx12Buffers.push_back(view);
    }
    mCommandList->IASetVertexBuffers(static_cast<UINT>(startIndex), static_cast<UINT>(numBuffers), dx12Buffers.data());
}

void DX12GfxCommandContext::SetIndexBuffer(const GfxIndexBuffer* indexBuffer)
{
    const DX12GfxIndexBuffer* dx12Buffer = DynamicCast<DX12GfxIndexBuffer>(indexBuffer);
    if (!dx12Buffer)
    {
        return;
    }
    Assert(indexBuffer->IsGPUReady());
    mCommandList->IASetIndexBuffer(&dx12Buffer->GetView());
}

void DX12GfxCommandContext::SetTopology(Gfx::RenderMode topology)
{
    mCommandList->IASetPrimitiveTopology(Gfx::ToTopology(topology));
}

void DX12GfxCommandContext::Draw(SizeT vertexCount, SizeT vertexOffset)
{
    mCommandList->DrawInstanced(static_cast<UINT>(vertexCount), 1, static_cast<UINT>(vertexOffset), 0);
}

void DX12GfxCommandContext::DrawIndexed(SizeT indexCount, SizeT indexOffset, SizeT vertexOffset)
{
    mCommandList->DrawIndexedInstanced(static_cast<UINT>(indexCount), 1, static_cast<UINT>(indexOffset), static_cast<UINT>(vertexOffset), 0);
}

} // namespace lf 
#pragma once

#include "AbstractEngine/Gfx/GfxRenderTexture.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
class DX12GfxResourceHeap;

class LF_ENGINE_API DX12GfxRenderTexture : public GfxRenderTexture
{
    DECLARE_CLASS(DX12GfxRenderTexture, GfxRenderTexture);
public:
    DX12GfxRenderTexture();

    bool Initialize(GfxDependencyContext& context) override;
    void Release() override;
    void Commit(GfxDevice& device, GfxCommandContext& context) override;

    bool IsRenderTarget() const { return mResourceState == D3D12_RESOURCE_STATE_RENDER_TARGET; }
    bool IsTexture() const { return mResourceState == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; }

    void BindAsRenderTarget() { mResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET; }
    void BindAsTexture() { mResourceState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; }

    bool Bind(Gfx::FrameCountType frame, D3D12_GPU_DESCRIPTOR_HANDLE& outHandle) const;

    ID3D12Resource* GetResource() { return mTexture.Get(); }

    D3D12_CPU_DESCRIPTOR_HANDLE GetSRV() const { return mSRV.mCPUHandle; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTV() const { return mRTV; }
private:
    Gfx::DescriptorView mSRV;
    D3D12_CPU_DESCRIPTOR_HANDLE mRTV;

    ComPtr<ID3D12Resource> mTexture;
    ComPtr<ID3D12DescriptorHeap> mRTVHeap;
    DX12GfxResourceHeap*   mResourceHeap;
    mutable Gfx::FrameCountType    mLastBoundFrame;
    D3D12_RESOURCE_STATES  mResourceState;
};

} // namespace lf
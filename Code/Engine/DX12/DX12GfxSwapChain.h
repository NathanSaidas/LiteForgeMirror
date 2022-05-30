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
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxSwapChain.h"
#include "AbstractEngine/App/Win32Window.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
DECLARE_ATOMIC_PTR(Win32Window);

class LF_ENGINE_API DX12GfxSwapChain : public GfxSwapChain
{
    DECLARE_CLASS(DX12GfxSwapChain, GfxSwapChain);
public:
    DX12GfxSwapChain();

    bool InitializeSwapChain(GfxDependencyContext& context, const AppWindowAtomicPtr& window) override;
    void Release() override;
    SizeT GetCurrentFrame() override;
    void Present() override;
    void SetDirty(bool value) override;
    bool IsDirty() override;


    bool IsRenderTarget() const { return mResourceState == D3D12_RESOURCE_STATE_RENDER_TARGET; }
    bool IsPresent() const { return mResourceState == D3D12_RESOURCE_STATE_PRESENT; }

    void BindForRenderTarget() { mResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET; }
    void BindForPresent() { mResourceState = D3D12_RESOURCE_STATE_PRESENT; }

    SizeT GetWidth() const { return mWidth; }
    SizeT GetHeight() const { return mHeight; }
    D3D12_CPU_DESCRIPTOR_HANDLE GetRTVDescriptor(SizeT frame);
    ComPtr<ID3D12Resource> GetRenderTarget(SizeT frame);
private:
    bool CreateSwapChain();

    SizeT mWidth;
    SizeT mHeight;
    Win32WindowAtomicPtr mWindow;
    Atomic32 mDirty;

    ComPtr<ID3D12Device> mDevice;
    ComPtr<IDXGIFactory4> mDeviceFactory;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<IDXGISwapChain3> mSwapChain;
    ComPtr<ID3D12Resource>  mRenderTargets[Gfx::FrameCount::Value];
    ComPtr<ID3D12DescriptorHeap> mRTVHeap;
    UINT                         mRTVDescriptorSize;
    D3D12_RESOURCE_STATES        mResourceState;

};

} // namespace lf
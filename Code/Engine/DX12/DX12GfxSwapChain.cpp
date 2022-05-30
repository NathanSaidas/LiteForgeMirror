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
#include "DX12GfxSwapChain.h"
#include "Core/Reflection/DynamicCast.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include <d3dx12.h>

namespace lf {

DEFINE_CLASS(lf::DX12GfxSwapChain) { NO_REFLECTION; }

DX12GfxSwapChain::DX12GfxSwapChain()
: Super()
, mWidth(0)
, mHeight(0)
, mWindow()
, mDirty(0)
, mDevice()
, mDeviceFactory()
, mCommandQueue()
, mSwapChain()
, mRenderTargets()
, mRTVHeap()
, mRTVDescriptorSize(0)
, mResourceState(D3D12_RESOURCE_STATE_PRESENT)
{}

bool DX12GfxSwapChain::InitializeSwapChain(GfxDependencyContext& context, const AppWindowAtomicPtr& window)
{
    ReportBug(mWindow == NULL_PTR); // SwapChain was not released?
    if (mWindow)
    {
        return false;
    }

    DX12GfxDependencyContext* dx12 = DynamicCast<DX12GfxDependencyContext>(&context);
    if (!dx12)
    {
        return false;
    }

    mWindow = DynamicCast<Win32WindowAtomicPtr>(window);
    if (!mWindow)
    {
        Release();
        return false;
    }

    mDevice = dx12->GetDevice();
    mDeviceFactory = dx12->GetDeviceFactory();
    mCommandQueue = dx12->GetCommandQueue();
    if (!mDevice || !mDeviceFactory || !mCommandQueue)
    {
        Release();
        return false;
    }

    if (!CreateSwapChain())
    {
        return false;
    }
    return true;
}

void DX12GfxSwapChain::Release()
{
    mWidth = 0;
    mHeight = 0;
    mWindow.Release();

    for (SizeT i = 0; i < Gfx::FrameCount::Value; ++i)
    {
        mRenderTargets[i].Reset();
    }
    mRTVHeap.Reset();
    mRTVDescriptorSize = 0;
    mSwapChain.Reset();
    mCommandQueue.Reset();
    mDeviceFactory.Reset();
    mDevice.Reset();
}

SizeT DX12GfxSwapChain::GetCurrentFrame()
{
    if (mSwapChain)
    {
        return static_cast<SizeT>(mSwapChain->GetCurrentBackBufferIndex());
    }
    return INVALID;
}

void DX12GfxSwapChain::Present()
{
    if (mSwapChain)
    {
        mSwapChain->Present(0, 0);
    }
}

void DX12GfxSwapChain::SetDirty(bool value)
{
    AtomicStore(&mDirty, value ? 1 : 0);
}

bool DX12GfxSwapChain::IsDirty()
{
    return AtomicLoad(&mDirty) != 0;
}

bool DX12GfxSwapChain::CreateSwapChain()
{
    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
    swapChainDesc.BufferCount = Gfx::FrameCount::Value;
    swapChainDesc.Width = static_cast<UINT>(mWindow->GetWidth());
    swapChainDesc.Height = static_cast<UINT>(mWindow->GetHeight());;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> tempSwapChain;
    if ((FAILED(mDeviceFactory->CreateSwapChainForHwnd(
        mCommandQueue.Get(),
        mWindow->GetWindowHandle(),
        &swapChainDesc,
        nullptr,
        nullptr,
        &tempSwapChain))))
    {
        return false;
    }

    if (FAILED(tempSwapChain.As(&mSwapChain)))
    {
        return false;
    }

    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors = Gfx::FrameCount::Value;
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (FAILED(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRTVHeap))))
    {
        return false;
    }

    // D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
    // dsvHeapDesc.NumDescriptors = NumFrames; // was 1 + FrameCount * 1;
    // dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    // dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    // if (FAILED(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mGame.mDSVHeap))))
    // {
    //     return;
    // }

    mRTVDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    for (SizeT i = 0; i < Gfx::FrameCount::Value; ++i)
    {
        if (FAILED(mSwapChain->GetBuffer(static_cast<UINT>(i), IID_PPV_ARGS(&mRenderTargets[i]))))
        {
            return false;
        }

        mDevice->CreateRenderTargetView(mRenderTargets[i].Get(), nullptr, rtvHandle);
        rtvHandle.Offset(1, mRTVDescriptorSize);

        mRenderTargets[i]->SetName(L"Render Window");
    }
    
    mWidth = mWindow->GetWidth();
    mHeight = mWindow->GetHeight();

    return true;
}

D3D12_CPU_DESCRIPTOR_HANDLE DX12GfxSwapChain::GetRTVDescriptor(SizeT frame)
{
    if (frame >= Gfx::FrameCount::Value)
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    }

    if (!mRTVHeap)
    {
        return D3D12_CPU_DESCRIPTOR_HANDLE{ 0 };
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(static_cast<INT>(frame), mRTVDescriptorSize);
    return rtvHandle;
}

ComPtr<ID3D12Resource> DX12GfxSwapChain::GetRenderTarget(SizeT frame)
{
    if (frame >= Gfx::FrameCount::Value)
    {
        return ComPtr<ID3D12Resource>();
    }

    return mRenderTargets[frame];
}


} // namespace lf
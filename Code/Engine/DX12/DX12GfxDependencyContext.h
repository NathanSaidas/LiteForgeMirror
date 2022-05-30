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
#include "AbstractEngine/Gfx/GfxBase.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
class DX12GfxResourceHeap;

class LF_ENGINE_API DX12GfxDependencyContext : public GfxDependencyContext
{
    DECLARE_CLASS(DX12GfxDependencyContext, GfxDependencyContext);
public:
    DX12GfxDependencyContext() = delete;
    DX12GfxDependencyContext(GfxDevice* gfxDevice
        , ComPtr<ID3D12Device> device
        , ComPtr<IDXGIFactory4> factory
        , ComPtr<ID3D12CommandQueue> commandQueue
        , ComPtr<ID3D12GraphicsCommandList> resourceCommandList
        , DX12GfxResourceHeap* resourceHeap
    );

    ComPtr<ID3D12Device>& GetDevice() { return mDevice; }
    ComPtr<IDXGIFactory4>& GetDeviceFactory() { return mDeviceFactory; }
    ComPtr<ID3D12CommandQueue>& GetCommandQueue() { return mCommandQueue; }
    ComPtr<ID3D12GraphicsCommandList>& GetResourceCommandList() { return mResourceCommandList; }
    DX12GfxResourceHeap* GetResourceHeap() { return mResourceHeap; }
private:
    ComPtr<ID3D12Device>  mDevice;
    ComPtr<IDXGIFactory4> mDeviceFactory;
    ComPtr<ID3D12CommandQueue> mCommandQueue;
    ComPtr<ID3D12GraphicsCommandList> mResourceCommandList;
    DX12GfxResourceHeap* mResourceHeap; // NOTE: Resource heap is bound to the GfxDevice, therefore it will live until the graphics device dies (effectively forever)
};

} // namespace lf
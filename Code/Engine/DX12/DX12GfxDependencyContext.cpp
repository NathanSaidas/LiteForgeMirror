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
#include "DX12GfxDependencyContext.h"

namespace lf {

DEFINE_ABSTRACT_CLASS(lf::DX12GfxDependencyContext) { NO_REFLECTION; }

DX12GfxDependencyContext::DX12GfxDependencyContext(
    GfxDevice* gfxDevice
  , ComPtr<ID3D12Device> device
  , ComPtr<IDXGIFactory4> factory
  , ComPtr<ID3D12CommandQueue> commandQueue
  , ComPtr<ID3D12GraphicsCommandList> resourceCommandList
  , DX12GfxResourceHeap* resourceHeap)
: Super(nullptr, gfxDevice)
, mDevice(std::move(device))
, mDeviceFactory(std::move(factory))
, mCommandQueue(std::move(commandQueue))
, mResourceCommandList(std::move(resourceCommandList))
, mResourceHeap(resourceHeap)
{
    SetType(typeof(DX12GfxDependencyContext));
}

} // namespace lf
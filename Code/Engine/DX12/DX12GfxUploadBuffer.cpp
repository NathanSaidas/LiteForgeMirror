// ********************************************************************
// Copyright (c) 2022 Nathan Hanlan
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
#include "DX12GfxUploadBuffer.h"
#include "Engine/DX12/DX12Util.h"

#include <d3dx12.h>

namespace lf {
DEFINE_CLASS(lf::DX12GfxUploadBuffer) { NO_REFLECTION; }
DX12GfxUploadBuffer::DX12GfxUploadBuffer()
: Super()
, mUploadBuffer()
, mMappedData(nullptr)
, mElementByteSize(0)
, mLastBoundFrame(INVALID)
{}
DX12GfxUploadBuffer::~DX12GfxUploadBuffer()
{}

void DX12GfxUploadBuffer::Commit(GfxDevice& device, GfxCommandContext&)
{
    if (mUploadBuffer)
    {
        ReportBugMsg("Cannot recommit a GfxUploadBuffer you must create a new one!");
        return;
    }

    mElementByteSize = GetElementSize();
    if (IsConstantBuffer())
    {
        mElementByteSize = Gfx::CalcConstantBufferByteSize(mElementByteSize);
    }

    auto dx12 = GetDX12Device(device);
    auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto buffer = CD3DX12_RESOURCE_DESC::Buffer(static_cast<UINT>(mElementByteSize * GetElementCount()));

    HRESULT hr = dx12->CreateCommittedResource(
        &heapProperties,
        D3D12_HEAP_FLAG_NONE,
        &buffer,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mUploadBuffer)
    );

    if (FAILED(hr))
    {
        return;
    }

    hr = mUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mMappedData));
}

void DX12GfxUploadBuffer::Release()
{
    if (mUploadBuffer)
    {
        mUploadBuffer->Unmap(0, nullptr);
    }

    mMappedData = nullptr;
    mUploadBuffer.Reset();
    mElementByteSize = 0;
}

Gfx::FrameCountType DX12GfxUploadBuffer::GetLastBoundFrame() const
{
    return mLastBoundFrame;
}

bool DX12GfxUploadBuffer::Bind(Gfx::FrameCountType frame, D3D12_GPU_VIRTUAL_ADDRESS& outHandle) const
{
    if (!IsMapped())
    {
        return false;
    }
    outHandle = mUploadBuffer->GetGPUVirtualAddress();
    mLastBoundFrame = frame;
    return true;
}

ByteT* DX12GfxUploadBuffer::GetMappedData()
{
    return mMappedData;;
}

SizeT  DX12GfxUploadBuffer::GetElementByteSize() const
{
    return mElementByteSize;
}


} // namespace lf
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
#include "DX12GfxIndexBuffer.h"
#include "AbstractEngine/Gfx/GfxDevice.h"

namespace lf {
DEFINE_CLASS(lf::DX12GfxIndexBuffer) { NO_REFLECTION; }

DX12GfxIndexBuffer::DX12GfxIndexBuffer()
: Super()
, mBuffer()
, mBufferView{0,0, DXGI_FORMAT_UNKNOWN}
, mClientBuffer()
, mLock()
{}
DX12GfxIndexBuffer::~DX12GfxIndexBuffer()
{}

void DX12GfxIndexBuffer::Release()
{
    mBuffer.Release();
    mBufferView.BufferLocation = 0;
    mBufferView.SizeInBytes = 0;
    mBufferView.Format = DXGI_FORMAT_UNKNOWN;
    Super::Release();
    SetGPUReady(false);
    Invalidate();
}
void DX12GfxIndexBuffer::Commit(GfxDevice& device, GfxCommandContext& context)
{
    DX12GfxBuffer::UploadType uploadType;
    switch (GetUsage())
    {
    case Gfx::BufferUsage::STATIC: uploadType = DX12GfxBuffer::UPLOAD_STATIC; break;
    case Gfx::BufferUsage::DYNAMIC: uploadType = DX12GfxBuffer::UPLOAD_FAST_DYNAMIC; break;
    case Gfx::BufferUsage::READ_WRITE: uploadType = DX12GfxBuffer::UPLOAD_DYNAMIC; break;
    default:
        return;
    }

    DXGI_FORMAT format;
    switch (GetStride())
    {
    case 2: format = DXGI_FORMAT_R16_UINT; break;
    case 4: format = DXGI_FORMAT_R32_UINT; break;
    default:
        return;
    }

    mBuffer.SetBufferType(DX12GfxBuffer::BUFFER_TYPE_INDEX);
    mBuffer.SetUploadType(uploadType);
    mBuffer.SetBufferData(mClientBuffer, GetStride(), GetNumElements());
    if (mBuffer.Commit(device, context))
    {
        mBufferView.BufferLocation = mBuffer.GetResource()->GetGPUVirtualAddress();
        mBufferView.Format = format;
        mBufferView.SizeInBytes = static_cast<UINT>(GetNumElements() * GetStride());
        SetGPUReady(true);
    }
    else
    {
        mBufferView.BufferLocation = 0;
        mBufferView.SizeInBytes = 0;
        mBufferView.Format = DXGI_FORMAT_UNKNOWN;
        SetGPUReady(false);
        Invalidate();
    }

    mBuffer.OnResourceDone(); // TODO: Remove 
}

APIResult<bool> DX12GfxIndexBuffer::SetIndices(const MemoryBuffer& indices, SizeT stride, SizeT numElements)
{
    if (indices.GetSize() != stride * numElements)
    {
        return ReportError(false, InvalidArgumentError, "indices|stride|numElements", "DX12GfxIndexBuffer::SetIndices Size mismatch; vertices.GetSize() == stride * numElements");
    }
    ScopeLock lock(mLock);

    if (InvalidEnum(GetUsage()))
    {
        return ReportError(false, OperationFailureError, "Invalid buffer usage, call SetUsage first.", "DX12GfxIndexBuffer::SetIndices");
    }

    if (mBuffer.IsInitialized() && GetUsage() == Gfx::BufferUsage::STATIC)
    {
        return ReportError(false, OperationFailureError, "Cannot update a static buffer.", "DX12GfxIndexBuffer::SetIndices");
    }

    SetStride(stride);
    SetNumElements(numElements);
    mClientBuffer.Copy(indices);
    return APIResult<bool>(true);
}

APIResult<bool> DX12GfxIndexBuffer::SetIndices(MemoryBuffer&& indices, SizeT stride, SizeT numElements)
{
    if (indices.GetSize() != stride * numElements)
    {
        return ReportError(false, InvalidArgumentError, "indices|stride|numElements", "DX12GfxIndexBuffer::SetIndices Size mismatch; vertices.GetSize() == stride * numElements");
    }
    ScopeLock lock(mLock);

    if (InvalidEnum(GetUsage()))
    {
        return ReportError(false, OperationFailureError, "Invalid buffer usage, call SetUsage first.", "DX12GfxIndexBuffer::SetIndices");
    }

    if (mBuffer.IsInitialized() && GetUsage() == Gfx::BufferUsage::STATIC)
    {
        return ReportError(false, OperationFailureError, "Cannot update a static buffer.", "DX12GfxIndexBuffer::SetIndices");
    }

    SetStride(stride);
    SetNumElements(numElements);
    mClientBuffer = std::move(indices);
    return APIResult<bool>(true);
}

} // namespace lf
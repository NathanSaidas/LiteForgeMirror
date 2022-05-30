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
#include "DX12GfxVertexBuffer.h"
#include "AbstractEngine/Gfx/GfxDevice.h"

namespace lf {

DEFINE_CLASS(lf::DX12GfxVertexBuffer) { NO_REFLECTION; }

DX12GfxVertexBuffer::DX12GfxVertexBuffer()
: Super()
, mBuffer()
, mBufferView{0, 0, 0}
, mClientBuffer()
, mLock()
{}
DX12GfxVertexBuffer::~DX12GfxVertexBuffer()
{

}

void DX12GfxVertexBuffer::Release()
{
    mBuffer.Release();
    mBufferView.BufferLocation = 0;
    mBufferView.SizeInBytes = 0;
    mBufferView.StrideInBytes = 0;
    Super::Release();
    SetGPUReady(false);
    Invalidate();
}

void DX12GfxVertexBuffer::Commit(GfxDevice& device, GfxCommandContext& context)
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

    mBuffer.SetBufferType(DX12GfxBuffer::BUFFER_TYPE_VERTEX);
    mBuffer.SetUploadType(uploadType);
    mBuffer.SetBufferData(mClientBuffer, GetStride(), GetNumElements());
    if (mBuffer.Commit(device,context))
    {
        mBufferView.BufferLocation = mBuffer.GetResource()->GetGPUVirtualAddress();
        mBufferView.StrideInBytes = static_cast<UINT>(GetStride());
        mBufferView.SizeInBytes = static_cast<UINT>(GetNumElements() * GetStride());
        SetGPUReady(true);
    }
    else
    {
        mBufferView.BufferLocation = 0;
        mBufferView.SizeInBytes = 0;
        mBufferView.StrideInBytes = 0;
        SetGPUReady(false);
        Invalidate();
    }

    mBuffer.OnResourceDone(); // TODO: Just make this inline
}

APIResult<bool> DX12GfxVertexBuffer::SetVertices(const MemoryBuffer& vertices, SizeT stride, SizeT numElements)
{
    if (vertices.GetSize() != stride * numElements)
    {
        return ReportError(false, InvalidArgumentError, "vertices|stride|numElements", "Size mismatch; vertices.GetSize() == stride * numElements");
    }
    ScopeLock lock(mLock);

    if (InvalidEnum(GetUsage()))
    {
        return ReportError(false, OperationFailureError, "Invalid buffer usage, call SetUsage first.", "DX12GfxVertexBuffer::SetVertices");
    }

    if (mBuffer.IsInitialized() && GetUsage() == Gfx::BufferUsage::STATIC)
    {
        return ReportError(false, OperationFailureError, "Cannot update a static buffer.", "DX12GfxVertexBuffer::SetVertices");
    }

    SetStride(stride);
    SetNumElements(numElements);
    mClientBuffer.Copy(vertices);
    return APIResult<bool>(true);

}
APIResult<bool> DX12GfxVertexBuffer::SetVertices(MemoryBuffer&& vertices, SizeT stride, SizeT numElements)
{
    if (vertices.GetSize() != stride * numElements)
    {
        return ReportError(false, InvalidArgumentError, "vertices|stride|numElements", "Size mismatch; vertices.GetSize() == stride * numElements");
    }
    ScopeLock lock(mLock);

    if (InvalidEnum(GetUsage()))
    {
        return ReportError(false, OperationFailureError, "Invalid buffer usage, call SetUsage first.", "DX12GfxVertexBuffer::SetVertices");
    }

    if (mBuffer.IsInitialized() && GetUsage() == Gfx::BufferUsage::STATIC)
    {
        return ReportError(false, OperationFailureError, "Cannot update a static buffer.", "DX12GfxVertexBuffer::SetVertices");
    }

    SetStride(stride);
    SetNumElements(numElements);
    mClientBuffer = std::move(vertices);
    return APIResult<bool>(true);
}

} // namespace lf
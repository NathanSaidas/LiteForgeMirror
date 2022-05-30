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
#include "DX12GfxBuffer.h"
#include "Core/Reflection/DynamicCast.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include "Engine/DX12/DX12Util.h"
// debug
#include "Core/Utility/Log.h"

#include <d3dx12.h>

namespace lf {

// DirectX notes:

// CreatePlacedResource
// CreateReservedResource
// CreateCommittedResource
// 
// CreatePlacedResource (placed in specific heap, so if we wanted to optimize memory a bit, we could create all our data in a single heap and reference it. This limits us though. )
// CreateCommiittedResource ( crease resource and heap )


// What is a heap?
// 
//     The heap is where we store resources.
// 
// What role does the view play.
//     The view is essentially a pointer to the resource.
// 
// What role does the resource play.
//     The resource is the data stored on the heap. We create a shader resource view
//
//

// RenderFrame:
// 
//      Allocate Buffer via CreateCommittedResource (instant)
//      UploadResources ( command list/async )
//      Create Shader Resource view (in Descriptor Heap)
//      
//      Use SetGraphicsRootConstantBufferView to set constant buffers
//      Use SetGraphicsRootShaderResourceView for StructuredBuffer
//      Use SetGraphicsRootDescriptorTable for Textures

DX12GfxBuffer::DX12GfxBuffer()
: mUploadType(UPLOAD_STATIC)
, mBufferType(BUFFER_TYPE_VERTEX)
, mResource()
, mResourceUpload()
, mDataBuffer()
, mDataWidth(0)
, mDataHeight(0)
, mDataStride(0)
, mState(EnumValue(State::None))
, mLock()
{
}
DX12GfxBuffer::~DX12GfxBuffer()
{

}


void DX12GfxBuffer::Release()
{
    // Because of asynchronous operations, we might need to do a 'release async'...
    mResource.Reset();
    mResourceUpload.Reset();

    mUploadType = UPLOAD_STATIC;
    mBufferType = BUFFER_TYPE_VERTEX;
    mDataBuffer.Free();
    mDataWidth = mDataHeight = mDataStride = 0;
    SetState(State::None);

}

void DX12GfxBuffer::OnResourceDone()
{
    ScopeLock lock(mLock);
    if (GetState() == State::ResourcePending)
    {
        SetState(State::ResourceUploaded);

        // mResourceUpload.Reset();
        mDataBuffer.Free();
        mDataWidth = mDataHeight = mDataStride = 0;
    }

    if (GetState() == State::CommitFailed)
    {
        gGfxLog.Warning(LogMessage("Failed to commit buffer"));
    }
}

APIResult<bool> DX12GfxBuffer::SetUploadType(UploadType value)
{
    ScopeLock lock(mLock);
    if (GetState() != State::None)
    {
        return ReportError(false, OperationFailureError, "Cannot adjust upload type when buffer is already uploading/uploaded.", "DX12GfxBuffer::SetUploadType");
    }
    mUploadType = value;
    return APIResult<bool>(true);
}

APIResult<bool> DX12GfxBuffer::SetBufferType(BufferType value)
{
    ScopeLock lock(mLock);
    if(GetState() != State::None)
    {
        return ReportError(false, OperationFailureError, "Cannot adjust buffer type when buffer is already uploading/uploaded.", "DX12GfxBuffer::SetBufferType");
    }
    mBufferType = value;
    return APIResult<bool>(true);
}

APIResult<bool> DX12GfxBuffer::SetBufferData(MemoryBuffer&& buffer, SizeT vertexStride, SizeT numElements)
{
    ScopeLock lock(mLock);
    if (buffer.GetSize() == 0)
    {
        return ReportError(false, InvalidArgumentError, "buffer", "Buffer cannot be empty.");
    }

    if (vertexStride == 0)
    {
        return ReportError(false, InvalidArgumentError, "vertexStride", "Vertex Stride cannot be 0.");
    }

    if (numElements == 0)
    {
        return ReportError(false, InvalidArgumentError, "numElements", "Num Elements cannot be 0.");
    }

    if (buffer.GetSize() != vertexStride * numElements)
    {
        return ReportError(false, OperationFailureError, "Buffer size must match the vertexStride * numElements, did you forget to call SetSize?", "DX12GfxBuffer::SetBufferData buffer|vertexStride|numElements");
    }

    if (GetBufferType() != BUFFER_TYPE_VERTEX && GetBufferType() != BUFFER_TYPE_INDEX)
    {
        return ReportError(false, OperationFailureError, "Cannot update the buffer data. Wrong buffer type, please select VERTEX or INDEX.", "DX12GfxBuffer::SetBufferData");
    }

    if (!CanUpdateBuffer())
    {
        return ReportError(false, OperationFailureError, "Cannot update the buffer data. Verify the buffer is not STATIC", "DX12GfxBuffer::SetBufferData");
    }

    mDataBuffer = std::move(buffer);
    mDataStride = vertexStride;
    mDataWidth = numElements;
    mDataHeight = 0;
    return APIResult<bool>(true);
}
APIResult<bool> DX12GfxBuffer::SetBufferData(const MemoryBuffer& buffer, SizeT vertexStride, SizeT numElements)
{
    ScopeLock lock(mLock);
    if (buffer.GetSize() == 0)
    {
        return ReportError(false, InvalidArgumentError, "buffer", "Buffer cannot be empty.");
    }

    if (vertexStride == 0)
    {
        return ReportError(false, InvalidArgumentError, "vertexStride", "Vertex Stride cannot be 0.");
    }

    if (numElements == 0)
    {
        return ReportError(false, InvalidArgumentError, "numElements", "Num Elements cannot be 0.");
    }

    if (buffer.GetSize() != vertexStride * numElements)
    {
        return ReportError(false, OperationFailureError, "Buffer size must match the vertexStride * numElements, did you forget to call SetSize?", "DX12GfxBuffer::SetBufferData buffer|vertexStride|numElements");
    }

    if (GetBufferType() != BUFFER_TYPE_VERTEX && GetBufferType() != BUFFER_TYPE_INDEX)
    {
        return ReportError(false, OperationFailureError, "Cannot update the buffer data. Wrong buffer type, please select VERTEX or INDEX.", "DX12GfxBuffer::SetBufferData");
    }

    if (!CanUpdateBuffer())
    {
        return ReportError(false, OperationFailureError, "Cannot update the buffer data. Verify the buffer is not STATIC", "DX12GfxBuffer::SetBufferData");
    }
    mDataBuffer.Copy(buffer);
    mDataStride = vertexStride;
    mDataWidth = numElements;
    mDataHeight = 0;
    return APIResult<bool>(true);
}
APIResult<bool> DX12GfxBuffer::SetTextureData(MemoryBuffer&& buffer, SizeT stride, SizeT width, SizeT height)
{
    ScopeLock lock(mLock);
    if (!CanUpdateBuffer())
    {
        return ReportError(false, OperationFailureError, "Cannot update the buffer data. Verify the buffer is not STATIC", "DX12GfxBuffer::SetBufferData");
    }
    mDataBuffer = std::move(buffer);
    mDataStride = stride;
    mDataWidth = width;
    mDataHeight = height;
    return APIResult<bool>(true);
}
APIResult<bool> DX12GfxBuffer::SetTextureData(const MemoryBuffer& buffer, SizeT stride, SizeT width, SizeT height)
{
    ScopeLock lock(mLock);
    if (!CanUpdateBuffer())
    {
        return ReportError(false, OperationFailureError, "Cannot update the buffer data. Verify the buffer is not STATIC", "DX12GfxBuffer::SetBufferData");
    }
    mDataBuffer.Copy(buffer);
    mDataStride = stride;
    mDataWidth = width;
    mDataHeight = height;
    return APIResult<bool>(true);
}

APIResult<bool> DX12GfxBuffer::Commit(GfxDevice& device, GfxCommandContext& context)
{
    ScopeLock lock(mLock);
    if (mUploadType == UPLOAD_STATIC && GetState() != State::None)
    {
        return ReportError(false, OperationFailureError, "Cannot commit the buffer, the buffer was uploaded with UPLOAD_STATIC", "DX12GfxBuffer::Commit");
    }

    if (GetState() == State::ResourcePending)
    {
        return ReportError(false, OperationFailureError, "Cannot commit the buffer, it has already been commited you must wait for the resource to be uploaded.", "DX12GfxBuffer::Commit");
    }

    switch (mBufferType)
    {
    case BUFFER_TYPE_VERTEX:
    {
        return APIResult<bool>(CreateVertexOrIndexBuffer(device, context));
    } break;
    case BUFFER_TYPE_INDEX:
    {
        return APIResult<bool>(CreateVertexOrIndexBuffer(device, context));
    } break;
    case BUFFER_TYPE_TEXTURE1D:
    {
        ReportBugMsg("Not implemented");
    } break;
    case BUFFER_TYPE_TEXTURE2D:
    {
        ReportBugMsg("Not implemented");
    } break;
    default:
        CriticalAssertMsg("Invalid enum value (DX12GfxBuffer::BufferType mBufferType)");
        break;
    }


    return APIResult<bool>(false);
}

bool DX12GfxBuffer::CanUpdateBuffer() const
{
    if (mBufferType == UPLOAD_STATIC)
    {
        return GetState() == State::None;
    }
    return true;
}

SizeT DX12GfxBuffer::GetBufferSize() const
{
    ReportBug(mBufferType == BUFFER_TYPE_INDEX || mBufferType == BUFFER_TYPE_VERTEX);
    if (mBufferType == BUFFER_TYPE_INDEX || mBufferType == BUFFER_TYPE_VERTEX)
    {
        return mDataStride * mDataWidth;
    }
    return 0;
}
SizeT DX12GfxBuffer::GetBufferStride() const
{
    ReportBug(mBufferType == BUFFER_TYPE_INDEX || mBufferType == BUFFER_TYPE_VERTEX);
    if (mBufferType == BUFFER_TYPE_INDEX || mBufferType == BUFFER_TYPE_VERTEX)
    {
        return mDataStride;
    }
    return 0;
}

bool DX12GfxBuffer::CreateVertexOrIndexBuffer(GfxDevice& device, GfxCommandContext& context)
{
    ReportBug(GetState() != State::ResourcePending);
    if (GetState() == State::ResourcePending)
    {
        return false;
    }

    ComPtr<ID3D12Device> dx12(GetDX12Device(device));
    ComPtr<ID3D12GraphicsCommandList> cmdList(GetDX12GraphicsCommandList(context));

    D3D12_HEAP_PROPERTIES heapProps = { };
    D3D12_RESOURCE_DESC resourceDesc = { };
    HRESULT hr;

    if (GetState() == State::None)
    {
        switch (mUploadType)
        {
            case UPLOAD_STATIC:
            case UPLOAD_FAST_DYNAMIC:
            {
                heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
                resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetBufferSize());
                hr = dx12->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&mResource));
                if (FAILED(hr))
                {
                    SetState(State::CommitFailed);
                    return false;
                }

                heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetBufferSize());
                hr = dx12->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mResourceUpload));
                if (FAILED(hr))
                {
                    SetState(State::CommitFailed);
                    return false;
                }

                D3D12_SUBRESOURCE_DATA vertexData = {};
                vertexData.pData = mDataBuffer.GetData();
                vertexData.RowPitch = GetBufferSize();
                vertexData.SlicePitch = vertexData.RowPitch;

                auto finalResourceState = mBufferType == BufferType::BUFFER_TYPE_VERTEX ? D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER : D3D12_RESOURCE_STATE_INDEX_BUFFER;
                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, finalResourceState);

                if (0 == UpdateSubresources<1>(cmdList.Get(), mResource.Get(), mResourceUpload.Get(), 0, 0, 1, &vertexData))
                {
                    SetState(State::CommitFailed);
                    return false;
                }
                cmdList->ResourceBarrier(1, &barrier);

                SetState(State::ResourcePending);
                return true;
            }
            case UPLOAD_DYNAMIC:
            {
                heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetBufferSize());
                hr = dx12->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mResource));
                if (FAILED(hr))
                {
                    SetState(State::CommitFailed);
                    return false;
                }

                void* resourceBuffer = nullptr;
                hr = mResource->Map(0, nullptr, &resourceBuffer);
                if (FAILED(hr))
                {
                    SetState(State::CommitFailed);
                    return false;
                }
                memcpy(resourceBuffer, mDataBuffer.GetData(), GetBufferSize());
                mResource->Unmap(0, nullptr);

                SetState(State::ResourceUploaded);
                return true;
            }
            default:
                CriticalAssertMsg("Invalid enum value (DX12GfxBuffer::UploadType mUploadType)");
                break;
        }
    }
    else if (GetState() == State::ResourceUploaded)
    {
        switch (mUploadType)
        {
            case UPLOAD_FAST_DYNAMIC:
            {
                ReportBug(mResource != nullptr);
                if (mResourceUpload == nullptr)
                {
                    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
                    resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(GetBufferSize());
                    hr = dx12->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&mResourceUpload));
                    if (FAILED(hr))
                    {
                        SetState(State::CommitFailed);
                        return false;
                    }
                }

                D3D12_SUBRESOURCE_DATA vertexData = {};
                vertexData.pData = mDataBuffer.GetData();
                vertexData.RowPitch = GetBufferSize();
                vertexData.SlicePitch = vertexData.RowPitch;

                D3D12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_COPY_DEST);
                cmdList->ResourceBarrier(1, &barrier);

                if (0 == UpdateSubresources<1>(cmdList.Get(), mResource.Get(), mResourceUpload.Get(), 0, 0, 1, &vertexData))
                {
                    SetState(State::CommitFailed);
                    return false;
                }
                barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
                cmdList->ResourceBarrier(1, &barrier);

                SetState(State::ResourcePending);
                
                // mGfxDevice->OnResourceComplete(GetWeakPointer());
            }
            case UPLOAD_DYNAMIC:
            {
                ReportBug(mResource != nullptr);
                void* resourceBuffer = nullptr;
                hr = mResource->Map(0, nullptr, &resourceBuffer);
                if (FAILED(hr))
                {
                    SetState(State::CommitFailed);
                    return false;
                }
                memcpy(resourceBuffer, mDataBuffer.GetData(), GetBufferSize());
                mResource->Unmap(0, nullptr);
            }
            case UPLOAD_STATIC:
            {
                ReportBugMsg("Invalid upload type for buffer. Cannot create vertex buffer for UPLOAD_STATIC when resource has been uploaded.");
            } break;
            default:
                CriticalAssertMsg("Invalid enum value (DX12GfxBuffer::UploadType mUploadType");
                break;

        }
    }
    return false;
}

} // namespace lf
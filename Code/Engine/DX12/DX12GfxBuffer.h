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
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Utility/APIResult.h"
#include "Core/Platform/SpinLock.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
class GfxDevice;
class GfxDependencyContext;
class GfxCommandContext;

class DX12GfxBuffer
{
    enum class State
    {
        None,
        ResourcePending,
        ResourceUploaded,
        CommitFailed
    };
public:
    enum UploadType
    {
        // ** Upload once for optimized GPU Read/Write, CPU no access
        UPLOAD_STATIC,
        // ** Upload on demand from CPU => GPU (every time)
        UPLOAD_DYNAMIC,
        // ** Upload on resource change from CPU => GPU, otherwise use optimized GPU Read/Write.
        UPLOAD_FAST_DYNAMIC
    };

    enum BufferType
    {
        BUFFER_TYPE_VERTEX,
        BUFFER_TYPE_INDEX,
        BUFFER_TYPE_TEXTURE1D,
        BUFFER_TYPE_TEXTURE2D
    };

    DX12GfxBuffer();
    ~DX12GfxBuffer();

    void Release();
    void OnResourceDone();

    // ** @threadsafe
    APIResult<bool> SetUploadType(UploadType value);
    UploadType GetUploadType() const { return mUploadType; }
    // ** @threadsafe
    APIResult<bool> SetBufferType(BufferType value);
    BufferType GetBufferType() const { return mBufferType; }
    // ** @threadsafe
    APIResult<bool> SetBufferData(MemoryBuffer&& buffer, SizeT vertexStride, SizeT numElements);
    // ** @threadsafe
    APIResult<bool> SetBufferData(const MemoryBuffer& buffer, SizeT vertexStride, SizeT numElements);
    // ** @threadsafe
    APIResult<bool> SetTextureData(MemoryBuffer&& buffer, SizeT stride, SizeT width, SizeT height);
    // ** @threadsafe
    APIResult<bool> SetTextureData(const MemoryBuffer& buffer, SizeT stride, SizeT width, SizeT height);


    
    // ** Commit resource data...
    APIResult<bool> Commit(GfxDevice& device, GfxCommandContext& context);

    bool IsInitialized() const { return GetState() != State::None; }

    const ComPtr<ID3D12Resource>& GetResource() { return mResource; }
private:
   

    bool CanUpdateBuffer() const;

    SizeT GetBufferSize() const;
    SizeT GetBufferStride() const;

    bool CreateVertexOrIndexBuffer(GfxDevice& device, GfxCommandContext& context);

    UploadType mUploadType;
    BufferType mBufferType;

    ComPtr<ID3D12Resource> mResource;
    ComPtr<ID3D12Resource> mResourceUpload;

    MemoryBuffer mDataBuffer;
    SizeT        mDataWidth;
    SizeT        mDataHeight;
    SizeT        mDataStride;

    void SetState(State value) { AtomicStore(&mState, EnumValue(value)); }
    State GetState() const { return static_cast<State>(AtomicLoad(&mState)); }
    volatile Atomic32 mState;
    SpinLock mLock;


    
    // Asset |-> (A)LoadBinary -> (A)Decompress -> (A)GfxBuffer::SetBuffer -> (A)GfxBuffer::Commit

};

} // namespace lf
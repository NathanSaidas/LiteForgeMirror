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
#include "AbstractEngine/Gfx/GfxIndexBuffer.h"
#include "Engine/DX12/DX12GfxBuffer.h"

namespace lf {


class DX12GfxIndexBuffer : public GfxIndexBuffer
{
    DECLARE_CLASS(DX12GfxIndexBuffer, GfxIndexBuffer);
public:
    DX12GfxIndexBuffer();
    ~DX12GfxIndexBuffer() override;
    
    void Release() override;
    void Commit(GfxDevice& device, GfxCommandContext& context) override;

    APIResult<bool> SetIndices(const MemoryBuffer& indices, SizeT stride, SizeT numElements) override;
    APIResult<bool> SetIndices(MemoryBuffer&& indices, SizeT stride, SizeT numElements) override;

    const D3D12_INDEX_BUFFER_VIEW& GetView() const { return mBufferView; }
private:
    DX12GfxBuffer mBuffer;
    D3D12_INDEX_BUFFER_VIEW mBufferView;

    MemoryBuffer mClientBuffer;
    SpinLock     mLock;
};

}
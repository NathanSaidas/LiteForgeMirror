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
#include "AbstractEngine/Gfx/GfxTexture.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
class DX12GfxResourceHeap;

class LF_ENGINE_API DX12GfxTexture : public GfxTexture
{
    DECLARE_CLASS(DX12GfxTexture, GfxTexture);
public:
    DX12GfxTexture();
    ~DX12GfxTexture();

    bool Initialize(GfxDependencyContext& context) override;
    void Release() override;

    SizeT GetRequestedDescriptors() const override;


    void Commit(GfxDevice& device, GfxCommandContext& context) override;

    void SetBinary(const GfxTextureBinaryAsset& binary) override;
    void SetResourceFormat(Gfx::ResourceFormat format) override;

    bool Bind(Gfx::FrameCountType frame, D3D12_GPU_DESCRIPTOR_HANDLE& outHandle) const;

    ComPtr<ID3D12Resource> GetResource() { return mResource; }
private:
    void DecodeTexture();

    DX12GfxResourceHeap* mResourceHeap;

    Gfx::ResourceFormat mResourceFormat;
    GfxTextureBinaryAsset mBinaryAsset;
    MemoryBuffer mBinary;

    ComPtr<ID3D12Resource> mResource;
    ComPtr<ID3D12Resource> mUploadResource;
    Gfx::DescriptorView    mView;
    mutable Gfx::FrameCountType    mLastBoundFrame; // TODO: actually set this

    SizeT mWidth;
    SizeT mHeight;

};

} // namespace lf
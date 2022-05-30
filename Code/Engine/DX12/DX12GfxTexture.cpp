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
#include "DX12GfxTexture.h"
#include "Core/Reflection/DynamicCast.h"
#include "Core/Utility/Log.h"
#include "Runtime/Async/Async.h"
#include "AbstractEngine/Gfx/GfxTextureBinary.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include "Engine/DX12/DX12GfxResourceHeap.h"
#include "Engine/DX12/DX12Util.h"

#include <d3dx12.h>
// #include <DirectXTex/DirectXTex.h>
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ASSERT(expr) Assert( (expr) )
#include <stb_image.h>

namespace lf {
DEFINE_CLASS(lf::DX12GfxTexture) { NO_REFLECTION; }

DX12GfxTexture::DX12GfxTexture()
: Super()
, mResourceHeap(nullptr)
, mResourceFormat(Gfx::ResourceFormat::R32G32B32A32_FLOAT)
, mBinaryAsset()
, mBinary()
, mResource()
, mView()
, mLastBoundFrame(INVALID)
, mWidth(0)
, mHeight(0)
{

}

DX12GfxTexture::~DX12GfxTexture()
{

}

bool DX12GfxTexture::Initialize(GfxDependencyContext& context)
{
    auto dx12 = DynamicCast<DX12GfxDependencyContext>(&context);
    if (!Super::Initialize(context) || !dx12)
    {
        return false;
    }

    mResourceHeap = dx12->GetResourceHeap();
    return mResourceHeap;
}


void DX12GfxTexture::Release()
{
    if (Valid(mView.mViewID))
    {
        mResourceHeap->ReleaseTexture2D(mResource.Get(), mView, mLastBoundFrame);
        mView = Gfx::DescriptorView();
        mLastBoundFrame = INVALID;
    }

    mResourceHeap = nullptr;
}

SizeT DX12GfxTexture::GetRequestedDescriptors() const
{
    return 1;
}

void DX12GfxTexture::Commit(GfxDevice& device, GfxCommandContext& context)
{
    if (!mBinaryAsset)
    {
        return;
    }
    
    if (mBinaryAsset->GetFormat() == Gfx::TextureFileFormat::PNG)
    {
        DecodeTexture();
    }

    ComPtr<ID3D12Device> dx12 = GetDX12Device(device);
    ComPtr<ID3D12GraphicsCommandList> cmdList = GetDX12GraphicsCommandList(context);


    D3D12_RESOURCE_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(D3D12_RESOURCE_DESC));
    texDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    texDesc.Alignment = 0;
    texDesc.Width = static_cast<UINT>(mWidth);
    texDesc.Height = static_cast<UINT>(mHeight);
    texDesc.DepthOrArraySize = 1;
    texDesc.MipLevels = 0;
    texDesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    texDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    HRESULT hr = S_OK;

    CD3DX12_HEAP_PROPERTIES props(D3D12_HEAP_TYPE_DEFAULT);

    hr = dx12->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &texDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mResource));

    if (FAILED(hr))
    {
        return;
    }

    const UINT64 uploadBufferSize = GetRequiredIntermediateSize(mResource.Get(), 0, 1);

    props = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
    hr = dx12->CreateCommittedResource(
        &props,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mUploadResource));

    if (FAILED(hr))
    {
        return;
    }

    const SizeT numComponents = 4;
    const SizeT bytePerPixel = numComponents * sizeof(Float32);

    D3D12_SUBRESOURCE_DATA initData;
    initData.pData = mBinary.GetData();
    initData.RowPitch = mWidth * bytePerPixel;
    initData.SlicePitch = mWidth * mHeight * bytePerPixel;

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    cmdList->ResourceBarrier(1, &barrier);

    UpdateSubresources<1>(cmdList.Get(), mResource.Get(), mUploadResource.Get(), 0, 0, 1, &initData);

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(mResource.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    cmdList->ResourceBarrier(1, &barrier);

    mView = mResourceHeap->CreateTexture2D(this, mResource.Get());

    /*
    if (!mBinaryAsset)
    {
        return;
    }

    if (mBinaryAsset->GetFormat() == Gfx::TextureFileFormat::PNG)
    {
        GetAsync().ExecuteOn(APP_THREAD_ID_RENDER_WORKER, AppThreadDispatchCallback::Make(this, &DX12GfxTexture::DecodeTexture));
    }
    else if(mBinaryAsset->GetFormat() == Gfx::TextureFileFormat::DDS)
    {
        Graphics().UploadAsync(GetAtomicPointer(this));
    }
    else
    {
        ReportBugMsg("DX12GfxTexture - Unsupported TextureFileFormat");
    }
    */
}

void DX12GfxTexture::SetBinary(const GfxTextureBinaryAsset& binary)
{
    mBinaryAsset = binary;
}

void DX12GfxTexture::SetResourceFormat(Gfx::ResourceFormat format)
{
    mResourceFormat = format;
}

bool DX12GfxTexture::Bind(Gfx::FrameCountType frame, D3D12_GPU_DESCRIPTOR_HANDLE& outHandle) const
{
    if (Invalid(mView.mViewID))
    {
        return false;
    }
    // NOTE: We do not want to resize the descriptor so our handle should be valid for eternity!
    mLastBoundFrame = frame;
    outHandle = mView.mGPUHandle;
    return true;
}

void DX12GfxTexture::DecodeTexture()
{
    const MemoryBuffer& memory = mBinaryAsset->GetData();

    int comp = 0;
    int width = 0;
    int height = 0;
    stbi_uc* decoded = stbi_load_from_memory(reinterpret_cast<const stbi_uc*>(memory.GetData()), static_cast<int>(memory.GetSize()), &width, &height, &comp, 4);
    if (decoded == nullptr)
    {
        stbi_image_free(decoded);
        return;
    }

    // convert to float for now...
    const SizeT bufferSize = sizeof(Float32) * 4 * width * height;
    mBinary.Allocate(bufferSize, alignof(Float32));
    mBinary.SetSize(bufferSize);

    Float32* destPtr = reinterpret_cast<Float32*>(mBinary.GetData());
    ByteT* srcPtr = decoded;

    mWidth = width;
    mHeight = height;
    for (SizeT y = 0; y < mHeight; ++y)
    {
        for (SizeT x = 0; x < mWidth; ++x)
        {
            destPtr[0] = static_cast<Float32>(srcPtr[0]) / 255.0f;
            destPtr[1] = static_cast<Float32>(srcPtr[1]) / 255.0f;
            destPtr[2] = static_cast<Float32>(srcPtr[2]) / 255.0f;
            destPtr[3] = comp == 3 ? 1.0f : static_cast<Float32>(srcPtr[3]) / 255.0f;

            destPtr += 4;
            srcPtr += 4;
        }
    }
    
    stbi_image_free(decoded);
}


}
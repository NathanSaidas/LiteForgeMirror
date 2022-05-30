#include "Engine/PCH.h"
#include "DX12GfxRenderTexture.h"
#include "Core/Math/Color.h"
#include "Core/Reflection/DynamicCast.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include "Engine/DX12/DX12Util.h"
#include "Engine/DX12/DX12GfxResourceHeap.h"

#include <d3dx12.h>

namespace lf {
    DEFINE_CLASS(lf::DX12GfxRenderTexture) { NO_REFLECTION; }

    DX12GfxRenderTexture::DX12GfxRenderTexture()
    : Super()
    , mSRV()
    , mRTV()
    , mTexture()
    , mRTVHeap()
    , mResourceHeap(nullptr)
    , mLastBoundFrame(INVALID)
    , mResourceState(D3D12_RESOURCE_STATE_RENDER_TARGET)
    {}

	bool DX12GfxRenderTexture::Initialize(GfxDependencyContext& context)
	{
		auto dx12 = DynamicCast<DX12GfxDependencyContext>(&context);
		if (!Super::Initialize(context) || !dx12)
		{
			return false;
		}

		mResourceHeap = dx12->GetResourceHeap();
        return mResourceHeap;
	}

    void DX12GfxRenderTexture::Release()
    {
        if (mResourceHeap)
        {
            mResourceHeap->ReleaseTexture2D(mTexture.Get(), mSRV, mLastBoundFrame);
        }
        mTexture.Reset();
        mRTVHeap.Reset();

        Super::Release();
    }

    void DX12GfxRenderTexture::Commit(GfxDevice& device, GfxCommandContext&)
    {
        if (mTexture || mRTVHeap)
        {
            return;
        }

        if (Invalid(GetWidth()) || Invalid(GetHeight()))
        {
            return;
        }

		auto dx12 = GetDX12Device(device);

		auto desc = CD3DX12_RESOURCE_DESC::Tex2D(Gfx::DX12Value(GetFormat()),
			static_cast<UINT64>(GetWidth()),
			static_cast<UINT>(GetHeight()),
			1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

		auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

		D3D12_CLEAR_VALUE optClear;
		optClear.Format = Gfx::DX12Value(GetFormat());
		optClear.DepthStencil.Depth = 1.0f;
		optClear.DepthStencil.Stencil = 0;
        optClear.Color[0] = Color::Azure.r;
        optClear.Color[1] = Color::Azure.g;
        optClear.Color[2] = Color::Azure.b;
        optClear.Color[3] = Color::Azure.a;


        mResourceState = D3D12_RESOURCE_STATE_RENDER_TARGET;

		HRESULT hr = dx12->CreateCommittedResource(
			&heapProps,
			D3D12_HEAP_FLAG_ALLOW_ALL_BUFFERS_AND_TEXTURES,
			&desc,
            mResourceState,
			&optClear,
			IID_PPV_ARGS(&mTexture));
		if (FAILED(hr))
		{
			return;
		}

        D3D12_DESCRIPTOR_HEAP_DESC descriptorDesc = {};
        descriptorDesc.NumDescriptors = 1;
        descriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        descriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        
        hr = dx12->CreateDescriptorHeap(&descriptorDesc, IID_PPV_ARGS(&mRTVHeap));
        if (FAILED(hr))
        {
            mTexture.Reset();
            return;
        }

        mSRV = mResourceHeap->CreateTexture2D(this, mTexture.Get());
        mRTV = mRTVHeap->GetCPUDescriptorHandleForHeapStart();
        dx12->CreateRenderTargetView(mTexture.Get(), nullptr, mRTV);

        mTexture->SetName(L"Render Texture");
    }

    bool DX12GfxRenderTexture::Bind(Gfx::FrameCountType frame, D3D12_GPU_DESCRIPTOR_HANDLE& outHandle) const
    {
        if (Invalid(mSRV.mViewID))
        {
            return false;
        }
        // NOTE: We do not want to resize the descriptor so our handle should be valid for eternity!
        mLastBoundFrame = frame;
        outHandle = mSRV.mGPUHandle;
        return true;
    }


} // naemspace lf
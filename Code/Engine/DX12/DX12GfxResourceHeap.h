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
#include "Core/Common/API.h"
#include "Core/Utility/UniqueNumber.h"
#include "Core/Utility/StdMap.h"
#include "Core/Platform/SpinLock.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
class GfxDependencyContext;
class GfxDevice;
class GfxTexture;
DECLARE_ATOMIC_WPTR(GfxResourceObject);

class LF_ENGINE_API DX12GfxResourceHeap
{
public:
    DX12GfxResourceHeap();
    ~DX12GfxResourceHeap();

    void Initialize(GfxDependencyContext& context);
    void Release();

    Gfx::DescriptorView CreateTexture2D(GfxTexture* texture, ID3D12Resource* resource);

    void ReleaseTexture2D(ID3D12Resource* resource, Gfx::DescriptorView viewID, Gfx::FrameCountType lastBoundFrame);

    void Resize(Gfx::FrameCountType masterFrame, SizeT count);
    void ReleaseFrame(Gfx::FrameCountType frame);
    void CollectGarbageViews(Gfx::FrameCountType frame);
    void CollectGarbageHeaps(Gfx::FrameCountType frame);

    ID3D12DescriptorHeap* GetHeap() { return mDescriptorHeap.Get(); }

    SizeT Size() const { return static_cast<SizeT>(AtomicLoad(&mSize)); }
    SizeT Capacity() const { return static_cast<SizeT>(AtomicLoad(&mCapacity)); }
private:
    struct GarbageHeap
    {
        UInt64                       mMasterFrame;
        ComPtr<ID3D12DescriptorHeap> mHeap;
    };
    enum class ViewType
    {
        Texture2D
    };
    struct GarbageView
    {
        ComPtr<ID3D12Resource>      mResource;
        Gfx::DescriptorView         mView;
        UInt64                      mLastBoundFrame;
        ViewType                    mViewType;
    };
    using IDGen = UniqueNumber<Gfx::DescriptorViewID, 64>;
    using GarbageViewPtr = TAtomicStrongPointer<GarbageView>;

    Gfx::DescriptorViewID CreateID();
    void ReleaseID(Gfx::DescriptorViewID id);

    void NewHeap(UInt64 masterFrame, SizeT capacity);
    void ReleaseTexture2DImpl(GarbageView& handle);


    GfxDevice*                   mGfxDevice;
    // ** Actual d3d device
    ComPtr<ID3D12Device>         mDevice;
    // ** ID generator for descriptors
    SpinLock                     mIDGenLock;
    IDGen                        mDescriptorIDGen;
    // ** The size increment of a descriptor
    UINT                         mDescriptorSize;
    // ** The descriptor heap
    ComPtr<ID3D12DescriptorHeap> mDescriptorHeap; // SRV Heap

    // ** The number of descriptors used.
    volatile Atomic32 mSize;
    // ** The total number of descriptors allocated.
    volatile Atomic32 mCapacity;
    // ** Heaps to free after a certain number of frames
    TVector<GarbageHeap> mGarbageHeaps;

    TMap<Gfx::FrameCountType, TVector<GarbageViewPtr>> mGarbageViews;
};

/*
class LF_ENGINE_API DX12GfxViewHandle
{
public:
    D3D12_CPU_DESCRIPTOR_HANDLE mCPU;
    D3D12_GPU_DESCRIPTOR_HANDLE mGPU;
    Gfx::DescriptorViewID       mID;
    SizeT                       mLastBoundFrame;
};
using DX12GfxViewHandlePtr = TAtomicStrongPointer<DX12GfxViewHandle>;

class LF_ENGINE_API DX12GfxResourceHeapEx
{
public:
    DX12GfxViewHandlePtr CreateTexture2D(ComPtr<ID3D12Resource>& resource);

    void ReleaseView(const DX12GfxViewHandlePtr& view);
private:
    enum { FrameCount = 3 };

    

    // tbl:

    

};
*/

} // namespace lf
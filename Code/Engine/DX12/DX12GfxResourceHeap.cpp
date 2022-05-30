

#include "Engine/PCH.h"
#include "DX12GfxResourceHeap.h"
#include "Core/Reflection/DynamicCast.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"
#include "AbstractEngine/Gfx/GfxTexture.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"

// temp
#include "Core/Utility/Log.h"

#include <d3dx12.h>

namespace lf {
DECLARE_ATOMIC_PTR(GfxResourceObject);

DX12GfxResourceHeap::DX12GfxResourceHeap()
    : mGfxDevice(nullptr)
    , mDevice()
    , mDescriptorIDGen()
    , mDescriptorSize(0)
    , mDescriptorHeap()
    , mSize(0)
    , mCapacity(0)
    , mGarbageHeaps()
{}
DX12GfxResourceHeap::~DX12GfxResourceHeap()
{
}

void DX12GfxResourceHeap::Initialize(GfxDependencyContext& context)
{
    auto dx12 = DynamicCast<DX12GfxDependencyContext>(&context);
    if (!dx12)
    {
        return;
    }

    mGfxDevice = dx12->GetGfxDevice();
    mDevice = dx12->GetDevice();
    mDescriptorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    NewHeap(INVALID, 2048); // No Garbage Heap!

    // TODO: It seems like we shouldn't be resizing descriptor heap.. we'll need to revisit memory management later.
}

// void 

void DX12GfxResourceHeap::Release()
{

}

Gfx::DescriptorView DX12GfxResourceHeap::CreateTexture2D(GfxTexture* texture, ID3D12Resource* resource)
{
    // TODO: Make sure its valid to do this.

    Gfx::DescriptorView view;

    if (!resource || !texture)
    {
        return view;
    }

    // Allocate an ID
    Gfx::DescriptorViewID id = CreateID();
    if (id >= Capacity())
    {
        ReportBugMsg("Failed to allocate Texture2D descriptor view. Is a resource failing to report the correct number of descriptors?");
        ReleaseID(id);
        return view;
    }
    AtomicIncrement32(&mSize);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // todo: need to specify certain components
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;
    
    // Set resource attribs
    srvDesc.Format = resource->GetDesc().Format;
    srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
    
    // Create the view
    CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(id), mDescriptorSize);
    mDevice->CreateShaderResourceView(resource, &srvDesc, descriptorHandle);

    // Setup the DescriptorView 
    view.mCPUHandle = descriptorHandle;
    view.mGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mDescriptorHeap->GetGPUDescriptorHandleForHeapStart(), static_cast<INT>(id), mDescriptorSize);
    view.mViewID = id;
#if defined(LF_DEBUG)
    view.mDebugHeap = mDescriptorHeap.Get(); // track descriptor heaps in debug, we shouldn't be binding a view that is no longer valid!
#endif

#if defined(LF_DEBUG)
    void* debugHeap = view.mDebugHeap;
#else
    void* debugHeap = 0;
#endif

    gGfxLog.Info(LogMessage("CreateTexture2D ( texture=") << LogPtr(texture) << ", heap=" << LogPtr(debugHeap) << ")");


    return view;

}

void DX12GfxResourceHeap::ReleaseTexture2D(ID3D12Resource* resource, Gfx::DescriptorView view, Gfx::FrameCountType lastBoundFrame)
{
    Gfx::FrameCountType key = lastBoundFrame;

    if (Invalid(lastBoundFrame) || lastBoundFrame <= mGfxDevice->GetLastCompletedFrame())
    {
        key = INVALID;
    }

    GarbageViewPtr viewHandle(LFNew<GarbageView>());
    viewHandle->mView = view;
    viewHandle->mLastBoundFrame = lastBoundFrame;
    viewHandle->mResource = resource;
    viewHandle->mViewType = ViewType::Texture2D;

    mGarbageViews[key].push_back(viewHandle);


    /*
    // Release the ID!
    mDescriptorIDGen.Free(viewID);

    // Release the resource!
    auto it = mResources.find(GetAtomicPointer(resource));
    ReportBug(it != mResources.end()); // Bad release!
    if (it == mResources.end())
    {
        return;
    }

    Assert(Valid(it->second));
    --it->second;
    Assert(Valid(it->second));

    if(it->second == 0)
    {
        mResources.erase(it);
    }

    --mSize;
    */
}

void DX12GfxResourceHeap::Resize(Gfx::FrameCountType masterFrame, SizeT count)
{
    if (Valid(masterFrame))
    {
        gGfxLog.Info(LogMessage("Resizing Resource Heap ") << Capacity() << " => " << count);
    }

    NewHeap(masterFrame, count);
}

void DX12GfxResourceHeap::ReleaseFrame(Gfx::FrameCountType frame)
{
    CollectGarbageViews(frame);
    CollectGarbageHeaps(frame);
}

void DX12GfxResourceHeap::CollectGarbageViews(Gfx::FrameCountType frame)
{
    auto it = mGarbageViews.find(frame);
    if (it != mGarbageViews.end())
    {
        for (auto& view : it->second)
        {
            switch (view->mViewType)
            {
            case ViewType::Texture2D:
                ReleaseTexture2DImpl(*view);
                break;
            default:
                break;
            }
        }

        mGarbageViews.erase(it);
    }
}

void DX12GfxResourceHeap::CollectGarbageHeaps(Gfx::FrameCountType frame)
{
    for (auto it = mGarbageHeaps.begin(); it != mGarbageHeaps.end(); )
    {
        if (it->mMasterFrame == frame)
        {
            gGfxLog.Info(LogMessage("Release garbage heap ") << LogPtr(it->mHeap.Get()));
            it = mGarbageHeaps.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }
}

Gfx::DescriptorViewID DX12GfxResourceHeap::CreateID()
{
    ScopeLock lock(mIDGenLock);
    return mDescriptorIDGen.Allocate();
}

void DX12GfxResourceHeap::ReleaseID(Gfx::DescriptorViewID id)
{
    ScopeLock lock(mIDGenLock);
    mDescriptorIDGen.Free(id);
}

void DX12GfxResourceHeap::NewHeap(UInt64 masterFrame, SizeT capacity)
{
    Assert(capacity != 0); // TODO: Define a release function and use that instead...

    auto garabageHeap = mDescriptorHeap;
    if (mDescriptorHeap)
    {
        GarbageHeap garbage;
        garbage.mMasterFrame = masterFrame;
        garbage.mHeap = mDescriptorHeap;
        mGarbageHeaps.push_back(garbage);
    }

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
    srvHeapDesc.NumDescriptors = static_cast<UINT>(capacity);
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    Assert(SUCCEEDED(mDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mDescriptorHeap))));

    if (garabageHeap)
    {
        mDevice->CopyDescriptorsSimple(static_cast<UINT>(Size()), mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), garabageHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    }
    AtomicStore(&mCapacity, static_cast<Atomic32>(capacity));
}

void DX12GfxResourceHeap::ReleaseTexture2DImpl(GarbageView& handle)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING; // todo: need to specify certain components
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MostDetailedMip = 0;
    srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDesc.Texture2D.PlaneSlice = 0;

    // Set resource attribs
    D3D12_RESOURCE_DESC desc = handle.mResource->GetDesc();
    srvDesc.Format = desc.Format;
    srvDesc.Texture2D.MipLevels = desc.MipLevels;

    CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(mDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), static_cast<INT>(handle.mView.mViewID), mDescriptorSize);
    mDevice->CreateShaderResourceView(nullptr, &srvDesc, descriptorHandle);
    
    ReleaseID(handle.mView.mViewID);
    AtomicDecrement32(&mSize);
}

} // namespace lf
// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "DX12GfxDevice.h"
#include "Core/Utility/Log.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "Runtime/Async/AppThread.h"
#include "AbstractEngine/App/AppService.h"
#include "AbstractEngine/App/Win32Window.h"
#include "AbstractEngine/Gfx/GfxRenderer.h"
#include "AbstractEngine/Gfx/GfxRendererDependencyContext.h"
#include "Engine/DX12/DX12GfxWindow.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include "Engine/DX12/DX12GfxShaderCompiler.h"
#include "Engine/DX12/DX12GfxSwapChain.h"
#include "Engine/DX12/DX12GfxFence.h"
#include <d3dx12.h>

#include "Core/Math/Vector4.h"

#include <set>

namespace lf {
DECLARE_ATOMIC_PTR(Win32Window);
DECLARE_ATOMIC_PTR(DX12GfxSwapChain);

static const char* GAME_WINDOW_CLASS_NAME = "LiteForgeGameWindow";

class RenderThreadDispatcher : public ThreadDispatcher
{
public:
    RenderThreadDispatcher(DX12GfxDevice* device) : ThreadDispatcher(), mDevice(device) {}

    DX12GfxDevice* mDevice;
};

static void GetHardwareAdapter(IDXGIFactory2* factory, IDXGIAdapter1** outAdapter)
{
    ComPtr<IDXGIAdapter1> adapter;
    *outAdapter = nullptr;

    for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != factory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
    {
        DXGI_ADAPTER_DESC1 desc;
        adapter->GetDesc1(&desc);

        if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
        {
            continue;
        }

        if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
        {
            break;
        }
    }

    *outAdapter = adapter.Detach();
}

GfxUploadBufferAtomicPtr GfxState::UploadBufferPool::Allocate(GfxDevice& device, SizeT size)
{
    ScopeLock lock(mLock);
    GfxUploadBufferAtomicPtr result;

    auto it = mFreeBuffers.find(size);
    if (it == mFreeBuffers.end()) 
    {
        result = device.CreateResource<GfxUploadBuffer>();
        result->SetElementCount(1);
        result->SetElementSize(size);
        result->SetUploadBufferType(mUploadBufferType);
        return result;
    }

    auto& list = it->second;
    result = list.back();
    list.pop_back();
    return result;
}
void GfxState::UploadBufferPool::Release(GfxDevice& device, const GfxUploadBufferAtomicPtr& buffer)
{
    if (!buffer || buffer->GetElementSize() == 0 || buffer->GetElementCount() != 1 || buffer->GetUploadBufferType() != mUploadBufferType)
    {
        return;
    }
    ScopeLock lock(mLock);

    // Buffer is owned by multiple, they must all release!
    const bool garbage = buffer.GetStrongRefs() == 2;
    if (!garbage)
    {
        return;
    }
    Gfx::FrameCountType key = buffer->GetLastBoundFrame();
    if (key <= device.GetLastCompletedFrame())
    {
        key = INVALID;
    }
    mGarbageBuffers[key].push_back(buffer);
}
void GfxState::UploadBufferPool::ReleaseFrame(Gfx::FrameCountType frame)
{
    ScopeLock lock(mLock);

    auto it = mGarbageBuffers.find(frame);
    if (it == mGarbageBuffers.end())
    {
        return;
    }

    for (auto& garbage : it->second)
    {
        mFreeBuffers[garbage->GetElementSize()].push_back(garbage);
    }
    mGarbageBuffers.erase(it);
}

DX12GfxDevice::DX12GfxDevice()
: mFlags({GfxDeviceFlags::GDF_DEBUG, GfxDeviceFlags::GDF_WORKERTHREADED})
, mDevice()
, mDeviceFactory()
, mCommandQueue()
, mResourceCommandList()
, mResourceCommandEventListeners()
, mResourceCommandLock()
, mObjectFactory()
, mRenderThreadScheduler()
, mRenderWorkerScheduler()
, mRenderThreadDispatcher()
, mRenderWorkerThreadDispatcher()
, mRenderThreadShutdown()
, mWorkerThreadShutdown()
, mPostInitializeFence()
, mGfxState()
, mBeginFrameTasks()
, mUpdateFrameTasks()
, mEndFrameTasks()
, mRenderThreadTasks()
{

}

DX12GfxDevice::~DX12GfxDevice()
{
}

GfxSwapChainAtomicPtr DX12GfxDevice::CreateSwapChain(const AppWindowAtomicPtr& window)
{
    AppWindowAtomicPtr copy = window;
    return GetOrCreateSwapChain(copy);
}

GfxFenceAtomicPtr DX12GfxDevice::CreateFence()
{
    return CreateFenceImpl();
}

GfxUploadBufferAtomicPtr DX12GfxDevice::CreateConstantBuffer(SizeT elementSize)
{
    return mGfxState.mConstantBufferPool.Allocate(*this, elementSize);
}

void DX12GfxDevice::ReleaseConstantBuffer(const GfxUploadBufferAtomicPtr& buffer)
{
    mGfxState.mConstantBufferPool.Release(*this, buffer);
}

GfxUploadBufferAtomicPtr DX12GfxDevice::CreateStructureBuffer(SizeT elementSize)
{
    return mGfxState.mStructureBufferPool.Allocate(*this, elementSize);
}

void DX12GfxDevice::ReleaseStructureBuffer(const GfxUploadBufferAtomicPtr& buffer)
{
    mGfxState.mStructureBufferPool.Release(*this, buffer);
}

Gfx::FrameCountType DX12GfxDevice::GetCurrentFrame() const
{
    return mGfxState.mMasterFrame;
}

Gfx::FrameCountType DX12GfxDevice::GetLastCompletedFrame() const
{
    return mGfxState.GetLastCompletedFrame();
}

void DX12GfxDevice::Register(GfxRenderer* renderer)
{
    Assert(GetAsync().GetAppThreadId() == APP_THREAD_ID_MAIN);
    GfxRendererAtomicPtr pinned(GetAtomicPointer(renderer));
    if (!pinned)
    {
        return;
    }

    if (std::find(mRenderers.begin(), mRenderers.end(), renderer) != mRenderers.end())
    {
        return;
    }

    GfxRendererDependencyContext context(
        GetServices()
        , this
        , mCommandQueue);

    if (!pinned->Initialize(context))
    {
        return;
    }

    mRenderers.push_back(pinned);
}

void DX12GfxDevice::Unregister(GfxRenderer* renderer)
{
    Assert(GetAsync().GetAppThreadId() == APP_THREAD_ID_MAIN);
    if (!renderer)
    {
        return;
    }

    auto it = std::find(mRenderers.begin(), mRenderers.end(), renderer);
    if (it != mRenderers.end())
    {
        (*it)->Shutdown();
        mRenderers.swap_erase(it);
    }
}

// bool DX12GfxDevice::Initialize(GfxDeviceFlagsBitfield flags)
// {
//     mObjectFactory.Initialize();
//     InitAppThread();
//     if (!InitDirectX(flags))
//     {
//         return false;
//     }
//     if (!flags.Has(GfxDeviceFlags::GDF_HEADLESS))
//     {
//         CreateGameWindow();
// 
//         ShowWindow(mGame.mWindowHandle, SW_SHOW);
//     }
// 
// 
//     
//     return true;
//     // Create a Device Context
// }

APIResult<ServiceResult::Value> DX12GfxDevice::OnStart()
{
    APIResult<ServiceResult::Value> result = Service::OnStart();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }
    CriticalAssert(mPostInitializeFence.Initialize());
    CriticalAssert(mPostInitializeFence.Set(true));
    InitAppThread();
    if (!InitDirectX())
    {
        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_FAILED);
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> DX12GfxDevice::OnPostInitialize()
{
    APIResult<ServiceResult::Value> result = Service::OnPostInitialize();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }
    mObjectFactory.Initialize();
    InitState();

    mBeginFrameTasks.push_back(GfxTaskPtr(LFNew<GfxTask::BeginRenderTask>()));

    mEndFrameTasks.push_back(GfxTaskPtr(LFNew<GfxTask::WaitRenderDoneTask>()));

    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::WaitRenderTask>()));
    // todo: TeardownResource ( remove unused resources from descriptor heap )
    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::TeardownResource>()));
    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::BeginRecord>()));
    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::ResizeDescriptors>()));
    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::SetupResource>()));
    // todo: Ensure resources bound to descriptor heap correctly.
    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::ExecuteRenderers>()));
    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::EndRecord>()));
    mRenderThreadTasks.push_back(GfxTaskPtr(LFNew<GfxTask::EndRenderTask>()));

    auto tasks = { &mBeginFrameTasks, &mUpdateFrameTasks, &mEndFrameTasks, &mRenderThreadTasks };
    for (auto& taskList : tasks)
    {
        for (auto& task : *taskList)
        {
            task->Initialize(&mGfxState);
        }
    }

    mGfxState.mWaitRenderFence.Set(true);
    mGfxState.mWaitFrameFence.Set(true);

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> DX12GfxDevice::OnBeginFrame()
{
    APIResult<ServiceResult::Value> result = Service::OnBeginFrame();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }

    Assert(mPostInitializeFence.Set(false));

    for (GfxTask::GfxTaskBase* task : mBeginFrameTasks)
    {
        task->Execute();
    }

    for (GfxRenderer* renderer : mRenderers)
    {
        renderer->OnBeginFrame();
    }

    // Game: FrameBegin => FrameUpdate => FrameEnd
    //
    // FrameBegin:
    //   CommitDirtyResources();                                        
    //   RenderThread().Signal()
    //                              RT =>  CommitBufferedDataSync();
    // FrameEnd:
    //   RenderThread().Wait()
    
    // [Any Thread] => CreateResourceAsync()...

    // // Commit our dirty resources
    // CommitDirtyResources();
    // 
    // // Signal for AppThread to wait at end of frame, then Signal for RenderThread to start.
    // mRenderThreadWaitFence.Set(true);
    // mRenderThreadExecuteFence.Set(false);

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> DX12GfxDevice::OnEndFrame()
{
    APIResult<ServiceResult::Value> result = Service::OnEndFrame();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }
    
    if (!mRenderWorkerScheduler.IsAsync())
    {
        mRenderWorkerScheduler.UpdateSync(0.16f);
    }

    for (GfxTask::GfxTaskBase* task : mEndFrameTasks)
    {
        task->Execute();
    }

    for (GfxRenderer* renderer : mRenderers)
    {
        renderer->OnEndFrame();
    }

    // // Create Resources while we wait.
    // mEndFrameScheduler.Execute();
    // // TODO: We can actually pass 'ServiceResult::SERVICE_RESULT_PENDING' back to allow the rest of the services to do work at end of frame...
    // mRenderThreadWaitFence.Wait();
    // 
    // // There should be no more commands in-flight we should be safe to collect the garbage...
    // // TODO: We can submit this as an async task and then wait in OnBeginFrame until it's complete...
    CollectGarbage();

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> DX12GfxDevice::OnFrameUpdate()
{
    APIResult<ServiceResult::Value> result = Service::OnFrameUpdate();
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }

    for (GfxTask::GfxTaskBase* task : mUpdateFrameTasks)
    {
        task->Execute();
    }

    for (GfxRenderer* renderer : mRenderers)
    {
        renderer->OnUpdate();
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> DX12GfxDevice::OnShutdown(ServiceShutdownMode mode)
{
    APIResult<ServiceResult::Value> result = Service::OnShutdown(mode);
    if (result == ServiceResult::SERVICE_RESULT_FAILED)
    {
        return result;
    }
    CollectGarbage();

    mGfxState.mWaitRenderFence.Set(false); // OK -- We might fail initalization
    Assert(mPostInitializeFence.Set(false) || mode != ServiceShutdownMode::SHUTDOWN_NORMAL);
    Assert(GetAsync().StopThread(APP_THREAD_ID_RENDER) || mode != ServiceShutdownMode::SHUTDOWN_NORMAL);
    Assert(GetAsync().StopThread(APP_THREAD_ID_RENDER_WORKER) || mode != ServiceShutdownMode::SHUTDOWN_NORMAL);
    ShutdownAppThread();
    ShutdownDirectX();
    ShutdownState();
    


    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

TAtomicStrongPointer<GfxResourceObject> DX12GfxDevice::CreateResourceObject(const Type* type)
{
    const Type* resourceType = mObjectFactory.GetType(type);
    if (!resourceType)
    {
        return NULL_PTR;
    }
    LF_SCOPED_MEMORY(MMT_GRAPHICS);
    TAtomicStrongPointer<GfxResourceObject> resource = GetReflectionMgr().CreateAtomic<GfxResourceObject>(resourceType);
    if (!resource)
    {
        return NULL_PTR;
    }
    mObjectFactory.TrackInstance(resource);

    if (resourceType->IsA(typeof(GfxSwapChain)))
    {
        gGfxLog.Warning(LogMessage("Called GfxDevice::CreateResource on a GfxSwapChain. The swap chain won't be initialized correctly, use CreateSwapChain instead."));
    }

    DX12GfxDependencyContext context(this,
        mDevice, 
        mDeviceFactory, 
        mCommandQueue->CommandQueue(),  
        mResourceCommandList.mCommandList,
        &mResourceHeap);
    resource->Initialize(context);

    gGfxLog.Info(LogMessage("Create & Initialize resource ") << resource->GetType()->GetFullName());

    return resource;
}

GfxFenceAtomicPtr DX12GfxDevice::CreateFenceImpl()
{
    TAtomicStrongPointer<DX12GfxFence> dx12Fence = GetReflectionMgr().CreateAtomic<DX12GfxFence>();
    if (dx12Fence)
    {
        dx12Fence->Initialize(mDevice.Get());
    }
    return dx12Fence;
}

GfxSwapChainAtomicPtr DX12GfxDevice::GetOrCreateSwapChain(const AppWindowAtomicPtr& window)
{
    Win32WindowAtomicPtr win32Window = DynamicCast<Win32WindowAtomicPtr>(window);
    if (!win32Window)
    {
        return GfxSwapChainAtomicPtr();
    }

    auto it = std::find_if(mWindowSwapChains.begin(), mWindowSwapChains.end(), [window](const GfxState::WindowSwapChain& ws) { return ws.mWindow == window; });
    if (it != mWindowSwapChains.end())
    {
        return it->mSwapChain;
    }

    const Type* swapChainType = mObjectFactory.GetType(typeof(GfxSwapChain));
    if (!swapChainType)
    {
        return GfxSwapChainAtomicPtr();
    }

    LF_SCOPED_MEMORY(MMT_GRAPHICS);
    TAtomicStrongPointer<GfxSwapChain> swapChain = GetReflectionMgr().CreateAtomic<GfxSwapChain>(swapChainType);
    if (!swapChain)
    {
        return GfxSwapChainAtomicPtr();
    }

    mObjectFactory.TrackInstance(swapChain);

    DX12GfxDependencyContext context(this,
        mDevice,
        mDeviceFactory,
        mCommandQueue->CommandQueue(),
        mResourceCommandList.mCommandList,
        &mResourceHeap);

    if (!swapChain->Initialize(context) || !swapChain->InitializeSwapChain(context, window))
    {
        return GfxSwapChainAtomicPtr();
    }

    GfxState::WindowSwapChain wsc;
    wsc.mWindow = window;
    wsc.mSwapChain = swapChain;

    mWindowSwapChains.push_back(wsc);

    return swapChain;
}

bool DX12GfxDevice::WaitForUpdate()
{
    mPostInitializeFence.Wait();
    return GetServiceState() == ServiceState::RUNNING;
}

void DX12GfxDevice::DispatchRenderThread()
{
    ReportBug(mRenderThreadDispatcher != NULL_PTR);
    if (mRenderThreadDispatcher)
    {
        mRenderThreadDispatcher->Dispatch();
    }
}

void DX12GfxDevice::DispatchWorkerThread()
{
    ReportBug(mRenderWorkerThreadDispatcher != NULL_PTR);
    if (mRenderWorkerThreadDispatcher)
    {
        
        mRenderWorkerThreadDispatcher->Dispatch();
    }
}

void DX12GfxDevice::RenderThread(AppThread* thread)
{
    SetThreadName("RenderThread");

    if (!WaitForUpdate())
    {
        mRenderThreadShutdown.Set(false);
        return;
    }

    // Task<void> task;
    while (thread->IsRunning())
    {
        // task.SetCallback(TCallback<void>::Make(this, &DX12GfxDevice::DispatchRenderThread));
        // task.Run(mRenderThreadScheduler);
        // CriticalAssert(task.Wait());
        // task = Task<void>();

        mRenderThreadDispatcher->Dispatch();

        for (GfxTask::GfxTaskBase* task : mRenderThreadTasks)
        {
            if (!thread->IsRunning())
            {
                break;
            }
            task->Execute();
        }

        // ThreadFence::WaitStatus ws = mRenderThreadExecuteFence.Wait(16);
        // if (ws != ThreadFence::WaitStatus::WS_SUCCESS)
        // {
        //     continue;
        // }
        // 
        // if (!thread->IsRunning())
        // {
        //     mRenderThreadWaitFence.Set(false);
        //     break;
        // }
        // // Wait next frame...
        // mRenderThreadExecuteFence.Set(true);
        // 
        // DispatchRenderThread();
        // mResourceCommandList->Reset(mResourceCommandAllocator.Get(), mDebugMaterial->GetPSO().Get());
        // 
        // mRenderThreadScheduler.UpdateSync(0.016f);
        // 
        // // Signal done!
        // mRenderThreadWaitFence.Set(false);
    }

    mRenderThreadShutdown.Set(false);
} 

void DX12GfxDevice::RenderWorkerThread(AppThread* thread)
{
    if (!WaitForUpdate())
    {
        mWorkerThreadShutdown.Set(false);
        return;
    }

    Task<void> task;
    while (thread->IsRunning())
    {
        // Check for work every "frame" or just start when we receive some work.
        const SizeT frameTimeMS = 16; // target 60fps?
        mRenderWorkerThreadDispatcher->Wait(frameTimeMS);

        if (!thread->IsRunning())
        {
            break;
        }

        task.SetCallback(TCallback<void>::Make(this, &DX12GfxDevice::DispatchWorkerThread));
        task.Run(mRenderWorkerScheduler);
        CriticalAssert(task.Wait());
        task = Task<void>();
    }

    mWorkerThreadShutdown.Set(false);
}

void DX12GfxDevice::RenderAppThread(AppThread* thread)
{
    RenderThreadDispatcher* dispatcher = static_cast<RenderThreadDispatcher*>(thread->GetDispatcher());
    dispatcher->mDevice->RenderThread(thread);
}
void DX12GfxDevice::RenderAppWorkerThread(AppThread* thread)
{
    RenderThreadDispatcher* dispatcher = static_cast<RenderThreadDispatcher*>(thread->GetDispatcher());
    dispatcher->mDevice->RenderWorkerThread(thread);
}

void DX12GfxDevice::InitAppThread()
{
    CriticalAssert(mRenderThreadShutdown.Initialize());
    CriticalAssert(mWorkerThreadShutdown.Initialize());
    CriticalAssert(mRenderThreadShutdown.Set(true));
    CriticalAssert(mWorkerThreadShutdown.Set(true));

    // mRenderThreadScheduler.Initialize((!mFlags.Has(GfxDeviceFlags::GDF_SINGLETHREADED)));
    // mRenderWorkerScheduler.Initialize(());

    mRenderThreadScheduler.Initialize(false);

    const bool workerAsync = !mFlags.Has(GfxDeviceFlags::GDF_SINGLETHREADED) && mFlags.Has(GfxDeviceFlags::GDF_WORKERTHREADED);
    TaskScheduler::OptionsType options;
    options.mNumWorkerThreads = 2;
    options.mDispatcherSize = 100;
    mRenderWorkerScheduler.Initialize(options, workerAsync);

    {
        LF_SCOPED_MEMORY(MMT_GRAPHICS);
        mRenderThreadDispatcher = ThreadDispatcherPtr(LFNew<RenderThreadDispatcher>(this));
        mRenderWorkerThreadDispatcher = ThreadDispatcherPtr(LFNew<RenderThreadDispatcher>(this));
    }

    // TODO: We need to start earlier... like in OnStart...
    AppThreadAttributes renderThreadAttribs;
    renderThreadAttribs.mDispatcher = mRenderThreadDispatcher;
    gGfxLog.Info(LogMessage("Initialize AppThread.Render"));
    CriticalAssert(GetAsync().StartThread(APP_THREAD_ID_RENDER, AppThreadCallback::Make(RenderAppThread), renderThreadAttribs));

    AppThreadAttributes workerThreadAttribs;
    workerThreadAttribs.mDispatcher = mRenderWorkerThreadDispatcher;
    gGfxLog.Info(LogMessage("Initialize AppThread.RenderWorker"));
    CriticalAssert(GetAsync().StartThread(APP_THREAD_ID_RENDER_WORKER, AppThreadCallback::Make(RenderAppWorkerThread), workerThreadAttribs));

    
}

void DX12GfxDevice::ShutdownAppThread()
{
    // Flush pending data...
    if (mRenderThreadScheduler.IsRunning())
    {
        mRenderThreadScheduler.Shutdown();
    }

    if (mRenderWorkerScheduler.IsRunning())
    {
        mRenderWorkerScheduler.Shutdown();
    }

    // Wait for completion
    mRenderThreadShutdown.Wait();
    mWorkerThreadShutdown.Wait();

    mRenderThreadShutdown.Destroy();
    mWorkerThreadShutdown.Destroy();
}

bool DX12GfxDevice::InitDirectX()
{
    UINT dxgiFactoryFlags = 0;

    // Enable debug mode if necessary.
#if !defined(LF_FINAL)
    if (mFlags.Has(GfxDeviceFlags::GDF_DEBUG))
    {
        ComPtr<ID3D12Debug> debugInterface;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface))))
        {
            debugInterface->EnableDebugLayer();
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#else
    (dxgiFactoryFlags);
#endif

    // Create the DirectX Device
    if (FAILED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mDeviceFactory))))
    {
        return false;
    }

    ComPtr<IDXGIAdapter1> hardwareAdapter;
    GetHardwareAdapter(mDeviceFactory.Get(), &hardwareAdapter);

    gGfxLog.Info(LogMessage("Initialize DX12 Device"));

    if (FAILED(D3D12CreateDevice(
        hardwareAdapter.Get(),
        D3D_FEATURE_LEVEL_11_0,
        IID_PPV_ARGS(&mDevice))))
    {
        return false;
    }

    mCommandQueue = MakeConvertibleAtomicPtr<DX12GfxCommandQueue>();
    DX12GfxDependencyContext queueContext(this,
        mDevice,
        mDeviceFactory,
        nullptr,
        nullptr,
        nullptr);
    if (!mCommandQueue->Initialize(queueContext))
    {
        return false;
    }

    DX12GfxDependencyContext context(this,
        mDevice,
        mDeviceFactory,
        mCommandQueue->CommandQueue(),
        nullptr,
        nullptr);

    if (!mResourceCommandList.Initialize(context))
    {
        return false;
    }

    DX12GfxDependencyContext heapContext(this,
        mDevice,
        mDeviceFactory,
        mCommandQueue->CommandQueue(),
        mResourceCommandList.mCommandList,
        nullptr);
    mResourceHeap.Initialize(context);

    return true;
}

void DX12GfxDevice::ShutdownDirectX()
{
    Assert(mResourceCommandEventListeners.empty());
    mResourceCommandList.Release();
    mResourceHeap.Release();
    mCommandQueue.Release();
    mDeviceFactory.Reset();
    mDevice.Reset();
}

void DX12GfxDevice::CollectGarbage()
{
    for (auto it = mWindowSwapChains.begin(); it != mWindowSwapChains.end(); )
    {
        if (!it->mWindow->IsOpen())
        {
            gGfxLog.Info(LogMessage("Disconnecting swap chain for window. ID=") << it->mWindow->GetID());
            mGarbageSwapChains.push_back(it->mSwapChain);
            it = mWindowSwapChains.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }

    for (auto it = mGarbageSwapChains.begin(); it != mGarbageSwapChains.end();)
    {
        if (it->GetStrongRefs() == 1)
        {
            (*it)->Release();
            it = mGarbageSwapChains.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }

    mObjectFactory.CollectGarbage(DX12GfxFactory::GarbageCallback::Make([this](GfxResourceObjectAtomicPtr& garbage)
        {
            gGfxLog.Info(LogMessage("Resource object out of scope, releasing. Type=") << garbage->GetType()->GetFullName());
            garbage->Release();
        }));


}

void DX12GfxDevice::InitState()
{
    CriticalAssert(mGfxState.mWaitFrameFence.Initialize());
    CriticalAssert(mGfxState.mWaitRenderFence.Initialize());

    CriticalAssert(mGfxState.mWaitFrameFence.Set(false));
    CriticalAssert(mGfxState.mWaitRenderFence.Set(false));
    mGfxState.mMasterFrame = 0;
    mGfxState.mMasterFrameIndex = 0;

    for (SizeT i = 0; i < LF_ARRAY_SIZE(mGfxState.mFrameResources); ++i)
    {
        mGfxState.mFrameResources[i].mCommandContext = CreateResource<GfxCommandContext>();
        mGfxState.mFrameResources[i].mFence = CreateFence();
        mGfxState.mFrameResources[i].mFence->StartThread();
    }

    mGfxState.mDevice = this;
    mGfxState.mCommandQueue = mCommandQueue;
    mGfxState.mSwapChains = &mWindowSwapChains;
    mGfxState.mWorkerScheduler = &mRenderWorkerScheduler;
    mGfxState.mRenderers = &mRenderers;
    mGfxState.mResourceHeap = &mResourceHeap;
    mGfxState.mObjectFactory = &mObjectFactory;
}

void DX12GfxDevice::ShutdownState()
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mGfxState.mFrameResources); ++i)
    {
        if (mGfxState.mFrameResources[i].mFence)
        {
            mGfxState.mFrameResources[i].mFence->Wait();
            mGfxState.mFrameResources[i].mFence->StopThread();
        }
    }

    for (SizeT i = 0; i < LF_ARRAY_SIZE(mGfxState.mFrameResources); ++i)
    {
        mGfxState.mFrameResources[i].mFence.Release();
        mGfxState.mFrameResources[i].mCommandContext.Release();
    }

    mGfxState.mWaitFrameFence.Destroy();
    mGfxState.mWaitRenderFence.Destroy();

}

// APIResult<GfxWindowAdapterAtomicPtr> DX12GfxDevice::CreateWindow(const String& title, SizeT width, SizeT height, const String& classID)
// {
//     static const char* STANDARD_PLATFORM_CLASS = "DefaultLiteForgeWindow";
//     APIResult<GfxAdapterAtomicPtr> result = CreateAdapter(typeof(GfxWindow));
//     GfxWindowAdapterAtomicPtr window = DynamicCast<GfxWindowAdapterAtomicPtr>(result.GetItem());
//     if (!window)
//     {
//         return APIResult<GfxWindowAdapterAtomicPtr>(GfxWindowAdapterAtomicPtr(), result);
//     }
// 
//     DX12GfxDependencyContext dependencies(mDevice, mDeviceFactory);
//     window->Initialize(dependencies, this, GfxObservedObject());
// 
//     // Set dynamic data
//     window->SetTitle(title);
//     window->SetWidth(static_cast<Int32>(width));
//     window->SetHeight(static_cast<Int32>(height));
//     window->SetClassName(classID.Empty() ? String(STANDARD_PLATFORM_CLASS, COPY_ON_WRITE) : classID);
// 
//     mFactory.RegisterAdapter(window, false);
// 
//     return APIResult<GfxWindowAdapterAtomicPtr>(window);
// }
// 
// bool DX12GfxDevice::QueryMappedTypes(const Type* baseType, const Type** implementation, const Type** adapter)
// {
//     if (baseType == nullptr)
//     {
//         ReportBugMsg("Invalid argument 'baseType'. Value=null");
//         return false;
//     }
//     if (!baseType->IsA(typeof(GfxObject)))
//     {
//         gGfxLog.Error(LogMessage("BaseType=") << baseType->GetFullName());
//         ReportBugMsg("Invalid argument 'baseType'. Type must inherit from GfxObject.");
//         return false;
//     }
//     if (implementation == nullptr && adapter == nullptr)
//     {
//         return false;
//     }
// 
//     GfxTypeFactory::TypeMapping mapping;
//     if (!mFactory.GetMappedTypes(baseType, mapping))
//     {
//         return false;
//     }
//     
//     if (implementation != nullptr)
//     {
//         *implementation = mapping.mImplementationType;
//     }
//     if (adapter != nullptr)
//     {
//         *adapter = mapping.mAdapterType;
//     }
//     return true;
// }
// 
// APIResult<GfxObjectAtomicPtr> DX12GfxDevice::CreateObject(const Type* )
// {
//     return APIResult<GfxObjectAtomicPtr>(NULL_PTR);
// }
// 
// APIResult<bool> DX12GfxDevice::CreateAdapter(GfxObject* object)
// {
//     if (object == nullptr)
//     {
//         ReportBugMsg("Argument is null 'object'");
//         return ReportError(false, ArgumentNullError, "object");
//     }
// 
//     if (object->GetAssetType() == nullptr && object->GetWeakPointer() == NULL_PTR)
//     {
//         return ReportError(false, InvalidArgumentError, "object", "Graphics objects that aren't assets must be convertible smart pointers.");
//     }
//     
//     if (object->GetAdapter() || object->GetDevice())
//     {
//         return ReportError(false, InvalidArgumentError, "object", "Graphics object already has Adapter or Device");
//     }
// 
//     if (!object->GetType())
//     {
//         return ReportError(false, InvalidArgumentError, "object", "Missing runtime type information.");
//     }
// 
//     GfxTypeFactory::TypeMapping mapping;
//     if (!mFactory.GetMappedTypes(object->GetType(), mapping))
//     {
//         gGfxLog.Warning(LogMessage("Attempting to initialize unmapped GfxObject. Type=") << object->GetType()->GetFullName());
//         return APIResult<bool>(false);
//     }
// 
//     GfxAdapterAtomicPtr adapter = GetReflectionMgr().CreateAtomic<GfxAdapter>(mapping.mAdapterType, MMT_GRAPHICS);
//     if (!adapter)
//     {
//         return APIResult<bool>(false);;
//     }
// 
//     // Hookup device
//     object->SetDevice(this);
// 
//     // Hookup adapter
//     object->SetAdapter(adapter);
// 
//     // Initialize Adapter
//     DX12GfxDependencyContext dependencies(mDevice, mDeviceFactory);
//     GfxObservedObject observed;
//     if (object->GetAssetType() && object->GetAssetType()->IsPrototype(object))
//     {
//         observed.mType = GfxObjectAssetType(object->GetAssetType());
//     }
//     else
//     {
//         observed.mInstance = GetAtomicPointer(object);
//     }
//     adapter->Initialize(dependencies, this, observed);
// 
//     // Register Object:
//     const bool attached = true;
//     mFactory.RegisterAdapter(adapter, attached);
//     return APIResult<bool>(true);
// }
// 
// APIResult<GfxAdapterAtomicPtr> DX12GfxDevice::CreateAdapter(const Type* baseType)
// {
//     if (baseType == nullptr)
//     {
//         return ReportError(GfxAdapterAtomicPtr(), ArgumentNullError, "baseType");
//     }
// 
//     GfxTypeFactory::TypeMapping mapping;
//     if (!mFactory.GetMappedTypes(baseType, mapping))
//     {
//         gGfxLog.Warning(LogMessage("Attempting to initialize unmapped GfxObject. Type=") << baseType->GetFullName());
//         return APIResult<GfxAdapterAtomicPtr>(GfxAdapterAtomicPtr());
//     }
// 
//     GfxAdapterAtomicPtr adapter = GetReflectionMgr().CreateAtomic<GfxAdapter>(mapping.mAdapterType, MMT_GRAPHICS);
//     if (!adapter)
//     {
//         return APIResult<GfxAdapterAtomicPtr>(NULL_PTR);;
//     }
// 
//     // Initialize Adapter
//     DX12GfxDependencyContext dependencies(mDevice, mDeviceFactory);
//     adapter->Initialize(dependencies, this, GfxObservedObject());
// 
//     const bool attached = false;
//     mFactory.RegisterAdapter(adapter, attached);
// 
//     return APIResult<GfxAdapterAtomicPtr>(adapter);
// }
// 
// APIResult<Gfx::ResourcePtr> DX12GfxDevice::CreateVertexBuffer(SizeT numElements, SizeT stride, Gfx::BufferUsage usage, const void* initialData, SizeT initialDataSize)
// {
//     (numElements);
//     (stride);
//     (usage);
//     (initialData);
//     (initialDataSize);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
//     return APIResult<Gfx::ResourcePtr>(Gfx::ResourcePtr());
// }
// APIResult<Gfx::ResourcePtr> DX12GfxDevice::CreateIndexBuffer(SizeT numElements, Gfx::IndexStride stride, Gfx::BufferUsage usage, const void* initialData, SizeT initialDataSize)
// {
//     (numElements);
//     (stride);
//     (usage);
//     (initialData);
//     (initialDataSize);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
//     return APIResult<Gfx::ResourcePtr>(Gfx::ResourcePtr());
// }
// 
// APIResult<bool> DX12GfxDevice::CopyVertexBuffer(Gfx::Resource* vertexBuffer, SizeT numElements, SizeT stride, const void* vertexData)
// {
//     (numElements);
//     (stride);
//     (vertexBuffer);
//     (vertexData);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
//     return APIResult<bool>(false);
// }
// 
// APIResult<bool> DX12GfxDevice::CopyIndexBuffer(Gfx::Resource* indexBuffer, SizeT numElements, Gfx::IndexStride stride, const void* indexData)
// {
//     (numElements);
//     (stride);
//     (indexBuffer);
//     (indexData);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
//     return APIResult<bool>(false);
// }
// 
// void DX12GfxDevice::BeginFrame(const GfxWindowAdapterAtomicWPtr& window)
// {
//     (window);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::EndFrame()
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::BindPipelineState(GfxMaterialAdapter* adapter)
// {
//     (adapter);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::UnbindPipelineState()
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::BindVertexBuffer(Gfx::Resource* vertexBuffer)
// {
//     (vertexBuffer);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// 
// }
// void DX12GfxDevice::UnbindVertexBuffer()
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// 
// void DX12GfxDevice::BindIndexBuffer(Gfx::Resource* indexBuffer) 
// {
//     (indexBuffer);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::UnbindIndexBuffer()
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// 
// void DX12GfxDevice::SetViewport(Float32 x, Float32 y, Float32 width, Float32 height, Float32 minDepth, Float32 maxDepth)
// {
//     (x);
//     (y);
//     (width);
//     (height);
//     (minDepth);
//     (maxDepth);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::SetScissorRect(Float32 x, Float32 y, Float32 width, Float32 height)
// {
//     (x);
//     (y);
//     (width);
//     (height);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// 
// void DX12GfxDevice::ClearColor(const Color& color)
// {
//     (color);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::ClearDepth()
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::SwapBuffers()
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::Draw(SizeT vertexCount)
// {
//     (vertexCount);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// void DX12GfxDevice::DrawIndexed(SizeT indexCount)
// {
//     (indexCount);
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// }
// 
// bool DX12GfxDevice::IsDebug() const
// {
//     return mDebug;
// }
// 
// APIResult<Gfx::ResourcePtr> DX12GfxDevice::CompileAndLoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines)
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
// 
//     (shaderType);
//     (shader);
//     (defines);
//     return APIResult<Gfx::ResourcePtr>(NULL_PTR);
// }
// 
// APIResult<Gfx::ResourcePtr> DX12GfxDevice::LoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines)
// {
//     gGfxLog.Error(LogMessage("TODO: Implement ") << __FUNCTION__);
//     (shaderType);
//     (shader);
//     (defines);
//     return APIResult<Gfx::ResourcePtr>(NULL_PTR);
// }

} // namespace lf
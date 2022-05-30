#pragma once
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
#include "AbstractEngine/Gfx/GfxDevice.h"

#include "Core/Concurrent/Task.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Platform/ThreadFence.h"
#include "Core/Math/MathFunctions.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "Runtime/Async/Async.h"
#include "Runtime/Async/ThreadDispatcher.h"
#include "AbstractEngine/Gfx/GfxTaskScheduler.h"
#include "AbstractEngine/Gfx/GfxSwapChain.h"
#include "AbstractEngine/Gfx/GfxRenderer.h"
#include "AbstractEngine/Gfx/GfxCommandContext.h"
#include "AbstractEngine/Gfx/GfxUploadBuffer.h"
#include "AbstractEngine/Gfx/GfxFence.h"
#include "Engine/DX12/DX12Common.h"
#include "Engine/DX12/DX12GfxFactory.h"
#include "Engine/DX12/DX12GfxResourceCommandList.h"
#include "Engine/DX12/DX12GfxResourceHeap.h"
#include "Engine/DX12/DX12GfxCommandQueue.h"

// temp:
#include "Core/Utility/Log.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"

#include "Core/Utility/StdUnorderedSet.h"

#ifdef CreateWindow
#undef CreateWindow
#endif

namespace lf {

DECLARE_ATOMIC_PTR(AppWindow);
DECLARE_ATOMIC_PTR(GfxRenderer);
DECLARE_ATOMIC_PTR(GfxFence);
DECLARE_ATOMIC_PTR(GfxCommandContext);
DECLARE_ATOMIC_PTR(GfxUploadBuffer);
DECLARE_PTR(DX12GfxFactory);
DECLARE_PTR(ThreadDispatcher);

class InputMgr;
class AppService;



// The Pipeline State Object(PSO) contains most of the state that is required to configure the rendering(or compute) pipeline.The graphics pipeline state object includes the following information[3]:
// 
// Shader bytecode(vertex, pixel, domain, hull, and geometry shaders)
// Vertex format input layout
// Primitive topology type(point, line, triangle, or patch)
// Blend state
// Rasterizer state
// Depth - stencil state
// Number of render targets and render target formats
// Depth - stencil format
// Multisample description
// Stream output buffer description
// Root signature

// Although the pipeline state object contains a lot of information, there are a few additional parameters that must be set outside of the pipeline state object.
// 
// Vertexand Index buffers
// Stream output buffer
// Render targets
// Descriptor heaps
// Shader parameters(constant buffers, read - write buffers, and read - write textures)
// Viewports
// Scissor rectangles
// Constant blend factor
// Stencil reference value
// Primitive topologyand adjacency information

// ** Graphics state we send to all the 'passes'
struct GfxState
{
    GfxState()
        : mDevice(nullptr)
        , mCommandQueue(nullptr)
        , mSwapChains(nullptr)
        , mWorkerScheduler(nullptr)
        , mResourceHeap(nullptr)
        , mRenderers(nullptr)
        , mObjectFactory(nullptr)
        , mConstantBufferPool(Gfx::UploadBufferType::Constant)
        , mStructureBufferPool(Gfx::UploadBufferType::Structured)
        , mFrameResources{}
        , mMasterFrame(0)
        , mMasterFrameIndex(0)
        , mWaitFrameFence()
        , mWaitRenderFence()
        , mCompletedFrameLock()
        , mCompletedFrames()
        , mLastCompletedFrame(0)
    {}

    struct FrameResources
    {
        GfxCommandContextAtomicPtr mCommandContext;
        GfxFenceAtomicPtr          mFence;
    };

    struct WindowSwapChain // TODO: Cleanup data structures
    {
        AppWindowAtomicPtr mWindow;
        GfxSwapChainAtomicPtr mSwapChain;
    };

    // Pool Assumes ElementCount == 1
    struct UploadBufferPool
    {
        UploadBufferPool(Gfx::UploadBufferType type)
            : mLock()
            , mGarbageBuffers()
            , mFreeBuffers()
            , mUploadBufferType(type)
        {}

        GfxUploadBufferAtomicPtr Allocate(GfxDevice& device, SizeT size);
        void Release(GfxDevice& device, const GfxUploadBufferAtomicPtr& buffer);
        void ReleaseFrame(Gfx::FrameCountType frame);

        SpinLock mLock;
        TMap<Gfx::FrameCountType, TVector<GfxUploadBufferAtomicPtr>> mGarbageBuffers;
        TMap<SizeT, TVector<GfxUploadBufferAtomicPtr>> mFreeBuffers;
        Gfx::UploadBufferType mUploadBufferType;
    };

    GfxDevice* mDevice;
    GfxCommandQueue* mCommandQueue;
    TVector<GfxState::WindowSwapChain>* mSwapChains; // TODO: There is no lock on the swap chains
    TaskSchedulerBase* mWorkerScheduler;
    DX12GfxResourceHeap* mResourceHeap;
    TVector<GfxRendererAtomicPtr>* mRenderers;
    DX12GfxFactory* mObjectFactory;
    UploadBufferPool mConstantBufferPool;
    UploadBufferPool mStructureBufferPool;

    FrameResources mFrameResources[Gfx::FrameCount::Value];
    Gfx::FrameCountType mMasterFrame;
    Gfx::FrameCountType mMasterFrameIndex;
    ThreadFence mWaitFrameFence;
    ThreadFence mWaitRenderFence;

    mutable SpinLock mCompletedFrameLock;
    TVector<Gfx::FrameCountType> mCompletedFrames;

    void SetLastCompletedFrame(Gfx::FrameCountType frame)
    {
        ScopeLock lock(mCompletedFrameLock);
        mLastCompletedFrame = frame;
    }

    Gfx::FrameCountType GetLastCompletedFrame() const
    {
        ScopeLock lock(mCompletedFrameLock);
        return mLastCompletedFrame;
    }
private:
    volatile Gfx::FrameCountType mLastCompletedFrame;
};

namespace GfxTask {
class GfxTaskBase
{
public:
    virtual ~GfxTaskBase()
    {}

    void Initialize(GfxState* state)
    {
        mState = state;
    }

    void Execute()
    {
        OnExecute();
    }

protected:
    virtual void OnExecute() = 0;

    GfxState& GetState() { return *mState; }
private:
    GfxState* mState;
};

class WaitRenderDoneTask : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        GetState().mWaitFrameFence.Wait();
    }
};

class BeginRenderTask : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        GetState().mWaitRenderFence.Set(false);
        GetState().mWaitFrameFence.Set(true);
    }
};

class EndRenderTask : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        GetState().mWaitRenderFence.Set(true);
        GetState().mWaitFrameFence.Set(false);
    }
};

class WaitRenderTask : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        GetState().mWaitRenderFence.Wait();
    }
};

class TeardownResource : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        TVector<Gfx::FrameCountType> completedFrames;
        {
            ScopeLock lock(GetState().mCompletedFrameLock);
            completedFrames.swap(GetState().mCompletedFrames);
        }

        for (auto frame : completedFrames)
        {
            GetState().mResourceHeap->ReleaseFrame(frame);
            GetState().mConstantBufferPool.ReleaseFrame(frame);
            GetState().mStructureBufferPool.ReleaseFrame(frame);
        }

        GetState().mResourceHeap->ReleaseFrame(INVALID);
        GetState().mConstantBufferPool.ReleaseFrame(INVALID);
        GetState().mStructureBufferPool.ReleaseFrame(INVALID);
    }
};

class BeginRecord : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        // Frame Management:
        Gfx::FrameCountType currentFrame = GetState().mMasterFrame;
        GfxState::FrameResources& frame = GetState().mFrameResources[GetState().mMasterFrameIndex];
        frame.mFence->Wait();
        frame.mFence->SetCallback(GfxFence::FenceWaitCallback::Make(
            [currentFrame, this]()
            {
                GetState().SetLastCompletedFrame(currentFrame);
                ScopeLock lock(GetState().mCompletedFrameLock);
                GetState().mCompletedFrames.push_back(currentFrame);
            }));

        frame.mCommandContext->BeginRecord(currentFrame);
    }
};

class SetupResource : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        GfxState::FrameResources& frame = GetState().mFrameResources[GetState().mMasterFrameIndex];

        for (GfxRenderer* renderer : *GetState().mRenderers)
        {
            renderer->SetupResource(*GetState().mDevice, *frame.mCommandContext);
        }
    }
};

class ResizeDescriptors : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        SizeT numDescriptors = 0;

        GetState().mObjectFactory->ForEachInstance(
            [&numDescriptors](const GfxResourceObject* resource)
            {
                numDescriptors += resource->GetRequestedDescriptors();
            });

        const SizeT requiredDescriptors = numDescriptors;
        const SizeT capacity = GetState().mResourceHeap->Capacity();
        if (requiredDescriptors > capacity)
        {
            const SizeT newCapacity = requiredDescriptors > capacity*2 ? requiredDescriptors*2 : capacity*2;
            GetState().mResourceHeap->Resize(GetState().mMasterFrame, newCapacity);
        }
        
    }
};

class ExecuteRenderers : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        GfxState::FrameResources& frame = GetState().mFrameResources[GetState().mMasterFrameIndex];

        // Recording
        for (GfxRenderer* renderer : *GetState().mRenderers)
        {
            GfxRendererAtomicPtr pinned(GetAtomicPointer(renderer));
            if (pinned)
            {
                pinned->OnRender(*GetState().mDevice, *frame.mCommandContext);
            }
        }
        
    }
};

class EndRecord : public GfxTaskBase
{
protected:
    void OnExecute() override
    {
        GfxState::FrameResources& frame = GetState().mFrameResources[GetState().mMasterFrameIndex];
        frame.mCommandContext->EndRecord();

        // Execution
        GetState().mCommandQueue->Execute(frame.mCommandContext);
        for (auto& pair : *GetState().mSwapChains)
        {
            if (pair.mSwapChain->IsDirty())
            {
                pair.mSwapChain->Present();
                pair.mSwapChain->SetDirty(false);
            }
        }
        GetState().mCommandQueue->Signal(frame.mFence);
        GetState().mMasterFrame++;
        GetState().mMasterFrameIndex = GetState().mMasterFrame % Gfx::FrameCount::Value;
    }
};

}

// TODO: Implement the index buffer
// TODO: Implement a 'render context' which is basically just a wrapper around command list
// TODO: Implement window creation/swap chain management...

// **********************************
// The graphics device manages resources and state.
// 
// 
// **********************************
class LF_ENGINE_API DX12GfxDevice : public GfxDevice
{
    friend class GraphicsApp; // TODO: Remove
public:
    DX12GfxDevice();
    virtual ~DX12GfxDevice();

    GfxSwapChainAtomicPtr CreateSwapChain(const AppWindowAtomicPtr& window) override;
    GfxFenceAtomicPtr CreateFence() override;
    GfxUploadBufferAtomicPtr CreateConstantBuffer(SizeT elementSize) override ;
    void ReleaseConstantBuffer(const GfxUploadBufferAtomicPtr& buffer) override;
    GfxUploadBufferAtomicPtr CreateStructureBuffer(SizeT elementSize) override;
    void ReleaseStructureBuffer(const GfxUploadBufferAtomicPtr& buffer) override;
    Gfx::FrameCountType GetCurrentFrame() const override;
    Gfx::FrameCountType GetLastCompletedFrame() const override;

    void Register(GfxRenderer* renderer);
    void Unregister(GfxRenderer* renderer);

    APIResult<ServiceResult::Value> OnStart() override;
    APIResult<ServiceResult::Value> OnPostInitialize() override;
    APIResult<ServiceResult::Value> OnBeginFrame() override;
    APIResult<ServiceResult::Value> OnEndFrame() override;
    APIResult<ServiceResult::Value> OnFrameUpdate() override;
    APIResult<ServiceResult::Value> OnShutdown(ServiceShutdownMode mode) override;


    // 
    // APIResult<GfxWindowAdapterAtomicPtr> CreateWindow(const String& title, SizeT width, SizeT height, const String& classID = EMPTY_STRING) override;
    // 
    // bool QueryMappedTypes(const Type* baseType, const Type** implementation, const Type** adapter) override;
    // 
    // APIResult<GfxObjectAtomicPtr> CreateObject(const Type* baseType) override;
    // APIResult<bool> CreateAdapter(GfxObject* object) override;
    // APIResult<GfxAdapterAtomicPtr> CreateAdapter(const Type* baseType);
    // 
    // APIResult<Gfx::ResourcePtr> CreateVertexBuffer(SizeT numElements, SizeT stride, Gfx::BufferUsage usage, const void* initialData = nullptr, SizeT initialDataSize = 0) override;
    // APIResult<Gfx::ResourcePtr> CreateIndexBuffer(SizeT numElements, Gfx::IndexStride stride, Gfx::BufferUsage usage, const void* initialData = nullptr, SizeT initialDataSize = 0) override;
    // 
    // APIResult<bool> CopyVertexBuffer(Gfx::Resource* vertexBuffer, SizeT numElements, SizeT stride, const void* vertexData) override;
    // 
    // APIResult<bool> CopyIndexBuffer(Gfx::Resource* indexBuffer, SizeT numElements, Gfx::IndexStride stride, const void* indexData) override;
    // 
    // void BeginFrame(const GfxWindowAdapterAtomicWPtr& window) override;
    // void EndFrame() override;
    // 
    // void BindPipelineState(GfxMaterialAdapter* adapter) override;
    // void UnbindPipelineState() override;
    // 
    // void BindVertexBuffer(Gfx::Resource* vertexBuffer) override;
    // void UnbindVertexBuffer() override;
    // 
    // void BindIndexBuffer(Gfx::Resource* indexBuffer) override;
    // void UnbindIndexBuffer() override;
    // 
    // void SetViewport(Float32 x, Float32 y, Float32 width, Float32 height, Float32 minDepth, Float32 maxDepth) override;
    // void SetScissorRect(Float32 x, Float32 y, Float32 width, Float32 height) override;
    // 
    // void ClearColor(const Color& color) override;
    // void ClearDepth() override;
    // void SwapBuffers() override;
    // 
    // void Draw(SizeT vertexCount) override;
    // void DrawIndexed(SizeT indexCount) override;
    // 
    // bool IsDebug() const override;
    // 
    // APIResult<Gfx::ResourcePtr> CompileAndLoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines) override;
    // APIResult<Gfx::ResourcePtr> LoadShader(Gfx::ShaderType shaderType, const GfxShaderAsset& shader, const TVector<Token>& defines) override;

    ComPtr<ID3D12Device> Device() { return mDevice; }

protected:
    TAtomicStrongPointer<GfxResourceObject> CreateResourceObject(const Type* type) override;


    GfxFenceAtomicPtr CreateFenceImpl();

    GfxSwapChainAtomicPtr GetOrCreateSwapChain(const AppWindowAtomicPtr& window);
private:
    

    bool WaitForUpdate();
    void DispatchRenderThread();
    void DispatchWorkerThread();
    void RenderThread(AppThread* thread);
    void RenderWorkerThread(AppThread* thread);
    static void RenderAppThread(AppThread* thread);
    static void RenderAppWorkerThread(AppThread* thread);

    void InitAppThread();
    void ShutdownAppThread();
    bool InitDirectX();
    void ShutdownDirectX();

    void CollectGarbage();

    GfxDeviceFlagsBitfield mFlags;
    ComPtr<ID3D12Device>  mDevice;
    ComPtr<IDXGIFactory4> mDeviceFactory;
    TAtomicStrongPointer<DX12GfxCommandQueue> mCommandQueue;
    DX12GfxFactory        mObjectFactory;
    
    // Resource Management:
    using ResourceCommandSet = TUnorderedSet<GfxResourceObjectAtomicPtr, std::hash<GfxResourceObject*>>;
    DX12GfxResourceHeap               mResourceHeap;
    DX12GfxResourceCommandList        mResourceCommandList;
    ResourceCommandSet                mResourceCommandEventListeners;
    SpinLock                          mResourceCommandLock;

    // Window & Render Management:
    
    TVector<GfxState::WindowSwapChain> mWindowSwapChains;
    TVector<GfxSwapChainAtomicPtr> mGarbageSwapChains;
    TVector<GfxRendererAtomicPtr> mRenderers;

    // Threading
    TaskScheduler mRenderThreadScheduler;
    TaskScheduler mRenderWorkerScheduler;
    ThreadDispatcherPtr mRenderThreadDispatcher;
    ThreadDispatcherPtr mRenderWorkerThreadDispatcher;
    ThreadFence    mRenderThreadShutdown;
    ThreadFence    mWorkerThreadShutdown;
    ThreadFence    mPostInitializeFence;

    // State & Task Management
    void InitState();
    void ShutdownState();
    GfxState mGfxState;
    using GfxTaskPtr = TStrongPointer<GfxTask::GfxTaskBase>;
    TVector<GfxTaskPtr> mBeginFrameTasks;
    TVector<GfxTaskPtr> mUpdateFrameTasks;
    TVector<GfxTaskPtr> mEndFrameTasks;
    TVector<GfxTaskPtr> mRenderThreadTasks;

    // Graphics Architecture:

    //             AppThread: Core logic launched from this thread.
    //          RenderThread: Processing logic to prepare and execute the pipeline
    // RenderWorkerThread[N]: Execute the pipeline, submitting work to the GPU
    //  AssetWorkerThread[N]: Communicate with graphics to create/initialize resources.
    // 
    //          RenderThread 
    //                    Idle() -- Can allocate/initialize resources
    //              BeginFrame() -  resources are locked to gpu threads
    //                MidFrame() -  resources are locked to gpu threads
    //                EndFrame() -  resources are locked to gpu threads
    //                    Idle() -- Can allocate/initialize resources
    // 
    // Graphics Resources:
    //          Material
    //          Vertex Buffer
    //          Index Buffer
    //          Texture
    //          Render Texture
    //          
    // Graphics Scene Entities
    //          Model
    //          Effect
    //          Skinned Model
    //          Light
};

} // namespace lf

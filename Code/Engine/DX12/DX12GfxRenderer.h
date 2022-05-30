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
/*
#include "Core/Platform/ThreadFence.h"
#include "Engine/DX12/GfxRenderer.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {

class GfxDependencyContext;

class LF_ENGINE_API DX12GfxRenderer : public GfxRenderer
{
    DECLARE_CLASS(DX12GfxRenderer, GfxRenderer);

    enum class State
    {
        None,
        Ready,
        Rendering,
        Failed
    };

    enum class CommandListType
    {
        BeginCommand,
        MidCommand,
        EndCommand
    };
    enum { NumCommandLists = 3 };
    enum { NumWorkers = 3 }; 
    enum { NumFrames = 3 };

    struct Worker
    {
        ComPtr<ID3D12CommandAllocator> mCommandAllocators[NumCommandLists];
        ComPtr<ID3D12GraphicsCommandList> mCommandLists[NumCommandLists];

        ComPtr<ID3D12CommandAllocator> mShadowCommandAllocator[NumWorkers];
        ComPtr<ID3D12GraphicsCommandList> mShadowCommandLists[NumWorkers];

        ComPtr<ID3D12CommandAllocator> mSceneCommandAllocator[NumWorkers];
        ComPtr<ID3D12GraphicsCommandList> mSceneCommandLists[NumWorkers];

        UInt64 mFenceValue;

        ThreadFence mWorkerSignal;
    };

public:
    virtual ~DX12GfxRenderer();

    bool Initialize(GfxDependencyContext& context);
    void Shutdown();

    void Update();
    void UpdateRenderThread();
    void UpdateRenderWorker();

private:
    bool Failed(HRESULT result);
    void BeginFrame();

    ComPtr<ID3D12Device>             mDevice;
    TStackVector<Worker, NumFrames>  mWorkers;
    Worker*                          mCurrentWorker;


    void SetState(State state) { AtomicStore(&mState, EnumValue(state)); }
    State GetState() const { return static_cast<State>(AtomicLoad(&mState)); }
    volatile Atomic32                mState;
};

} // namespace lf
*/
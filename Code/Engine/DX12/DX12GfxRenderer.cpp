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
#include "DX12GfxRenderer.h"
/*
#include "Core/Reflection/DynamicCast.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"

namespace lf {

DEFINE_CLASS(lf::DX12GfxRenderer) { NO_REFLECTION; }



DX12GfxRenderer::~DX12GfxRenderer()
{}

bool DX12GfxRenderer::Initialize(GfxDependencyContext& context)
{
    DX12GfxDependencyContext* dx12 = DynamicCast<DX12GfxDependencyContext>(&context);
    if (!dx12)
    {
        return false;
    }

    mDevice = dx12->GetDevice();


    return true;
}

void DX12GfxRenderer::Shutdown()
{
    mDevice.Reset();
}

void DX12GfxRenderer::Update()
{
    // Garbage...


}

void DX12GfxRenderer::UpdateRenderThread()
{
    //
    // ForEach Visible Light Render Shadow Map
    // Render Sky Light Shadow Map
    // 
    // Render Geometry to GBuffer
    // 
    // Render Particles
    // 
    // ForEach Visible Light Render 
    //
    // Apply Post Process

    //
    //

    // Initialize our commandlists
    // 

    // Create Pipeline State Object...

}

void DX12GfxRenderer::UpdateRenderWorker()
{

}

bool DX12GfxRenderer::Failed(HRESULT result)
{
    // TODO: Capture Callstack
    // TODO: Record Error
    return FAILED(result);
}

void DX12GfxRenderer::BeginFrame()
{
    
    // Reset command allocators and lists for main thread.
    // for (SizeT i = 0; i < NumCommandLists; ++i)
    // {
    //     if (Failed(mCurrentWorker->mCommandAllocators[i]->Reset()))
    //     {
    //         return;
    //     }
    // 
    //     if (Failed(mCurrentWorker->mCommandLists[i]->Reset(mCurrentWorker->mCommandAllocators[i].Get(), mPipelineState.Get())));
    //     {
    //         return;
    //     }
    // }
}

} // namespace lf 

*/
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
#include "DX12GfxCommandQueue.h"
#include "Core/Reflection/DynamicCast.h"
#include "Engine/DX12/DX12GfxCommandContext.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"
#include "Engine/DX12/DX12GfxFence.h"

namespace lf {
DEFINE_CLASS(lf::DX12GfxCommandQueue) { NO_REFLECTION; }

bool DX12GfxCommandQueue::Initialize(GfxDependencyContext& context)
{
    DX12GfxDependencyContext* dx12 = DynamicCast<DX12GfxDependencyContext>(&context);
    if (!dx12)
    {
        return false;
    }

    ComPtr<ID3D12Device> device = dx12->GetDevice();
    if (!device)
    {
        return false;
    }

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.NodeMask = 0;

    HRESULT hr = device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue));
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}
void DX12GfxCommandQueue::Release()
{
    mCommandQueue.Reset();
}

bool DX12GfxCommandQueue::Execute(GfxCommandContext* context)
{
    if (!mCommandQueue)
    {
        return false;
    }

    DX12GfxCommandContext* dx12Context = DynamicCast<DX12GfxCommandContext>(context);
    if (!dx12Context || dx12Context->CommandList() == nullptr)
    {
        return false;
    }

    ID3D12CommandList* commandLists[1] = { dx12Context->CommandList().Get() };
    mCommandQueue->ExecuteCommandLists(1, commandLists);
    return true;
}

bool DX12GfxCommandQueue::Execute(SizeT numContexts, GfxCommandContext** contexts)
{
    if (!mCommandQueue)
    {
        return false;
    }
    TStackVector<ID3D12CommandList*, 8> commandLists;
    for (SizeT i = 0; i < numContexts; ++i)
    {
        DX12GfxCommandContext* dx12Context = DynamicCast<DX12GfxCommandContext>(contexts[i]);
        if (!dx12Context)
        {
            return false;
        }

        commandLists.push_back(dx12Context->CommandList().Get());
    }

    mCommandQueue->ExecuteCommandLists(static_cast<UINT>(commandLists.size()), commandLists.data());
    return true;
}

bool DX12GfxCommandQueue::Signal(GfxFence* fence)
{
    DX12GfxFence* dx12Fence = DynamicCast<DX12GfxFence>(fence);
    if (!dx12Fence)
    {
        return false;
    }

    UInt64 fenceValue = fence->NextValue();
    mCommandQueue->Signal(dx12Fence->Fence().Get(), fenceValue);
    dx12Fence->Signal();
    // todo:

    return true;
}

} // namespace lf
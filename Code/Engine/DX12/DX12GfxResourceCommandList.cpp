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
#include "DX12GfxResourceCommandList.h"
#include "Core/Reflection/DynamicCast.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"

namespace lf {
DX12GfxResourceCommandList::DX12GfxResourceCommandList()
    : GfxResourceCommandList()
{}
DX12GfxResourceCommandList::~DX12GfxResourceCommandList()
{

}
bool DX12GfxResourceCommandList::Initialize(GfxDependencyContext& context)
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

    mCommandQueue = dx12->GetCommandQueue();
    if (!mCommandQueue)
    {
        return false;
    }


    if (FAILED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator))))
    {
        return false;
    }

    if (FAILED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList))))
    {
        return false;
    }

    if (FAILED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence))))
    {
        return false;
    }

    mFenceValue = 1;
    mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr)
    {
        return false;
    }

    if (FAILED(mCommandList->Close()))
    {
        return false;
    }

    return true;
}
void DX12GfxResourceCommandList::Release()
{
    if (mFenceEvent != nullptr)
    {
        CloseHandle(mFenceEvent);
        mFenceEvent = nullptr;
    }
    mFence.Reset();
    mCommandList.Reset();
    mCommandAllocator.Reset();
    mCommandQueue.Reset();
}
void DX12GfxResourceCommandList::BeginRecord()
{
    Assert(SUCCEEDED(mCommandAllocator->Reset()));
    Assert(SUCCEEDED(mCommandList->Reset(mCommandAllocator.Get(), nullptr)));
}
void DX12GfxResourceCommandList::EndRecord()
{
    Assert(SUCCEEDED(mCommandList->Close()));
    ID3D12CommandList* lists[] = { mCommandList.Get() };
    mCommandQueue->ExecuteCommandLists(1, lists);

    UINT64 fenceValue = InterlockedIncrement(&mFenceValue);
    Assert(SUCCEEDED(mCommandQueue->Signal(mFence.Get(), fenceValue)));
    mFence->SetEventOnCompletion(fenceValue, mFenceEvent);
}
void DX12GfxResourceCommandList::Wait()
{
    WaitForSingleObject(mFenceEvent, INFINITE);
}

} // namespace lf
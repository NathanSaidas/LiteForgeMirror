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
#include "DX12GfxFence.h"

namespace lf {

DEFINE_CLASS(lf::DX12GfxFence) { NO_REFLECTION; }

DX12GfxFence::DX12GfxFence()
: Super()
, mFence()
, mEvent(nullptr)
{

}

DX12GfxFence::~DX12GfxFence()
{
    if (mEvent)
    {
        CloseHandle(mEvent);
        mEvent = nullptr;
    }
}


void DX12GfxFence::WaitImpl()
{
    ScopeLock lock(mWaitLock);
    UInt64 fenceValue = GetFenceValue();
    UInt64 completedValue = mFence->GetCompletedValue();
    if (completedValue < fenceValue)
    {
        mFence->SetEventOnCompletion(fenceValue, mEvent);
        WaitForSingleObject(mEvent, INFINITE);
    }
}

UInt64 DX12GfxFence::GetCompletedValue() const
{
    return mFence->GetCompletedValue();
}

void DX12GfxFence::Initialize(ID3D12Device* device)
{
    Assert(!mEvent);
    device->CreateFence(GetFenceValue(), D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
    mEvent = CreateEventExA(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
}

void DX12GfxFence::Release()
{
    if (mEvent)
    {
        CloseHandle(mEvent);
        mEvent = nullptr;
    }
}

} // namespace lf 
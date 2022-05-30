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
#include "DX12GfxWindow.h"
#include "Core/Utility/Log.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "Engine/DX12/DX12Common.h"
#include "Engine/DX12/DX12GfxDependencyContext.h"

namespace lf {
#if 0

DEFINE_CLASS(lf::DX12GfxWindowAdapter) { NO_REFLECTION; }
DEFINE_CLASS(lf::DX12GfxWindow) { NO_REFLECTION; }

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DX12GfxWindowAdapter* adapter = reinterpret_cast<DX12GfxWindowAdapter*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (adapter)
    {
        return adapter->ProcessMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

DX12GfxWindowAdapter::DX12GfxWindowAdapter()
: Super()
, mPlatformDevice()
, mPlatformDeviceFactory()
, mCommandQueue()
, mCommandAllocator()
, mCommandList()
, mRTVHeap()
, mRTVDescriptionSize(0)
, mRenderTargets()
, mSwapChain()
, mFence()
, mFenceEvent(NULL)
, mFenceValue{ 0,0 }
, mFrameIndex(0)
, mWindowHandle(NULL)
, mClassHandle(NULL)
, mClassName()
, mTitle()
, mVisible(true)
, mWidth(640)
, mHeight(640)
{

}
DX12GfxWindowAdapter::~DX12GfxWindowAdapter()
{

}
void DX12GfxWindowAdapter::OnInitialize(GfxDependencyContext& context)
{
    DX12GfxDependencyContext* dx12Context = DynamicCast<DX12GfxDependencyContext>(&context);
    CriticalAssert(dx12Context);
    mPlatformDevice = dx12Context->GetDevice();
    mPlatformDeviceFactory = dx12Context->GetDeviceFactory();
}
void DX12GfxWindowAdapter::OnShutdown()
{
    Close();
}

bool DX12GfxWindowAdapter::Open()
{
    if (mClassName.Empty())
    {
        LF_LOG_DEBUG(gSysLog, LogMessage("Failed to open window, invalid argument 'className'"));
        return false;
    }

    if (mWindowHandle != NULL)
    {
        LF_LOG_DEBUG(gSysLog, LogMessage("Failed to open window, invalid operation, window is already created.'"));
        return false;
    }

    if (mWidth == 0)
    {
        LF_LOG_DEBUG(gSysLog, LogMessage("Failed to open window, invalid argument 'mWidth'"));
        return false;
    }
    if (mHeight == 0)
    {
        LF_LOG_DEBUG(gSysLog, LogMessage("Failed to open window, invalid argument 'mWidth'"));
        return false;
    }

    if (!CreateClass())
    {
        DestroyClass();
        return false;
    }
    if (!CreateWindow())
    {
        DestroyWindow();
        DestroyClass();
        return false;
    }
    if (!CreateGraphicsResources())
    {
        DestroyGraphicsResources();
        DestroyWindow();
        DestroyClass();
        return false;
    }
    
    ShowWindow(mWindowHandle, IsVisible() ? SW_SHOW : SW_HIDE);
    return true;
}

void DX12GfxWindowAdapter::Close()
{
    DestroyGraphicsResources();
    DestroyWindow();
    DestroyClass();
}

void DX12GfxWindowAdapter::WaitForPreviousFrame()
{
    // NOTE: It's advised to not wait for the previous frame to complete
    // NOTE: If we want to make this thread-safe make sure to add a lock before we begin.
    ReportBug(mSwapChain.Get() != nullptr);
    if (mSwapChain.Get() == nullptr)
    {
        return;
    }
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
    ReportBug(mFence[mFrameIndex].Get() != nullptr);
    if (mFence[mFrameIndex].Get() == nullptr)
    {
        return;
    }
    ReportBug(mFenceEvent != NULL);
    if (mFenceEvent == NULL)
    {
        return;
    }
    WaitForFrame(mFrameIndex);
}

void DX12GfxWindowAdapter::SetTitle(const String& title)
{
    const bool changed = mTitle != title;
    mTitle = title;
    if (changed && IsOpen())
    {
        SetWindowTextA(mWindowHandle, title.CStr());
    }
}
void DX12GfxWindowAdapter::SetVisible(bool visible)
{
    const bool changed = mVisible != visible;
    mVisible = visible;
    if (changed && IsOpen())
    {
        ShowWindow(mWindowHandle, mVisible ? SW_SHOW : SW_HIDE);
    }
}
void DX12GfxWindowAdapter::SetWidth(Int32 width)
{
    const bool changed = mWidth != width;
    mWidth = width;
    if (changed && IsOpen())
    {
        // Window Coords
        // (0,0) 
        //      (1,1)
        //
        // Windows use RECTs so we must get the current rect for it's position and adjust the 'right'
        // and then correct the RECT because it does not take the full area into account.
        RECT clientRect = { 0 };
        GetClientRect(mWindowHandle, &clientRect);
        clientRect.right = clientRect.left + width;
        AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);
        RECT windowRect = { 0 };
        GetWindowRect(mWindowHandle, &windowRect);

        SetWindowPos(mWindowHandle, nullptr, windowRect.left, windowRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, 0);
    }
}
void DX12GfxWindowAdapter::SetHeight(Int32 height)
{
    const bool changed = mHeight != height;
    mHeight = height;
    if (changed && IsOpen())
    {
        // Window Coords
        // (0,0) 
        //      (1,1)
        //
        // Windows use RECTs so we must get the current rect for it's position and adjust the 'bottom'
        // and then correct the RECT because it does not take the full area into account.
        RECT clientRect = { 0 };
        GetClientRect(mWindowHandle, &clientRect);
        clientRect.bottom = clientRect.top + height;
        AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);
        RECT windowRect = { 0 };
        GetWindowRect(mWindowHandle, &windowRect);

        SetWindowPos(mWindowHandle, nullptr, windowRect.left, windowRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, 0);
    }
}

void DX12GfxWindowAdapter::SetClassName(const String& className)
{
    if (!IsOpen())
    {
        mClassName = className;
    }
}

Rect DX12GfxWindowAdapter::GetRect() const
{
    if (!IsOpen())
    {
        return Rect();
    }

    RECT windowRect;
    GetClientRect(mWindowHandle, &windowRect);

    RECT globalRect;
    GetWindowRect(mWindowHandle, &globalRect);

    Rect r;
    r.x = static_cast<Float32>(globalRect.left);
    r.y = static_cast<Float32>(globalRect.top);
    r.width = static_cast<Float32>(windowRect.right - windowRect.left);
    r.height = static_cast<Float32>(windowRect.bottom - windowRect.top);
    return r;
}

LRESULT DX12GfxWindowAdapter::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE:
        {

            DestroyGraphicsResources();
            DestroyWindow();
            DestroyClass();
        } break;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

bool DX12GfxWindowAdapter::CreateClass()
{
    WNDCLASSEX windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = mClassName.CStr();
    if (RegisterClassExA(&windowClass) == 0)
    {
        return false;
    }
    mClassHandle = windowClass.hInstance;
    return true;
}

bool DX12GfxWindowAdapter::CreateWindow()
{
    Assert(mClassHandle != INVALID_HANDLE_VALUE);
    Int32 screenWidth = GetSystemMetrics(SM_CXSCREEN);
    Int32 screenHeight = GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0 };
    // TODO: Implement fullscreen for DX12GfxWindow
    // if (mFullscreen)
    // {
    //     windowRect.left = 0;
    //     windowRect.top = 0;
    //     windowRect.right = screenWidth;
    //     windowRect.bottom = screenHeight;
    // }
    // else
    {
        windowRect.left = (screenWidth / 2) - (mWidth / 2);
        windowRect.top = (screenHeight / 2) - (mHeight / 2);
        windowRect.right = windowRect.left + mWidth;
        windowRect.bottom = windowRect.top + mHeight;
    }

    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    mWindowHandle = CreateWindowA(
        mClassName.CStr(),
        mTitle.CStr(),
        WS_OVERLAPPEDWINDOW,
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        mClassHandle,
        nullptr);
    if (mWindowHandle == NULL)
    {
        return false;
    }

    SetWindowLongPtrA(mWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    // ShowCursor(TRUE);
    // Show after graphics creation
    ShowWindow(mWindowHandle, SW_HIDE);
    return true;
}

bool DX12GfxWindowAdapter::CreateGraphicsResources()
{
    // Create the command queue
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    if (GetDevice()->IsDebug())
    {
        queueDesc.Flags |= D3D12_COMMAND_QUEUE_FLAG_DISABLE_GPU_TIMEOUT;
    }
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.NodeMask = 0;
    if (FAILED(mPlatformDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue))))
    {
        return false;
    }

    // Create swap chain
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { };
    swapChainDesc.BufferCount = FRAME_COUNT;
    swapChainDesc.Width = mWidth;
    swapChainDesc.Height = mHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    if ((FAILED(mPlatformDeviceFactory->CreateSwapChainForHwnd(
        mCommandQueue.Get(),
        mWindowHandle,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain))))
    {
        return false;
    }

    if (FAILED(swapChain.As(&mSwapChain)))
    {
        return false;
    }

    // Create descriptor heaps.
    {
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = FRAME_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        if (FAILED(mPlatformDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRTVHeap))))
        {
            return false;
        }
        mRTVDescriptionSize = mPlatformDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resource
    {
        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRTVHeap->GetCPUDescriptorHandleForHeapStart());

        for (UInt32 n = 0; n < FRAME_COUNT; ++n)
        {
            if (FAILED(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n]))))
            {
                return false;
            }

            mPlatformDevice->CreateRenderTargetView(mRenderTargets[n].Get(), nullptr, rtvHandle);
            rtvHandle.ptr += 1 * mRTVDescriptionSize;
        }
    }

    // Create command allocator
    for (SizeT i = 0; i < FRAME_COUNT; ++i)
    {
        if(FAILED(mPlatformDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator[i]))))
        {
            return false;
        }
    }

    if (FAILED(mPlatformDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator[0].Get(), nullptr, IID_PPV_ARGS(&mCommandList))))
    {
        return false;
    }
     
    // Command lists are created in the recording state, we need to Close them as other resources will expect it to be closed.
    if (FAILED(mCommandList->Close()))
    {
        return false;
    }

    // Create fence resources
    for (SizeT i = 0; i < FRAME_COUNT; ++i)
    {
        if (FAILED(mPlatformDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence[i]))))
        {
            return false;
        }
        mFenceValue[i] = 0;
    }
    mFenceEvent = CreateEventA(nullptr, FALSE, FALSE, nullptr);
    if (mFenceEvent == nullptr)
    {
        return false;
    }
    mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

    return true;
}

void DX12GfxWindowAdapter::DestroyClass()
{
    Assert(mWindowHandle == NULL);
    if (mClassHandle != NULL)
    {
        UnregisterClassA(mClassName.CStr(), mClassHandle);
        mClassHandle = NULL;
        mClassName.Clear();
    }

}
void DX12GfxWindowAdapter::DestroyWindow()
{
    // Ensure we've called DestroyGraphicsResources first.
    Assert(mSwapChain.Get() == nullptr);
    for (SizeT n = 0; n < FRAME_COUNT; ++n)
    {
        Assert(mRenderTargets[n].Get() == nullptr);
        Assert(mCommandAllocator[n].Get() == nullptr);
    }
    Assert(mRTVHeap.Get() == nullptr);
    Assert(mCommandQueue.Get() == nullptr);
    Assert(mCommandList.Get() == nullptr);

    if (mWindowHandle != NULL)
    {
        ::DestroyWindow(mWindowHandle);
        mWindowHandle = NULL;
    }
}
void DX12GfxWindowAdapter::DestroyGraphicsResources()
{
    if (mFenceEvent != NULL) // Hacky check to make sure we've created our fence resources. (eg Partial Window Creation)
    {
        WaitForFrame(0);
        WaitForFrame(1);
    }

    mCommandList.Reset();
    for (SizeT n = 0; n < FRAME_COUNT; ++n)
    {
        mCommandAllocator[n].Reset();
        mRenderTargets[n].Reset();
        mFence[n].Reset();
        mFenceValue[n] = 0;
    }
    mFrameIndex = 0;
    CloseHandle(mFenceEvent);
    mRTVDescriptionSize = 0;
    mRTVHeap.Reset();
    mCommandQueue.Reset();
    mSwapChain.Reset();
}

void DX12GfxWindowAdapter::WaitForFrame(SizeT frameIndex)
{
    Assert(frameIndex < FRAME_COUNT);

    const UInt64 fence = mFenceValue[frameIndex];
    mCommandQueue->Signal(mFence[frameIndex].Get(), fence);
    ++mFenceValue[frameIndex];

    if (mFence[frameIndex]->GetCompletedValue() < fence)
    {
        if (FAILED(mFence[frameIndex]->SetEventOnCompletion(fence, mFenceEvent)))
        {
            // This might be ok?
            ReportBugMsg("Unexpected failure calling SetEventOnCompletion.");
        }
        else
        {
            WaitForSingleObject(mFenceEvent, INFINITE);
        }
    }
    // if (mFence[frameIndex]->GetCompletedValue() < mFenceValue[frameIndex])
    // {
    //     if (FAILED(mFence[frameIndex]->SetEventOnCompletion(mFenceValue[frameIndex], mFenceEvent)))
    //     {
    //         // This might be ok?
    //         ReportBugMsg("Unexpected failure calling SetEventOnCompletion.");
    //     }
    //     else
    //     {
    //         WaitForSingleObject(mFenceEvent, INFINITE);
    //     }
    // }
    // mFenceValue[frameIndex]++;
}

DX12GfxWindow::DX12GfxWindow()
: Super()
, mClassName()
, mTitle()
, mVisible(true)
, mWidth(640)
, mHeight(640)
{

}
DX12GfxWindow::~DX12GfxWindow()
{

}

bool DX12GfxWindow::Open()
{
    DX12GfxWindowAdapter* adapter = GetAdapterAs<DX12GfxWindowAdapter>();
    return adapter->Open();
}

// ** name
void DX12GfxWindow::SetName(const String& value)
{
    mTitle = value;

    DX12GfxWindowAdapter* adapter = GetAdapterAs<DX12GfxWindowAdapter>();
    adapter->SetTitle(mTitle);
}
const String& DX12GfxWindow::GetName() const
{
    return GetAdapterAs<DX12GfxWindowAdapter>()->GetTitle();
}

// ** width
void DX12GfxWindow::SetWidth(SizeT value)
{
    mWidth = value;
    GetAdapterAs<DX12GfxWindowAdapter>()->SetWidth(static_cast<Int32>(mWidth));
}
SizeT DX12GfxWindow::GetWidth() const
{
    return GetAdapterAs<DX12GfxWindowAdapter>()->GetWidth();
}

// ** height
void DX12GfxWindow::SetHeight(SizeT value)
{
    mHeight = value;
    GetAdapterAs<DX12GfxWindowAdapter>()->SetHeight(static_cast<Int32>(mHeight));
}
SizeT DX12GfxWindow::GetHeight() const
{
    return GetAdapterAs<DX12GfxWindowAdapter>()->GetHeight();
}

Rect DX12GfxWindow::GetRect() const
{
    return GetAdapterAs<DX12GfxWindowAdapter>()->GetRect();
}

// ** fullscreen
void DX12GfxWindow::SetFullscreen(bool value)
{
    (value);
    ReportBugMsg("Missing implementation. TODO: Implement fullscreen for DX12GfxWindow");
}

bool DX12GfxWindow::IsFullscreen() const
{
    ReportBugMsg("Missing implementation. TODO: Implement fullscreen for DX12GfxWindow");
    return false;
}

// ** visible
bool DX12GfxWindow::IsVisible() const
{
    return GetAdapterAs<DX12GfxWindowAdapter>()->IsVisible();
}
// ** created
bool DX12GfxWindow::IsCreated() const
{
    return GetAdapterAs<DX12GfxWindowAdapter>()->IsOpen();
}

void DX12GfxWindow::SetClassName(const String& value)
{
    mClassName = value;
}

#endif


} // namespace lf
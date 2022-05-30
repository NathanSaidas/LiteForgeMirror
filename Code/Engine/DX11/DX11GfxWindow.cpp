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
#include "DX11GfxWindow.h"
#include "Core/Math/MathFunctions.h"
#include "Core/Math/Color.h"
#include "Core/Input/KeyboardEvents.h"
#include "Core/Input/MouseEvents.h"
#include "Core/Input/USB.h"
#include "Core/String/StringCommon.h"
#include "Core/Utility/Log.h"
#include "Runtime/Service/Service.h"
#include "AbstractEngine/Input/InputMgr.h"
#include "AbstractEngine/Input/MouseDevice.h"
#include "AbstractEngine/Input/KeyboardDevice.h"
#include "Engine/DX11/DX11GfxDependencyContext.h"

#include <windowsx.h>

namespace lf {

#if defined(LF_DIRECTX11)
DEFINE_CLASS(lf::DX11GfxWindowAdapter) { NO_REFLECTION; }
DEFINE_CLASS(lf::DX11GfxWindow) { NO_REFLECTION; }

const UINT RAW_INPUT_ERROR = (UINT)-1;

LRESULT CALLBACK DX11WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    DX11GfxWindowAdapter* adapter = reinterpret_cast<DX11GfxWindowAdapter*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (adapter)
    {
        return adapter->ProcessMessage(hwnd, message, wParam, lParam);
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

struct DisplayMode
{
    DisplayMode() : width(0), height(0), refreshNumerator(0), refreshDenominator(0) {}
    DisplayMode(SizeT inWidth, SizeT inHeight, SizeT inRefreshNumerator, SizeT inRefreshDenominator) :
        width(inWidth),
        height(inHeight),
        refreshNumerator(inRefreshNumerator),
        refreshDenominator(inRefreshDenominator)
    {
    }

    SizeT width;
    SizeT height;
    SizeT refreshNumerator;
    SizeT refreshDenominator;
};
typedef TArray<DisplayMode> DisplayModes;

struct DisplayInfo
{
    DisplayInfo() : displayModes(), bestDisplayModeId(0), vram(0), videoCardDescription() {}

    DisplayModes displayModes;
    String videoCardDescription;
    SizeT bestDisplayModeId;
    SizeT vram;
};

// Get a list of display infos and also find the best DisplayMode given a target.
static SizeT GetDisplayInfo(DisplayInfo& displayInfo, const DisplayMode& targetMode)
{
    const DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
    const UINT flags = DXGI_ENUM_MODES_INTERLACED;

    HRESULT result;

    ComPtr<IDXGIFactory> factory;
    result = CreateDXGIFactory(IID_PPV_ARGS(&factory));
    if (FAILED(result))
    {
        gGfxLog.Error(LogMessage("Failed to GetDisplayInfo. (CreateDXGIFactory)"));
        return INVALID;
    }

    // Find Video Card
    ComPtr<IDXGIAdapter> adapter;
    result = factory->EnumAdapters(0, adapter.ReleaseAndGetAddressOf());
    if (FAILED(result))
    {
        gGfxLog.Error(LogMessage("Failed to GetDisplayInfo. (EnumAdapters)"));
        return INVALID;
    }

    ComPtr<IDXGIOutput> adapterOutput;
    result = adapter->EnumOutputs(0, adapterOutput.ReleaseAndGetAddressOf());
    if (FAILED(result))
    {
        gGfxLog.Error(LogMessage("Failed to GetDisplayInfo. (EnumOutputs)"));
        return INVALID;
    }

    UINT numModes = 0;
    result = adapterOutput->GetDisplayModeList(format, flags, &numModes, NULL);
    if (FAILED(result))
    {
        gGfxLog.Error(LogMessage("Failed to GetDisplayInfo. (GetDisplayModeList.1)"));
        return INVALID;
    }

    TStackVector<DXGI_MODE_DESC, 16> displayModeList;
    displayModeList.resize(numModes);

    result = adapterOutput->GetDisplayModeList(format, flags, &numModes, displayModeList.data());
    if (FAILED(result))
    {
        gGfxLog.Error(LogMessage("Failed to GetDisplayInfo. (GetDisplayModeList.2)"));
        return INVALID;
    }

    displayInfo.bestDisplayModeId = 0;
    SizeT bestScore = SIZE_MAX;
    for (UINT i = 0; i < numModes; ++i)
    {
        DisplayMode dm(
            displayModeList[i].Width,
            displayModeList[i].Height,
            displayModeList[i].RefreshRate.Numerator,
            displayModeList[i].RefreshRate.Denominator
        );
        displayInfo.displayModes.Add(dm);

        SizeT score = Abs(static_cast<Int32>(dm.width) - static_cast<Int32>(targetMode.width));
        score += Abs(static_cast<Int32>(dm.height) - static_cast<Int32>(targetMode.height));
        if (score < bestScore)
        {
            displayInfo.bestDisplayModeId = i;
            bestScore = score;
        }
    }

    DXGI_ADAPTER_DESC adapterDesc;
    result = adapter->GetDesc(&adapterDesc);
    if (FAILED(result))
    {
        gGfxLog.Error(LogMessage("Failed to GetDisplayInfo. (GetDesc)"));
        return false;
    }
    displayInfo.vram = static_cast<SizeT>(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

    WString wdesc = adapterDesc.Description;
    displayInfo.videoCardDescription = StrConvert(wdesc);
    return true;
}

DX11GfxWindowAdapter::DX11GfxWindowAdapter()
: Super()
, mPlatformDevice()
, mPlatformDeviceContext()
, mRenderTargetView()
, mSwapChain()
, mWindowHandle(NULL)
, mClassHandle(NULL)
, mClassName()
, mTitle()
, mVisible(true)
, mWidth(640)
, mHeight(640)
, mInputService(nullptr)
{

}
DX11GfxWindowAdapter::~DX11GfxWindowAdapter()
{

}
void DX11GfxWindowAdapter::OnInitialize(GfxDependencyContext& context)
{
    DX11GfxDependencyContext* dx11Context = DynamicCast<DX11GfxDependencyContext>(&context);
    CriticalAssert(dx11Context);
    mPlatformDevice = dx11Context->GetDevice();
    mPlatformDeviceContext = dx11Context->GetDeviceContext();
    mInputService = context.GetServices()->GetService<InputMgr>();
}
void DX11GfxWindowAdapter::OnShutdown()
{
    Close();
}

bool DX11GfxWindowAdapter::Open()
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

    InitializeInput();

    ShowWindow(mWindowHandle, IsVisible() ? SW_SHOW : SW_HIDE);
    return true;
}

void DX11GfxWindowAdapter::Close()
{
    DestroyGraphicsResources();
    DestroyWindow();
    DestroyClass();
}

void DX11GfxWindowAdapter::SetTitle(const String& title)
{
    const bool changed = mTitle != title;
    mTitle = title;
    if (changed && IsOpen())
    {
        SetWindowTextA(mWindowHandle, title.CStr());
    }
}
void DX11GfxWindowAdapter::SetVisible(bool visible)
{
    const bool changed = mVisible != visible;
    mVisible = visible;
    if (changed && IsOpen())
    {
        ShowWindow(mWindowHandle, mVisible ? SW_SHOW : SW_HIDE);
    }
}
void DX11GfxWindowAdapter::SetWidth(Int32 width)
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
void DX11GfxWindowAdapter::SetHeight(Int32 height)
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

void DX11GfxWindowAdapter::SetClassName(const String& className)
{
    if (!IsOpen())
    {
        mClassName = className;
    }
}


Rect DX11GfxWindowAdapter::GetRect() const
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

LRESULT DX11GfxWindowAdapter::ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE:
        {
            DestroyGraphicsResources();
            DestroyWindow();
            DestroyClass();
        } break;
        case WM_SIZE:
        {
            mWidth = LOWORD(lParam);
            mHeight = HIWORD(lParam);
        } break;
        case WM_KEYDOWN:
        {
            if (mInputService)
            {
                KeyboardDeviceAtomicPtr keyboard = GetWindowKeyboard();
                if (keyboard)
                {
                    InputCode code = keyboard->VirtualKeyToCode(static_cast<Int32>(wParam));
                    if (code != InputCode::NONE)
                    {
                        keyboard->ReportPress(code);
                    }
                    else
                    {
                        gSysLog.Warning(LogMessage("Unknown virtual keyboard input skipped! ") << wParam);
                    }
                }
            }

        } break;
        case WM_KEYUP:
        {
            if (mInputService)
            {
                KeyboardDeviceAtomicPtr keyboard = GetWindowKeyboard();
                if (keyboard)
                {
                    InputCode code = keyboard->VirtualKeyToCode(static_cast<Int32>(wParam));
                    if (code != InputCode::NONE)
                    {
                        keyboard->ReportRelease(code);
                    }
                    else
                    {
                        gSysLog.Warning(LogMessage("Unknown virtual keyboard input skipped! ") << wParam);
                    }
                }
            }
        } break;
        case WM_MOUSEMOVE:
        {
            if (mInputService)
            {
                MouseDeviceAtomicPtr windowMouse = GetWindowMouse();
                if (windowMouse)
                {
                    Int32 x = static_cast<Int32>(GET_X_LPARAM(lParam));
                    Int32 y = static_cast<Int32>(GET_Y_LPARAM(lParam));
                    mWindowMouse->ReportCursorPosition(x, y, GetAtomicPointer(this));
                }
            }

        } break;
        case WM_MBUTTONDOWN:
        {
            gSysLog.Info(LogMessage("Middle Mouse Button Down!"));
        } break;
        case WM_MBUTTONUP:
        {
            gSysLog.Info(LogMessage("Middle Mouse Button Up!"));
        } break;
        case WM_INPUT:
        {
            TArray<ByteT> inputData;
            UINT dwSize = 0;
            GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
            inputData.Resize(static_cast<SizeT>(dwSize));

            if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, inputData.GetData(), &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
            {
                break;
            }

            RAWINPUT* input = reinterpret_cast<RAWINPUT*>(inputData.GetData());
            if (input->header.dwType == RIM_TYPEKEYBOARD)
            {
                gSysLog.Info(LogMessage("Processing WM_INPUT.Keyboard"));

            }
            else if (input->header.dwType == RIM_TYPEMOUSE)
            {
                HANDLE handle = input->header.hDevice;
                ULONG flags = input->data.mouse.ulButtons;
                bool processed = false;
                // same as 1
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_LEFT_BUTTON_DOWN, InputCode::MOUSE_BUTTON_LEFT, InputEventType::BUTTON_PRESSED);
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_LEFT_BUTTON_UP, InputCode::MOUSE_BUTTON_LEFT, InputEventType::BUTTON_RELEASED);

                // same as 2       
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_RIGHT_BUTTON_DOWN, InputCode::MOUSE_BUTTON_RIGHT, InputEventType::BUTTON_PRESSED);
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_RIGHT_BUTTON_UP, InputCode::MOUSE_BUTTON_RIGHT, InputEventType::BUTTON_RELEASED);

                // same as 3      
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_MIDDLE_BUTTON_DOWN, InputCode::MOUSE_BUTTON_MIDDLE, InputEventType::BUTTON_PRESSED);
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_MIDDLE_BUTTON_UP, InputCode::MOUSE_BUTTON_MIDDLE, InputEventType::BUTTON_RELEASED);

                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_BUTTON_4_DOWN, InputCode::MOUSE_AUX_BUTTON_1, InputEventType::BUTTON_PRESSED);
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_BUTTON_4_UP, InputCode::MOUSE_AUX_BUTTON_1, InputEventType::BUTTON_RELEASED);

                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_BUTTON_5_DOWN, InputCode::MOUSE_AUX_BUTTON_2, InputEventType::BUTTON_PRESSED);
                processed |= ProcessMouseButton(handle, flags & RI_MOUSE_BUTTON_5_UP, InputCode::MOUSE_AUX_BUTTON_2, InputEventType::BUTTON_RELEASED);

                LONG lastX = input->data.mouse.lLastX;
                LONG lastY = input->data.mouse.lLastY;
                if (lastX != 0 || lastY != 0)
                {
                    MouseDeviceAtomicPtr device = GetMouseDevice(handle);
                    if (device)
                    {
                        device->ReportCursorDelta(static_cast<Int32>(lastX), static_cast<Int32>(lastY));
                        processed = true;
                    }
                }


                if (!processed)
                {
                    gSysLog.Info(LogMessage("Unprocessed mouse input!"));
                }
            }
            else
            {

            }


        } break;
    }

    return DefWindowProcA(hwnd, message, wParam, lParam);
}

bool DX11GfxWindowAdapter::CreateClass()
{
    WNDCLASSEX windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = DX11WindowProc;
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

bool DX11GfxWindowAdapter::CreateWindow()
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

bool DX11GfxWindowAdapter::CreateGraphicsResources()
{
    DisplayInfo displayInfo;
    DisplayMode displayMode(mWidth, mHeight, 0, 0);
    SizeT bestMode = GetDisplayInfo(displayInfo, displayMode);
    Assert(Valid(bestMode));

    // // Get Factory:
    ComPtr<IDXGIDevice> device;
    Assert(SUCCEEDED(mPlatformDevice->QueryInterface(IID_PPV_ARGS(&device))));
    ComPtr<IDXGIAdapter> adapter;
    Assert(SUCCEEDED(device->GetParent(IID_PPV_ARGS(&adapter))));
    ComPtr<IDXGIFactory> factory;
    Assert(SUCCEEDED(adapter->GetParent(IID_PPV_ARGS(&factory))));

    // Initialize Device and SwapChain:
    DXGI_SWAP_CHAIN_DESC swd;
    ZeroMemory(&swd, sizeof(swd));

    // Back buffer setup
    swd.BufferCount = 1;
    swd.BufferDesc.Width = static_cast<UINT>(mWidth);
    swd.BufferDesc.Height = static_cast<UINT>(mHeight);
    swd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

    //if (window->IsVsyncEnabled())
    //{
    //    swd.BufferDesc.RefreshRate.Numerator = static_cast<UINT>(displayInfo.displayModes[bestMode].refreshNumerator);
    //    swd.BufferDesc.RefreshRate.Denominator = static_cast<UINT>(displayInfo.displayModes[bestMode].refreshDenominator);
    //}
    //else
    {
        swd.BufferDesc.RefreshRate.Numerator = 0;
        swd.BufferDesc.RefreshRate.Denominator = 1;
    }

    swd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swd.OutputWindow = mWindowHandle;
    swd.SampleDesc.Count = 1;
    swd.SampleDesc.Quality = 0;

    swd.Windowed = TRUE; //  !window->IsFullscreen();

    swd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    swd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    swd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
    swd.Flags = 0;

    Assert(SUCCEEDED(factory->CreateSwapChain(mPlatformDevice.Get(), &swd, mSwapChain.ReleaseAndGetAddressOf())));

    ComPtr<ID3D11Texture2D> backBufferPtr;
    Assert(SUCCEEDED(mSwapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferPtr))));

    Assert(SUCCEEDED(mPlatformDevice->CreateRenderTargetView(backBufferPtr.Get(), NULL, mRenderTargetView.ReleaseAndGetAddressOf())));

    // TODO: This depth texture/view shouldn't be created in the material, but rather we should bind to in the rendering part.
    // TOOD: Move creation of this to Window?
    D3D11_TEXTURE2D_DESC texDesc;
    ZeroMemory(&texDesc, sizeof(texDesc));

    texDesc.Width = static_cast<UINT>(mWidth);
    texDesc.Height = static_cast<UINT>(mHeight);
    texDesc.MipLevels = 1; // Depth Buffer only needs 1 mip level.
    texDesc.ArraySize = 1; // Depth Buffer only needs 1 texture.
                           // Acceptable Format Types
                           // DXGI_FORMAT_D32_FLOAT_S8X24_UINT - 32 Bit, 8 Bits (UINT) for stencil + 24 bits for padding.
                           // DXGI_FORMAT_D32_FLOAT - 32 Bit, floating point buffer.
                           // DXGI_FORMAT_D24_UNORM_S8_UINT - 32 Bit, 24 Bits normalized values for depth [0...1] with 8 bits for stencil.)
                           // DXGI_FORMAT_D16_UNORM - 16 Bit, 16 Bits normalized values for depth buffer [0...1] NO STENCIL.
    texDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

    // Use this method to calculate Quality for SampleCount & Format
    // HRESULT ID3D11Device::CheckMultisampleQualityLevels( DXGI_FORMAT Format, UINT SampleCount, UINT *pNumQualityLevels); 
    // Sample count of 4 or 8 is common balance between quality and performance
    // No multisampling is { Count = 1, Quality = 0 }
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    // Use default/standard depth texture settings.
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    Assert(SUCCEEDED(mPlatformDevice->CreateTexture2D(&texDesc, NULL, mDepthTexture.ReleaseAndGetAddressOf())));


    D3D11_DEPTH_STENCIL_VIEW_DESC viewDesc;
    ZeroMemory(&viewDesc, sizeof(viewDesc));
    viewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    viewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    viewDesc.Texture2D.MipSlice = 0; // This is probably redundant with ZeroMemory and all.

    Assert(SUCCEEDED(mPlatformDevice->CreateDepthStencilView(mDepthTexture.Get(), &viewDesc, mDepthView.ReleaseAndGetAddressOf())));

    // TODO: Make Current:
    // if (mWindows.Empty())
    // {
    //     mDeviceContext->OMSetRenderTargets(1, &renderTargetView, nullptr);
    // 
    //     mActiveWindowIndex = mWindows.Size();
    //     mActiveViewWidth = window->GetWidth();
    //     mActiveViewHeight = window->GetHeight();
    //     SetViewport(0.0f, 0.0f, static_cast<Float32>(window->GetWidth()), static_cast<Float32>(window->GetHeight()), 0.0f, 1.0f);
    // }

    return true;
}

void DX11GfxWindowAdapter::DestroyClass()
{
    Assert(mWindowHandle == NULL);
    if (mClassHandle != NULL)
    {
        UnregisterClassA(mClassName.CStr(), mClassHandle);
        mClassHandle = NULL;
        mClassName.Clear();
    }

}
void DX11GfxWindowAdapter::DestroyWindow()
{
    // Ensure we've called DestroyGraphicsResources first.
    Assert(mSwapChain.Get() == nullptr);
    Assert(mRenderTargetView.Get() == nullptr);

    if (mWindowHandle != NULL)
    {
        ::DestroyWindow(mWindowHandle);
        mWindowHandle = NULL;
    }
}
void DX11GfxWindowAdapter::DestroyGraphicsResources()
{
    mDepthView.Reset();
    mDepthTexture.Reset();
    mRenderTargetView.Reset();
    mSwapChain.Reset();
}

void DX11GfxWindowAdapter::InitializeInput()
{
    if (!mInputService)
    {
        return;
    }

    GetWindowKeyboard();
    GetWindowMouse();

    TStackVector<RAWINPUTDEVICE, 2> devices;

    RAWINPUTDEVICE device;
    device.hwndTarget = mWindowHandle;
    device.usUsagePage = USB::UsagePage::USAGE_PAGE_GENERIC_DESKTOP_CONTROLS;
    device.usUsage = USB::UsageIdGenericDesktopControls::USAGE_ID_MOUSE;
    device.dwFlags = 0;
    devices.push_back(device);

    // device.hwndTarget = mWindowHandle;
    // device.usUsagePage = USB::UsagePage::USAGE_PAGE_GENERIC_DESKTOP_CONTROLS;
    // device.usUsage = USB::UsageIdGenericDesktopControls::USAGE_ID_KEYBOARD;
    // device.dwFlags = 0;
    // devices.Add(device);

    if (RegisterRawInputDevices(devices.data(), static_cast<UINT>(devices.size()), sizeof(RAWINPUTDEVICE)) == FALSE)
    {
        gSysLog.Error(LogMessage("Failed to register raw input devices for window. LastError=") << (int)GetLastError());
    }
}

bool DX11GfxWindowAdapter::ProcessMouseButton(HANDLE handle, ULONG bit, InputCode code, InputEventType eventType)
{
    if (!(bit > 0))
    {
        return false;
    }

    MouseDeviceAtomicPtr device = GetMouseDevice(handle);
    if (!device)
    {
        return false;
    }

    if (eventType == InputEventType::BUTTON_PRESSED)
    {
        device->ReportPress(code, GetAtomicPointer(this));
    }
    else
    {
        device->ReportRelease(code, GetAtomicPointer(this));
    }
    return true;
}

MouseDeviceAtomicPtr DX11GfxWindowAdapter::GetMouseDevice(HANDLE handle)
{
    UInt64 handleId = reinterpret_cast<UInt64>(handle);
    auto iter = mMouseDevices.find(handleId);
    if (iter != mMouseDevices.end())
    {
        return iter->second;
    }
    else
    {
        UINT nameLength = 0;
        GetRawInputDeviceInfoA(handle, RIDI_DEVICENAME, NULL, &nameLength);

        String deviceName;
        deviceName.Resize(nameLength);
        if (GetRawInputDeviceInfoA(handle, RIDI_DEVICENAME, const_cast<char*>(deviceName.CStr()), &nameLength) == RAW_INPUT_ERROR)
        {
            return NULL_PTR;
        }
        Token deviceNameToken(deviceName);

        for (auto& it : mMouseDevices)
        {
            MouseDeviceAtomicPtr device = it.second;
            if (device && device->GetDeviceName() == deviceNameToken)
            {
                // Device lost connection?
                device->Disconnect();
                device->Connect(mInputService);
                return device;
            }
        }

        MouseDeviceAtomicPtr mouse = MakeConvertibleAtomicPtr<MouseDevice>();
        if (!mouse->Connect(mInputService))
        {
            return false;
        }
        mouse->SetDeviceName(deviceNameToken);
        mMouseDevices[handleId] = mouse;
        return mouse;
    }
}

MouseDeviceAtomicPtr DX11GfxWindowAdapter::GetWindowMouse()
{
    MouseDeviceAtomicPtr mouse = mWindowMouse;
    if (!mouse)
    {
        mouse = MakeConvertibleAtomicPtr<MouseDevice>();
        if (mouse->Connect(mInputService))
        {
            mWindowMouse = mouse;
            mWindowMouse->SetDeviceName(Token("__WINDOW_MOUSE"));
        }
        else
        {
            mouse = NULL_PTR;
        }
    }
    return mouse;
}
KeyboardDeviceAtomicPtr DX11GfxWindowAdapter::GetWindowKeyboard()
{
    KeyboardDeviceAtomicPtr keyboard = mWindowKeyboard;
    if (!keyboard)
    {
        keyboard = MakeConvertibleAtomicPtr<KeyboardDevice>();
        if (keyboard->Connect(mInputService))
        {
            mWindowKeyboard = keyboard;
            mWindowKeyboard->SetDeviceName(Token("__WINDOW_KEYBOARD"));
        }
        else
        {
            keyboard = NULL_PTR;
        }
    }
    return keyboard;
}

DX11GfxWindow::DX11GfxWindow()
: Super()
, mClassName()
, mTitle()
, mVisible(true)
, mWidth(640)
, mHeight(640)
{

}
DX11GfxWindow::~DX11GfxWindow()
{

}

bool DX11GfxWindow::Open()
{
    DX11GfxWindowAdapter* adapter = GetAdapterAs<DX11GfxWindowAdapter>();
    return adapter->Open();
}

// ** name
void DX11GfxWindow::SetName(const String& value)
{
    mTitle = value;

    DX11GfxWindowAdapter* adapter = GetAdapterAs<DX11GfxWindowAdapter>();
    adapter->SetTitle(mTitle);
}
const String& DX11GfxWindow::GetName() const
{
    return GetAdapterAs<DX11GfxWindowAdapter>()->GetTitle();
}

// ** width
void DX11GfxWindow::SetWidth(SizeT value)
{
    mWidth = value;
    GetAdapterAs<DX11GfxWindowAdapter>()->SetWidth(static_cast<Int32>(mWidth));
}
SizeT DX11GfxWindow::GetWidth() const
{
    return GetAdapterAs<DX11GfxWindowAdapter>()->GetWidth();
}

// ** height
void DX11GfxWindow::SetHeight(SizeT value)
{
    mHeight = value;
    GetAdapterAs<DX11GfxWindowAdapter>()->SetHeight(static_cast<Int32>(mHeight));
}
SizeT DX11GfxWindow::GetHeight() const
{
    return GetAdapterAs<DX11GfxWindowAdapter>()->GetHeight();
}

Rect DX11GfxWindow::GetRect() const
{
    return GetAdapterAs<DX11GfxWindowAdapter>()->GetRect();
}

// ** fullscreen
void DX11GfxWindow::SetFullscreen(bool value)
{
    (value);
    ReportBugMsg("Missing implementation. TODO: Implement fullscreen for DX11GfxWindow");
}

bool DX11GfxWindow::IsFullscreen() const
{
    ReportBugMsg("Missing implementation. TODO: Implement fullscreen for DX11GfxWindow");
    return false;
}

// ** visible
bool DX11GfxWindow::IsVisible() const
{
    return GetAdapterAs<DX11GfxWindowAdapter>()->IsVisible();
}
// ** created
bool DX11GfxWindow::IsCreated() const
{
    return GetAdapterAs<DX11GfxWindowAdapter>()->IsOpen();
}

void DX11GfxWindow::SetClassName(const String& value)
{
    mClassName = value;
}


#endif

} // namespace
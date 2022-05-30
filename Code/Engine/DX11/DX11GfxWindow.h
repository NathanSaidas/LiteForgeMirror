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
#include "Engine/DX11/DX11Common.h"
#include "Core/Utility/StdMap.h"
#include "Core/Input/InputTypes.h"

namespace lf {

#if defined(LF_DIRECTX11)
class InputMgr;
DECLARE_ATOMIC_WPTR(MouseDevice);
DECLARE_ATOMIC_PTR(MouseDevice);
DECLARE_ATOMIC_WPTR(KeyboardDevice);
DECLARE_ATOMIC_PTR(KeyboardDevice);

class LF_ENGINE_API DX11GfxWindowAdapter : public GfxWindowAdapter
{
    DECLARE_CLASS(DX11GfxWindowAdapter, GfxWindowAdapter);
public:
    DX11GfxWindowAdapter();
    virtual ~DX11GfxWindowAdapter();

    void OnInitialize(GfxDependencyContext& context) override;
    void OnShutdown() override;

    bool Open() override;
    void Close() override;

    void SetTitle(const String& title) override;
    void SetVisible(bool visible) override;
    void SetWidth(Int32 width) override;
    void SetHeight(Int32 height) override;
    void SetClassName(const String& className) override;

    Int32 GetWidth() const override { return mWidth; }
    Int32 GetHeight() const override { return mHeight; }

    Rect GetRect() const;

    const String& GetTitle() const { return mTitle; }
    bool IsVisible() const { return mVisible; }

    bool IsOpen() const { return mWindowHandle != NULL; }

    LRESULT ProcessMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

    ID3D11RenderTargetView* GetRenderTargetView() { return mRenderTargetView.Get(); }
    ID3D11DepthStencilView* GetDepthStencilView() { return mDepthView.Get(); }
    IDXGISwapChain* GetSwapChain() { return mSwapChain.Get(); }
private:
    bool CreateClass();
    bool CreateWindow();
    bool CreateGraphicsResources();

    void DestroyClass();
    void DestroyWindow();
    void DestroyGraphicsResources();

    void InitializeInput();
    bool ProcessMouseButton(HANDLE handle, ULONG bit, InputCode code, InputEventType eventType);

    MouseDeviceAtomicPtr GetMouseDevice(HANDLE handle);
    MouseDeviceAtomicPtr GetWindowMouse();
    KeyboardDeviceAtomicPtr GetWindowKeyboard();


    enum { FRAME_COUNT = 2 };

    // **********************************
    // ComPtr to the DirectX device
    // @static, weak
    // **********************************
    ComPtr<ID3D11Device>                mPlatformDevice;
    // **********************************
    // ComPtr to the DirectX device factory
    // @static, weak
    // **********************************
    ComPtr<ID3D11DeviceContext>         mPlatformDeviceContext;
    // **********************************
    // ComPtrs to the render target views for this window.
    // @static, owned
    // **********************************
    ComPtr<ID3D11RenderTargetView>      mRenderTargetView;
    // **********************************
    // ComPtr to depth texture
    // @owned
    // **********************************
    ComPtr<ID3D11Texture2D>             mDepthTexture; // <-- Used in depth stencil view
    // **********************************
    // ComPtr to depth view
    // @owned
    // **********************************
    ComPtr<ID3D11DepthStencilView>      mDepthView;
    // **********************************
    // ComPtr to the swap chain for this window.
    // @static, owned
    // **********************************
    ComPtr<IDXGISwapChain>              mSwapChain;
    // **********************************
    // Handle to the window.
    // @static, owned
    // **********************************
    HWND                                mWindowHandle;

    // **********************************
    // Handle associated with the window class.
    // @static, weak
    // **********************************
    HINSTANCE                           mClassHandle;
    // **********************************
    // The name of the window class.
    // @static
    // **********************************
    String                              mClassName;
    // **********************************
    // The title of the window
    // @dynamic
    // **********************************
    String                              mTitle;
    // **********************************
    // The state of whether or not the window is fullscreen.
    // @dynamic
    // TODO: Support fullscreen
    // **********************************
    // bool                             mFullscreen;
    // **********************************
    // The visible state of the window.
    // @dynamic
    // **********************************
    bool                                mVisible;
    // **********************************
    // The width of the window.
    // @dynamic
    // **********************************
    Int32                               mWidth;
    // **********************************
    // The height of the window.
    // @dynamic
    // **********************************
    Int32                               mHeight;
    // **********************************
    // A pointer to the input service from the to send keyboard/mouse inputs to.
    //
    // @weak,raw_safe
    // @nullable
    // **********************************
    InputMgr*                           mInputService;

    TMap<UInt64, MouseDeviceAtomicWPtr> mMouseDevices;
    MouseDeviceAtomicWPtr               mWindowMouse;
    KeyboardDeviceAtomicWPtr            mWindowKeyboard;
};

class LF_ENGINE_API DX11GfxWindow : public GfxWindow
{
    DECLARE_CLASS(DX11GfxWindow, GfxWindow);
public:
    DX11GfxWindow();
    virtual ~DX11GfxWindow();

    // **********************************
    // **********************************
    bool Open() override;

    // ** name
    void SetName(const String& value) override;
    const String& GetName() const override;

    // ** width
    void SetWidth(SizeT value) override;
    SizeT GetWidth() const override;

    // ** height
    void SetHeight(SizeT value) override;
    SizeT GetHeight() const override;

    Rect GetRect() const override;

    // ** fullscreen
    void SetFullscreen(bool value) override;
    bool IsFullscreen() const override;

    // ** visible
    bool IsVisible() const override;
    // ** created
    bool IsCreated() const override;

    void SetClassName(const String& value) override;
private:
    // **********************************
    // The name of the window class.
    // @static, cached
    // **********************************
    String                              mClassName;
    // **********************************
    // The title of the window
    // @dynamic, cached
    // **********************************
    String                              mTitle;
    // **********************************
    // The visible state of the window.
    // @dynamic, cached
    // **********************************
    bool                                mVisible;
    // **********************************
    // The width of the window.
    // @dynamic, cached
    // **********************************
    SizeT                               mWidth;
    // **********************************
    // The height of the window.
    // @dynamic, cached
    // **********************************
    SizeT                               mHeight;
};

#endif

} // namespace lf
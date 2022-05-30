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
#include "AbstractEngine/App/AppWindow.h"
#include "AbstractEngine/Input/KeyboardDevice.h"
#include "AbstractEngine/Input/MouseDevice.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers.
#endif
#include <Windows.h>

namespace lf {
DECLARE_ATOMIC_PTR(KeyboardDevice);
DECLARE_ATOMIC_PTR(MouseDevice);

class LF_ABSTRACT_ENGINE_API Win32Window : public AppWindow
{
    DECLARE_CLASS(Win32Window, AppWindow);
public:
    ~Win32Window() override;
    void InitDependencies(const ServiceContainer& services) override;

    bool Create(const AppWindowDesc& desc) override;
    void Destroy() override;
    bool Show() override;
    bool Hide() override;
    void SetTitle(const String& title) override;
    void SetSize(SizeT width, SizeT height) override;
    bool IsOpen() const override;

    HWND GetWindowHandle() { return mWindowHandle; }
private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    LRESULT ProcessWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
    bool ProcessMouseButton(HANDLE handle, ULONG bit, InputCode code, InputEventType eventType);

    HWND mWindowHandle;
    HINSTANCE mWindowClass;

    InputMgr* mInput;
    MouseDeviceAtomicPtr mMouse;
    KeyboardDeviceAtomicPtr mKeyboard;
};

} // namespace lf
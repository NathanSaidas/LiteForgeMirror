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
#include "AbstractEngine/PCH.h"
#include "Win32Window.h"
#include "Core/Reflection/DynamicCast.h"
#include "Core/Utility/Log.h"
#include "Runtime/Service/Service.h"
#include "AbstractEngine/Input/InputMgr.h"

#include <windowsx.h>

namespace lf {

DEFINE_CLASS(lf::Win32Window) { NO_REFLECTION; }

Win32Window::~Win32Window()
{
    Destroy();
}

void Win32Window::InitDependencies(const ServiceContainer& services)
{
    mInput = services.GetService<InputMgr>();
}

bool Win32Window::Create(const AppWindowDesc& desc)
{
    if (mWindowHandle)
    {
        return false;
    }
    Assert(!mWindowClass);

    UpdateID(desc.mID);
    UpdateTitle(desc.mTitle);
    UpdateWidth(desc.mWidth);
    UpdateHeight(desc.mHeight);

    WNDCLASSEX windowClass;
    memset(&windowClass, 0, sizeof(windowClass));
    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.hInstance = GetModuleHandle(NULL);
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.lpszClassName = GetID().CStr();
    if (RegisterClassExA(&windowClass) == 0)
    {
        return false;
    }
    mWindowClass = windowClass.hInstance;


    Int32 screenWidth = GetSystemMetrics(SM_CXSCREEN);
    Int32 screenHeight = GetSystemMetrics(SM_CYSCREEN);

    RECT windowRect = { 0 };
    windowRect.left = (screenWidth / 2) - (static_cast<Int32>(GetWidth()) / 2);
    windowRect.top = (screenHeight / 2) - (static_cast<Int32>(GetHeight()) / 2);
    windowRect.right = windowRect.left + static_cast<Int32>(GetWidth());
    windowRect.bottom = windowRect.top + static_cast<Int32>(GetHeight());

    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    mWindowHandle = CreateWindowA(
        GetID().CStr(),
        GetTitle().CStr(),
        WS_OVERLAPPEDWINDOW,
        windowRect.left,
        windowRect.top,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        nullptr,
        nullptr,
        mWindowClass,
        nullptr);
    if (mWindowHandle == NULL)
    {
        UnregisterClassA(GetID().CStr(), mWindowClass);
        mWindowClass = nullptr;
        return false;
    }

    SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    ShowWindow(mWindowHandle, desc.mDefaultHidden ? SW_HIDE : SW_SHOW);

    if (mInput)
    {
        mMouse = DynamicCast<MouseDeviceAtomicPtr>(mInput->FindInputDevice(GetMouseDeviceName()));
        if (!mMouse)
        {
            mMouse = MakeConvertibleAtomicPtr<MouseDevice>();
            mMouse->SetDeviceName(GetMouseDeviceName());
            if (!mMouse->Connect(mInput))
            {
                mMouse.Release();
            }
        }

        mKeyboard = DynamicCast<KeyboardDeviceAtomicPtr>(mInput->FindInputDevice(GetKeyboardDeviceName()));
        if (!mKeyboard)
        {
            mKeyboard = MakeConvertibleAtomicPtr<KeyboardDevice>();
            mKeyboard->SetDeviceName(GetMouseDeviceName());
            if (!mKeyboard->Connect(mInput))
            {
                mKeyboard.Release();
            }
        }
    }

    return true;
}

void Win32Window::Destroy()
{
    if (mWindowHandle)
    {
        SetWindowLongPtr(mWindowHandle, GWLP_USERDATA, 0);
        DestroyWindow(mWindowHandle);
        if (mWindowClass)
        {
            UnregisterClassA(GetID().CStr(), mWindowClass);
            mWindowClass = nullptr;
        }
        mWindowHandle = nullptr;
    }
}

bool Win32Window::Show()
{
    return ShowWindow(mWindowHandle, SW_SHOW);
}

bool Win32Window::Hide()
{
    return ShowWindow(mWindowHandle, SW_HIDE);
}

void Win32Window::SetTitle(const String& title)
{
    SetWindowTextA(mWindowHandle, title.CStr());
}

void Win32Window::SetSize(SizeT width, SizeT height) 
{
    // Window Coords
    // (0,0) 
    //      (1,1)
    //
    // Windows use RECTs so we must get the current rect for it's position and adjust the 'right'
    // and then correct the RECT because it does not take the full area into account.
    RECT clientRect = { 0 };
    GetClientRect(mWindowHandle, &clientRect);
    clientRect.right = clientRect.left + static_cast<Int32>(width);
    clientRect.bottom = clientRect.top + static_cast<Int32>(height);
    AdjustWindowRect(&clientRect, WS_OVERLAPPEDWINDOW, FALSE);
    RECT windowRect = { 0 };
    GetWindowRect(mWindowHandle, &windowRect);

    SetWindowPos(mWindowHandle, nullptr, windowRect.left, windowRect.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, 0);
}

bool Win32Window::IsOpen() const
{
    return mWindowHandle != nullptr;
}

LRESULT Win32Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    Win32Window* window = reinterpret_cast<Win32Window*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (window)
    {
        return window->ProcessWindowProc(hwnd, message, wParam, lParam);
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}

LRESULT Win32Window::ProcessWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_CLOSE:
        {
            DestroyWindow(mWindowHandle);
            mWindowHandle = NULL;
        } break;
        case WM_DESTROY:
        {
            mWindowHandle = NULL;
            if (mWindowClass)
            {
                UnregisterClassA(GetID().CStr(), mWindowClass);
                mWindowClass = nullptr;
            }
        } break;
        case WM_SIZE:
        {
            UpdateWidth(LOWORD(lParam));
            UpdateHeight(HIWORD(lParam));
        } break;
        case WM_KEYDOWN:
        {
            if (mKeyboard)
            {
                InputCode code = mKeyboard->VirtualKeyToCode(static_cast<Int32>(wParam));
                if (code != InputCode::NONE)
                {
                    mKeyboard->ReportPress(code);
                }
                else
                {
                    gSysLog.Warning(LogMessage("Unknown virtual keyboard input skipped! ") << wParam);
                }
            }
        } break;
        case WM_KEYUP:
        {
            if (mKeyboard)
            {
                InputCode code = mKeyboard->VirtualKeyToCode(static_cast<Int32>(wParam));
                if (code != InputCode::NONE)
                {
                    mKeyboard->ReportRelease(code);
                }
                else
                {
                    gSysLog.Warning(LogMessage("Unknown virtual keyboard input skipped! ") << wParam);
                }
            }
        } break;
        case WM_MOUSEMOVE:
        {
            if (mMouse)
            {
                Int32 x = static_cast<Int32>(GET_X_LPARAM(lParam));
                Int32 y = static_cast<Int32>(GET_Y_LPARAM(lParam));
                mMouse->ReportCursorPosition(x, y, GetAtomicPointer(this));
            }
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
                    if (mMouse)
                    {
                        mMouse->ReportCursorDelta(static_cast<Int32>(lastX), static_cast<Int32>(lastY));
                        processed = true;
                    }
                }


                if (!processed)
                {
                    gSysLog.Info(LogMessage("Unprocessed mouse input!"));
                }
            }
        } break;
    }
    return DefWindowProcA(hwnd, message, wParam, lParam);
}


bool Win32Window::ProcessMouseButton(HANDLE , ULONG bit, InputCode code, InputEventType eventType)
{
    if (!(bit > 0))
    {
        return false;
    }

    if (!mMouse)
    {
        return false;
    }

    if (eventType == InputEventType::BUTTON_PRESSED)
    {
        mMouse->ReportPress(code, GetAtomicPointer(this));
    }
    else
    {
        mMouse->ReportRelease(code, GetAtomicPointer(this));
    }
    return true;
}

} // namespace lf
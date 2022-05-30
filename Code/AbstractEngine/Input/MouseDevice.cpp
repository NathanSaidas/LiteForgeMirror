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
#include "AbstractEngine/PCH.h"
#include "MouseDevice.h"
#include "Core/Input/MouseEvents.h"
#include "AbstractEngine/Input/InputMgr.h"

namespace lf 
{
DEFINE_CLASS(lf::MouseDevice) { NO_REFLECTION; }
MouseDevice::MouseDevice()
: Super()
, mButtons { 0 }
, mCursorX(0)
, mCursorY(0)
, mCursorDeltaX(0)
, mCursorDeltaY(0)
{
    SetType(typeof(MouseDevice));
}
MouseDevice::~MouseDevice()
{

}

void MouseDevice::Update()
{
    if (!GetInputService())
    {
        return;
    }
    for (ButtonState& state : mButtons)
    {
        state.mPressed = state.mReleased = false;
    }

    if (mCursorDeltaX != 0 || mCursorDeltaY != 0)
    {
        MouseMoveEvent event;
        event.mCode = InputCode::CURSOR_MOUSE_DELTA;
        event.mCursorX = mCursorDeltaX;
        event.mCursorY = mCursorDeltaY;
        event.mLocalDeviceId = GetLocalDeviceId();
        event.mWindowData.mCursorX = mCursorX;
        event.mWindowData.mCursorY = mCursorY;
        mCursorDeltaX = 0;
        mCursorDeltaY = 0;
        GetInputService()->ReportInput(event);
    }

    // As long as the mouse button is pressed 'down' keep sending these events
    // vice versa if 'up'
}

void MouseDevice::ReportPress(InputCode inputCode, const ObjectAtomicWPtr& window)
{
    if (!GetInputService())
    {
        return;
    }

    auto begin = EnumValue(InputCode::MOUSE_BUTTON_LEFT);
    auto end = EnumValue(InputCode::MOUSE_AUX_BUTTON_10);
    auto code = EnumValue(inputCode);
    if (code >= begin && code <= end)
    {
        SizeT index = static_cast<SizeT>(code - begin);
        if (!mButtons[index].mDown)
        {
            MouseButtonEvent event;
            event.mCode = inputCode;
            InitBinaryInputState(event.mBinaryInputState, true, true, false);
            event.mCursorX = mCursorX;
            event.mCursorY = mCursorY;
            event.mLocalDeviceId = GetLocalDeviceId();
            event.mWindowData.mAtomicWindow = window;
            event.mWindowData.mCursorX = mCursorX;
            event.mWindowData.mCursorY = mCursorY;

            mButtons[index].mDown = true;
            mButtons[index].mPressed = true;
            mButtons[index].mReleased = false;
            GetInputService()->ReportInput(event);
        }
    }
}
void MouseDevice::ReportRelease(InputCode inputCode, const ObjectAtomicWPtr& window)
{
    if (!GetInputService())
    {
        return;
    }

    auto begin = EnumValue(InputCode::MOUSE_BUTTON_LEFT);
    auto end = EnumValue(InputCode::MOUSE_AUX_BUTTON_10);
    auto code = EnumValue(inputCode);
    if (code >= begin && code <= end)
    {
        SizeT index = static_cast<SizeT>(code - begin);
        if (mButtons[index].mDown)
        {
            MouseButtonEvent event;
            event.mCode = inputCode;
            InitBinaryInputState(event.mBinaryInputState, false, false, true);
            event.mCursorX = mCursorX;
            event.mCursorY = mCursorY;
            event.mLocalDeviceId = GetLocalDeviceId();
            event.mWindowData.mAtomicWindow = window;
            event.mWindowData.mCursorX = mCursorX;
            event.mWindowData.mCursorY = mCursorY;

            mButtons[index].mDown = false;
            mButtons[index].mPressed = false;
            mButtons[index].mReleased = true;
            GetInputService()->ReportInput(event);
        }
    }
}
void MouseDevice::ReportCursorDelta(Int32 x, Int32 y)
{
    if (!GetInputService())
    {
        return;
    }

    mCursorDeltaX += x;
    mCursorDeltaY += y;
}
void MouseDevice::ReportCursorPosition(Int32 x, Int32 y, const ObjectAtomicWPtr& window)
{
    if (!GetInputService())
    {
        return;
    }
    bool changed = mCursorX != x || mCursorY != y;
    if (changed)
    {
        MouseMoveEvent event;
        event.mCode = InputCode::CURSOR_MOUSE_X; // really just any input
        event.mCursorX = x;
        event.mCursorY = y;
        event.mLocalDeviceId = GetLocalDeviceId();
        event.mWindowData.mAtomicWindow = window;
        event.mWindowData.mCursorX = x;
        event.mWindowData.mCursorY = y;

        mCursorX = x;
        mCursorY = y;
        GetInputService()->ReportInput(event);
    }
}


} // namespace lf
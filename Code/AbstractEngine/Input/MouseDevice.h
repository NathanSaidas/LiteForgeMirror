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
#pragma once
#include "AbstractEngine/Input/InputDevice.h"
#include "Core/Input/InputTypes.h"

namespace lf
{

DECLARE_ATOMIC_WPTR(Object);


class LF_ABSTRACT_ENGINE_API MouseDevice : public InputDevice
{
    DECLARE_CLASS(MouseDevice, InputDevice);
public:
    MouseDevice();
    virtual ~MouseDevice();

    void Update() override;

    void ReportPress(InputCode inputCode, const ObjectAtomicWPtr& window);
    void ReportRelease(InputCode inputCode, const ObjectAtomicWPtr& window);
    void ReportCursorDelta(Int32 x, Int32 y);
    void ReportCursorPosition(Int32 x, Int32 y, const ObjectAtomicWPtr& window);
private:
    struct ButtonState
    {
        bool mDown;
        bool mPressed;
        bool mReleased;
    };
    enum { MAX_BUTTON = (EnumValue(InputCode::MOUSE_AUX_BUTTON_10) - EnumValue(InputCode::MOUSE_BUTTON_LEFT)) + 1 };

    ButtonState mButtons[MAX_BUTTON];
    Int32 mCursorX;
    Int32 mCursorY;
    Int32 mCursorDeltaX;
    Int32 mCursorDeltaY;
};

} // namespace lf

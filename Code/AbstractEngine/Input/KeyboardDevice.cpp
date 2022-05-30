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
#include "KeyboardDevice.h"
#include "Core/Input/KeyboardEvents.h"
#include "AbstractEngine/Input/InputMgr.h"

namespace lf
{

struct VirtualKey
{
    Int32 mVirtualKey;
    InputCode mCode;
};

// This isn't exactly complete, or 100% accurate.
VirtualKey VIRTUAL_KEY_MAP[]
{
    { 0, InputCode::NONE },
    { 1, InputCode::NONE },
    { 2, InputCode::NONE },
    { 3, InputCode::NONE },
    { 4, InputCode::NONE },
    { 5, InputCode::NONE },
    { 6, InputCode::NONE },
    { 7, InputCode::NONE },
    { 8, InputCode::NONE },       // VK_BACK
    { 9, InputCode::TAB },              // VK_TAB
    { 10, InputCode::NONE },         
    { 11, InputCode::NONE },      
    { 12, InputCode::NONE },       
    { 13, InputCode::ENTER },            // VK_RETURN
    { 14, InputCode::NONE },    
    { 15, InputCode::NONE },     
    { 16, InputCode::SHIFT },       // VK_SHIFT
    { 17, InputCode::CTRL },        // VK_CONTROL
    { 18, InputCode::ALT },
    { 19, InputCode::NONE },
    { 20, InputCode::NONE },
    { 21, InputCode::NONE },
    { 22, InputCode::NONE },
    { 23, InputCode::NONE },
    { 24, InputCode::NONE },
    { 25, InputCode::NONE },
    { 26, InputCode::NONE },
    { 27, InputCode::NONE },        // VK_ESCAPE
    { 28, InputCode::NONE },
    { 29, InputCode::NONE },
    { 30, InputCode::NONE },
    { 31, InputCode::NONE },
    { 32, InputCode::SPACE },       // VK_SPACE
    { 33, InputCode::NONE },        // VK_PRIOR (Page Up)
    { 34, InputCode::NONE },        // VK_NEXT (Page Down)
    { 35, InputCode::NONE },        // VK_END
    { 36, InputCode::NONE },        // VK_HOME
    { 37, InputCode::LEFT },        // VK_LEFT
    { 38, InputCode::UP },          // VK_UP
    { 39, InputCode::RIGHT },       // VK_RIGHT
    { 40, InputCode::DOWN },        // VK_DOWN
    { 41, InputCode::NONE },
    { 42, InputCode::NONE },
    { 43, InputCode::NONE },
    { 44, InputCode::NONE },
    { 45, InputCode::NONE },        // VK_INSERT
    { 46, InputCode::NONE },        // VK_DELETE
    { 47, InputCode::NONE },
    { 48, InputCode::ALPHA_0 },
    { 49, InputCode::ALPHA_1 },
    { 50, InputCode::ALPHA_2 },
    { 51, InputCode::ALPHA_3 },
    { 52, InputCode::ALPHA_4 },
    { 53, InputCode::ALPHA_5 },
    { 54, InputCode::ALPHA_6 },
    { 55, InputCode::ALPHA_7 },
    { 56, InputCode::ALPHA_8 },
    { 57, InputCode::ALPHA_9},
    { 58, InputCode::COLON },
    { 59, InputCode::SEMI_COLON },
    { 60, InputCode::LESS },
    { 61, InputCode::EQUALS },
    { 62, InputCode::GREATER },
    { 63, InputCode::SYM_QUESTION_MARK },
    { 64, InputCode::SYM_AT },
    { 65, InputCode::A },
    { 66, InputCode::B },
    { 67, InputCode::C },
    { 68, InputCode::D },
    { 69, InputCode::E },
    { 70, InputCode::F },
    { 71, InputCode::G },
    { 72, InputCode::H },
    { 73, InputCode::I },
    { 74, InputCode::J },
    { 75, InputCode::K },
    { 76, InputCode::L },
    { 77, InputCode::M },
    { 78, InputCode::N },
    { 79, InputCode::O },
    { 80, InputCode::P },
    { 81, InputCode::Q },
    { 82, InputCode::R },
    { 83, InputCode::S },
    { 84, InputCode::T },
    { 85, InputCode::U },
    { 86, InputCode::V },
    { 87, InputCode::W },
    { 88, InputCode::X },
    { 89, InputCode::Y },
    { 90, InputCode::Z },
    { 91, InputCode::NONE },
    { 92, InputCode::NONE },
    { 93, InputCode::NONE },
    { 94, InputCode::NONE },
    { 95, InputCode::NONE },
    { 96, InputCode::NONE },
    { 97, InputCode::NONE },
    { 98, InputCode::NONE },
    { 99, InputCode::NONE },
    { 100, InputCode::NONE },
    { 101, InputCode::NONE },
    { 102, InputCode::NONE },
    { 103, InputCode::NONE },
    { 104, InputCode::NONE },
    { 105, InputCode::NONE },
    { 106, InputCode::NONE },   // VK_MULTIPLY
    { 107, InputCode::PLUS },   // VK_ADD
    { 108, InputCode::NONE },   // VK_SEPARATOR
    { 109, InputCode::MINUS },   // VK_SUBTRACT
    { 110, InputCode::NONE },   // VK_DECIMAL
    { 111, InputCode::NONE },   // VK_DIVIDE
    { 112, InputCode::F1 },     // VK_F1
    { 113, InputCode::F2},      // VK_F2
    { 114, InputCode::F3},      // VK_F3
    { 115, InputCode::F4},      // VK_F4
    { 116, InputCode::F5},      // VK_F5
    { 117, InputCode::F6},      // VK_F6
    { 118, InputCode::F7},      // VK_F7
    { 119, InputCode::F8},      // VK_F8
    { 120, InputCode::NONE},
    { 121, InputCode::NONE},
    { 122, InputCode::NONE },
    { 123, InputCode::NONE },
    { 124, InputCode::NONE },
    { 125, InputCode::NONE },
    { 126, InputCode::NONE },
    { 127, InputCode::NONE },
    { 128, InputCode::NONE },

};

DEFINE_CLASS(lf::KeyboardDevice) { NO_REFLECTION; }

KeyboardDevice::KeyboardDevice() 
: Super()
, mButtons { false }
{
    SetType(typeof(KeyboardDevice));
}
KeyboardDevice::~KeyboardDevice()
{

}

void KeyboardDevice::Update()
{
    if (!GetInputService())
    {
        return;
    }

    for (ButtonState& button : mButtons)
    {
        button.mPressed = false;
        button.mReleased = false;
    }

    // As long as the key button is pressed 'down' keep sending these events
    // vice versa if 'up'
}

void KeyboardDevice::ReportPress(InputCode inputCode)
{
    if (!GetInputService())
    {
        return;
    }

    auto code = EnumValue(inputCode);
    if (code >= INPUT_CODE_BEGIN && code <= INPUT_CODE_END)
    {
        SizeT index = static_cast<SizeT>(code - INPUT_CODE_BEGIN);
        if (!mButtons[index].mDown)
        {
            KeyboardEvent event;
            event.mCode = inputCode;
            InitBinaryInputState(event.mBinaryInputState, true, true, false);
            event.mLocalDeviceId = GetLocalDeviceId();

            mButtons[index].mDown = true;
            mButtons[index].mPressed = true;
            mButtons[index].mReleased = false;
            GetInputService()->ReportInput(event);
        }
    }
}
void KeyboardDevice::ReportRelease(InputCode inputCode)
{
    if (!GetInputService())
    {
        return;
    }

    auto code = EnumValue(inputCode);
    if (code >= INPUT_CODE_BEGIN && code <= INPUT_CODE_END)
    {
        SizeT index = static_cast<SizeT>(code - INPUT_CODE_BEGIN);
        if (mButtons[index].mDown)
        {
            KeyboardEvent event;
            event.mCode = inputCode;
            InitBinaryInputState(event.mBinaryInputState, false, false, true);
            event.mLocalDeviceId = GetLocalDeviceId();

            mButtons[index].mDown = false;
            mButtons[index].mPressed = false;
            mButtons[index].mReleased = true;
            GetInputService()->ReportInput(event);
        }
    }
}

InputCode KeyboardDevice::VirtualKeyToCode(Int32 virtualKey)
{
    if (virtualKey < LF_ARRAY_SIZE(VIRTUAL_KEY_MAP))
    {
        CriticalAssert(virtualKey == VIRTUAL_KEY_MAP[virtualKey].mVirtualKey);
        return VIRTUAL_KEY_MAP[virtualKey].mCode;
    }
    return InputCode::NONE;
}

} // namespace lf
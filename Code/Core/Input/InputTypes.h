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
#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Common/Enum.h"
#include "Core/Utility/Bitfield.h"

namespace lf 
{
using InputDeviceId = UInt32;
enum : InputDeviceId { INVALID_INPUT_DEVICE_ID = INVALID32 };

LF_INLINE const char* DefaultInputFilter() { return "__default"; }

// InputType: The types of input there are.
// 
// BinaryInput: Input that is in 1 of 2 states (Down/Up)
// AxisInput: Input bound from 0.0 to 1.0 (normalized value)
// DeltaInput: Input from change in value (-1 or +1)
// CursorInput: Sampled input value.
DECLARE_STRICT_ENUM(InputType,
BINARY,
AXIS,
DELTA,
CURSOR);

// BinaryInputState: The state of a binary input
// 
// Pressed: Frame of input change (down)
// Released: Frame of input change (up)
// Down: 
// Up:
DECLARE_STRICT_ENUM(BinaryInputState,
PRESSED,
RELEASED,
DOWN,
UP);

// InputEventType: Types of events signalled from input
// 
DECLARE_STRICT_ENUM(InputEventType,
DATA_CHANGED,
BUTTON_DOWN,
BUTTON_UP,
BUTTON_PRESSED,
BUTTON_RELEASED);

DECLARE_STRICT_ENUM(InputDeviceType,
KEYBOARD,
MOUSE,
GAMEPAD);

DECLARE_STRICT_ENUM(InputConfigFlags,
HIDDEN,
PRIMARY,
SECONDARY);
using InputConfigBitfield = Bitfield<InputConfigFlags>;

DECLARE_STRICT_ENUM(InputBindingType,
AXIS,
ACTION,
CURSOR,
INDICATOR);

/** Supported keycodes*/
DECLARE_STRICT_ENUM(InputCode,
NONE,
ANY,
// BEGIN Keyboard
A,
B,
C,
D,
E,
F,
G,
H,
I,
J,
K,
L,
M,
N,
O,
P,
Q,
R,
S,
T,
U,
V,
W,
X,
Y,
Z,
ALPHA_0,
ALPHA_1,
ALPHA_2,
ALPHA_3,
ALPHA_4,
ALPHA_5,
ALPHA_6,
ALPHA_7,
ALPHA_8,
ALPHA_9,
SPACE,
CTRL,
SHIFT,
ALT,
ENTER,
LEFT,
RIGHT,
UP,
DOWN,
BACK_SPACE,
F1,
F2,
F3,
F4,
F5,
F6,
F7,
F8,
COMMA,
PERIOD,
FORWARD_SLASH,
SEMI_COLON,
DOUBLE_QUOTE,
SINGLE_QUOTE,
OPEN_BRACKET,
CLOSE_BRACKET,
BACK_SLASH,
MINUS,
EQUALS,
PLUS,
SYM_QUESTION_MARK,
SYM_EXCLAMATION,
SYM_AT,
SYM_NUMBER,
SYM_DOLLAR,
SYM_PERCENT,
SYM_XOR,
SYM_AND,
SYM_OR,
SYM_ASTERISK,
OPEN_PARENTHESES,
CLOSE_PARENTHESES,
OPEN_BRACE,
CLOSE_BRACE,
LESS,
GREATER,
COLON,
UNDERSCORE,
TAB,
// END Keyboard
// BEGIN Mouse
MOUSE_BUTTON_LEFT,
MOUSE_BUTTON_RIGHT,
MOUSE_BUTTON_MIDDLE,
MOUSE_AUX_BUTTON_0,
MOUSE_AUX_BUTTON_1,
MOUSE_AUX_BUTTON_2,
MOUSE_AUX_BUTTON_3,
MOUSE_AUX_BUTTON_4,
MOUSE_AUX_BUTTON_5,
MOUSE_AUX_BUTTON_6,
MOUSE_AUX_BUTTON_7,
MOUSE_AUX_BUTTON_8,
MOUSE_AUX_BUTTON_9,
MOUSE_AUX_BUTTON_10,
// END Mouse
// BEGIN Cursor
CURSOR_X,
CURSOR_Y,
CURSOR_MOUSE_X,
CURSOR_MOUSE_Y,
CURSOR_DELTA,
CURSOR_MOUSE_DELTA);

// TODO: Gamepad Bindings:
struct AxisSettings
{
    LF_INLINE AxisSettings()
    : mScaleType(SCALE_LINEAR)
    {}
    LF_INLINE ~AxisSettings() {}

    enum Scale
    {
        // -1.0 to 1.0
        SCALE_LINEAR,
        // 0.0 to 1.0
        SCALE_NORMALIZED
    };
    // ** How to scale the axis data
    Scale mScaleType;
};

struct BinaryAxisSettings : AxisSettings
{
    LF_INLINE BinaryAxisSettings() 
    : AxisSettings()
    , mPositiveIncrementDelta(1.0f)
    , mPositiveDecrementDelta(1.0f)
    , mNegativeIncrementDelta(1.0f)
    , mNegativeDecrementDelta(1.0f)
    , mSnapToDefault(true)
    {}
    LF_INLINE ~BinaryAxisSettings() {}

    // ** How fast to move to 'max' when positive key 'down'
    Float32 mPositiveIncrementDelta;
    // ** How fast to move to 'default' when positive key 'up'
    Float32 mPositiveDecrementDelta;
    // ** How fast to move to 'min' when negative key 'down'
    Float32 mNegativeIncrementDelta;
    // ** How fast to move to 'default' when negative key 'up'
    Float32 mNegativeDecrementDelta;
    // ** Whether or not to snap back to default when the down/up key are not pressed.
    bool mSnapToDefault;
};

struct InputModifier
{
    InputCode mInputCode;
    bool      mInputStates[ENUM_SIZE(BinaryInputState)];
};

// Device Events:
struct KeyboardEvent;
struct MouseButtonEvent;
struct MouseMoveEvent;

// System Events:
struct InputEvent;

LF_INLINE void InitBinaryInputState(bool(&state)[ENUM_SIZE(BinaryInputState)], bool down, bool pressed, bool released)
{
    if (!down)
    {
        pressed = false;
    }
    else
    {
        released = false;
    }

    state[EnumValue(BinaryInputState::DOWN)] = down;
    state[EnumValue(BinaryInputState::UP)] = !down;
    state[EnumValue(BinaryInputState::PRESSED)] = pressed;
    state[EnumValue(BinaryInputState::RELEASED)] = released;
}

LF_INLINE bool BinaryInputPressed(const bool(&state)[ENUM_SIZE(BinaryInputState)])
{
    return state[EnumValue(BinaryInputState::PRESSED)];
}

LF_INLINE bool BinaryInputReleased(const bool(&state)[ENUM_SIZE(BinaryInputState)])
{
    return state[EnumValue(BinaryInputState::RELEASED)];
}

LF_INLINE bool IsCursor(InputCode value, InputCode expectedCursor)
{
    switch (expectedCursor)
    {
        case InputCode::CURSOR_X:
            return value == InputCode::CURSOR_X || value == InputCode::CURSOR_MOUSE_X;
        case InputCode::CURSOR_Y:
            return value == InputCode::CURSOR_Y || value == InputCode::CURSOR_MOUSE_Y;
        default:
            break;
    }
    return false;
}

} // namespace lf

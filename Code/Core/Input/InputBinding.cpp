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
#include "Core/PCH.h"
#include "InputBinding.h"
#include "Core/Input/KeyboardEvents.h"
#include "Core/Input/MouseEvents.h"
#include "Core/Input/InputEvents.h"
#include "Core/Utility/Error.h"
#include "Core/Utility/Log.h"
#include "Core/Math/MathFunctions.h"

namespace lf
{
InputBinding::InputBinding()
: InterfaceType()
, mType(InputBindingType::INVALID_ENUM)
, mFilterScope()
, mBindings()
, mAxisBindings()
, mListeners()
, mBoundEventTypes{false}
, mLastEventData()
, mAction()
, mAxis()
{
}
InputBinding::~InputBinding()
{

}

APIResult<bool> InputBinding::InitializeAction(const Token& filterScope, bool defaultDown)
{
    if (ValidEnum(mType))
    {
        return ReportError(false, OperationFailureError, "Cannot initialize binding as action, action is already bound. Call Release!", "<NONE>");
    }

    mType = InputBindingType::ACTION;
    mFilterScope = filterScope;
    mAction.mDown = defaultDown;
    mAction.mPressed = false;
    mAction.mReleased = false;
    return APIResult<bool>(true);
}

APIResult<bool> InputBinding::InitializeAxis(const Token& filterScope)
{
    if (ValidEnum(mType))
    {
        return ReportError(false, OperationFailureError, "Cannot initialize binding as action, action is already bound. Call Release!", "<NONE>");
    }

    mType = InputBindingType::AXIS;
    mFilterScope = filterScope;
    memset(&mAxis, 0, sizeof(mAxis));
    mAxis.mMin = 0.0f;
    mAxis.mMax = 1.0f;
    return APIResult<bool>(true);
}

APIResult<bool> InputBinding::InitializeIndicator(const Token& filterScope, SizeT numIndicators)
{
    if (ValidEnum(mType))
    {
        return ReportError(false, OperationFailureError, "Cannot initialize binding as action, action is already bound. Call Release!", "<NONE>");
    }

    numIndicators = Max<SizeT>(1, numIndicators);

    mType = InputBindingType::INDICATOR;
    mFilterScope = filterScope;
    mIndicator.resize(numIndicators);
    return APIResult<bool>(true);
}

APIResult<bool> InputBinding::InitializeCursor(const Token& filterScope, SizeT numIndicators)
{
    if (ValidEnum(mType))
    {
        return ReportError(false, OperationFailureError, "Cannot initialize binding as action, action is already bound. Call Release!", "<NONE>");
    }

    numIndicators = Max<SizeT>(1, numIndicators);

    mType = InputBindingType::CURSOR;
    mFilterScope = filterScope;
    mIndicator.resize(numIndicators);
    return APIResult<bool>(true);
}

APIResult<bool> InputBinding::CreateAxis(const InputBindingData& axis, const AxisSettings& settings)
{
    gSysLog.Error(LogMessage("TODO: Implement" __FUNCTION__));
    (axis);
    (settings);
    return APIResult<bool>(false);
}
APIResult<bool> InputBinding::CreateBinaryAxis(const InputBindingData& positiveAxis, const InputBindingData& negativeAxis, const BinaryAxisSettings& settings)
{
    if (mType != InputBindingType::AXIS)
    {
        return ReportError(false, OperationFailureError, "CreateBinaryAxis, InputBinding.Type != ACTION", TInputBindingType::GetString(mType));
    }

    switch (positiveAxis.GetInputType())
    {
    case InputType::AXIS:
        return ReportError(false, InvalidArgumentError, "positiveAxis.InputType", "BinaryAxis can only be created with binary input. (Use CreateAxis instead)", "InputType::AXIS");
    case InputType::BINARY:
        break;
    case InputType::CURSOR:
        return ReportError(false, InvalidArgumentError, "positiveAxis.InputType", "BinaryAxis can only be created with binary input.", "InputType::CURSOR");
    case InputType::DELTA:
        return ReportError(false, InvalidArgumentError, "positiveAxis.InputType", "BinaryAxis can only be created with binary input.", "InputType::DELTA");
    default:
        return ReportError(false, InvalidArgumentError, "positiveAxis.InputType", "BinaryAxis can only be created with binary input.", "InputType::INVALID_ENUM");
    }

    switch (negativeAxis.GetInputType())
    {
    case InputType::AXIS:
        return ReportError(false, InvalidArgumentError, "negativeAxis.InputType", "BinaryAxis can only be created with binary input. (Use CreateAxis instead)", "InputType::AXIS");
    case InputType::BINARY:
        break;
    case InputType::CURSOR:
        return ReportError(false, InvalidArgumentError, "negativeAxis.InputType", "BinaryAxis can only be created with binary input.", "InputType::CURSOR");
    case InputType::DELTA:
        return ReportError(false, InvalidArgumentError, "negativeAxis.InputType", "BinaryAxis can only be created with binary input.", "InputType::DELTA");
    default:
        return ReportError(false, InvalidArgumentError, "negativeAxis.InputType", "BinaryAxis can only be created with binary input.", "InputType::INVALID_ENUM");
    }

    for (AxisBindingData& axis : mAxisBindings)
    {
        if (axis.mSettings.mScaleType != settings.mScaleType)
        {
            return ReportError(false, InvalidArgumentError, "settings.ScaleType", "BinaryAxis cannot have conflicting scale types.");
        }
    }

    if (mAxisBindings.empty())
    {
        switch (settings.mScaleType) 
        {
        case AxisSettings::SCALE_LINEAR:
        {
            mAxis.mMin = -1.0f;
            mAxis.mMax = 1.0f;
        } break;
        case AxisSettings::SCALE_NORMALIZED:
        {
            mAxis.mMin = 0.0f;
            mAxis.mMax = 1.0f;
        } break;
        default:
            return ReportError(false, InvalidArgumentError, "settings.ScaleType", "Unknown scale type.");
        }

        mAxis.mValue = (mAxis.mMin + mAxis.mMax) / 2.0f;
    }

    mAxisBindings.push_back(AxisBindingData());
    AxisBindingData& binding = mAxisBindings.back();
    binding.mPositive = positiveAxis;
    binding.mNegative = negativeAxis;
    binding.mBinary = true;
    binding.mSettings = settings;

    mBoundEventTypes[EnumValue(positiveAxis.GetEventType())] = true;
    mBoundEventTypes[EnumValue(negativeAxis.GetEventType())] = true;


    return APIResult<bool>(true);
}
APIResult<bool> InputBinding::CreateAction(const InputBindingData& action)
{
    if (mType != InputBindingType::ACTION)
    {
        return ReportError(false, OperationFailureError, "CreateAction, InputBinding.Type != ACTION", TInputBindingType::GetString(mType));
    }

    switch (action.GetInputType())
    {
    case InputType::AXIS:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Actions can only be created with binary input.", "InputType::AXIS");
    case InputType::BINARY:
        break;
    case InputType::CURSOR:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Actions can only be created with binary input.", "InputType::CURSOR");
    case InputType::DELTA:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Actions can only be created with binary input.", "InputType::DELTA");
    default:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Actions can only be created with binary input.", "InputType::INVALID_ENUM");
    }

    mBoundEventTypes[EnumValue(action.GetEventType())] = true;
    mBindings.push_back(action);
    return APIResult<bool>(true);
}

APIResult<bool> InputBinding::CreateIndicator(const InputBindingData& indicator)
{
    if (mType != InputBindingType::INDICATOR)
    {
        return ReportError(false, OperationFailureError, "CreateIndicator, InputBinding.Type != INDICATOR", TInputBindingType::GetString(mType));
    }

    switch (indicator.GetInputType())
    {
    case InputType::AXIS:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Indicators can only be created with binary or cursor inputs.", "InputType::AXIS");
    case InputType::BINARY:
    case InputType::CURSOR:
        break;
    case InputType::DELTA:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Indicators can only be created with binary or cursor inputs.", "InputType::DELTA");
    default:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Indicators can only be created with binary or cursor inputs.", "InputType::INVALID_ENUM");
    }

    mBoundEventTypes[EnumValue(indicator.GetEventType())] = true;
    mBindings.push_back(indicator);

    return APIResult<bool>(true);
}
APIResult<bool> InputBinding::CreateCursor(const InputBindingData& cursor)
{
    if (mType != InputBindingType::CURSOR)
    {
        return ReportError(false, OperationFailureError, "CreateCursor, InputBinding.Type != CURSOR", TInputBindingType::GetString(mType));
    }

    switch (cursor.GetInputType())
    {
    case InputType::AXIS:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Cursors can only be created with cursor inputs.", "InputType::AXIS");
    case InputType::BINARY:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Cursors can only be created with cursor inputs.", "InputType::BINARY");
    case InputType::CURSOR:
        break;
    case InputType::DELTA:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Cursors can only be created with cursor inputs.", "InputType::DELTA");
    default:
        return ReportError(false, InvalidArgumentError, "action.InputType", "Cursors can only be created with cursor inputs.", "InputType::INVALID_ENUM");
    }

    mBoundEventTypes[EnumValue(cursor.GetEventType())] = true;
    mBindings.push_back(cursor);
    return APIResult<bool>(true);
}

static bool AcceptKeyboard(InputEventType type)
{
    switch (type)
    {
    case InputEventType::BUTTON_DOWN:
    case InputEventType::BUTTON_UP:
    case InputEventType::BUTTON_PRESSED:
    case InputEventType::BUTTON_RELEASED:
    case InputEventType::DATA_CHANGED:
        return true;
    default:
        break;
    }
    return false;
}

static bool AcceptKeyboardAxis(InputEventType type)
{
    switch (type)
    {
    case InputEventType::BUTTON_DOWN:
    case InputEventType::BUTTON_UP:
    case InputEventType::DATA_CHANGED:
        return true;
    default:
        break;
    }
    return false;
}

static bool AcceptMouseIndicator(InputEventType type)
{
    switch (type)
    {
    case InputEventType::BUTTON_DOWN:
    case InputEventType::BUTTON_UP:
    case InputEventType::BUTTON_PRESSED:
    case InputEventType::BUTTON_RELEASED:
    case InputEventType::DATA_CHANGED:
        return true;
    default:
        break;
    }
    return false;
}

static bool AcceptMouseCursor(InputEventType type)
{
    switch (type)
    {
    case InputEventType::DATA_CHANGED:
        return true;
    default:
        break;
    }
    return false;
}

// for action: modify state, trigger callback
// for axis: modify value, trigger callback
// for cursor delta: ignore
// for cursor: ignore
// for indicator: ignore
void InputBinding::ProcessEvent(const KeyboardEvent& event)
{
    if (mType != InputBindingType::ACTION && mType != InputBindingType::AXIS)
    {
        return;
    }

    switch (mType)
    {
        case InputBindingType::ACTION:
        {
            // Find First Accepted Binding
            for (const InputBindingData& binding : mBindings)
            {
                if (binding.GetDeviceType() == InputDeviceType::KEYBOARD
                    && AcceptKeyboard(binding.GetEventType())
                    && binding.GetInputCode() == event.mCode)
                {
                    UpdateActionState(InputDeviceType::KEYBOARD, event, binding);
                    break;
                }
            }
        } break;
        case InputBindingType::AXIS:
        {
            for (const AxisBindingData& binding : mAxisBindings)
            {
                if (binding.mPositive.GetDeviceType() == InputDeviceType::KEYBOARD
                    && AcceptKeyboardAxis(binding.mPositive.GetEventType())
                    && binding.mPositive.GetInputCode() == event.mCode)
                {
                    UpdateAxisState(InputDeviceType::KEYBOARD, event, true, binding.mSettings, binding.mPositive);
                    break;
                }
                if (binding.mNegative.GetDeviceType() == InputDeviceType::KEYBOARD
                    && AcceptKeyboardAxis(binding.mNegative.GetEventType())
                    && binding.mNegative.GetInputCode() == event.mCode)
                {
                    UpdateAxisState(InputDeviceType::KEYBOARD, event, false, binding.mSettings, binding.mPositive);
                    break;
                }
            }
        } break;
        default:
            ReportBugMsg("Invalid state!");
            break;
    }
}

// for action: modify state, trigger callback
// for axis: modify value, trigger callback 
// for cursor delta: ignore
// for cursor: ignore
// for indicator: modify state, trigger callback

void InputBinding::ProcessEvent(const MouseButtonEvent& event)
{
    if (mType != InputBindingType::ACTION && mType != InputBindingType::AXIS && mType != InputBindingType::INDICATOR)
    {
        return;
    }

    switch (mType)
    {
        case InputBindingType::ACTION:
        {
            KeyboardEvent actionEvent;
            memcpy(actionEvent.mBinaryInputState, event.mBinaryInputState, sizeof(event.mBinaryInputState));
            actionEvent.mCode = event.mCode;
            actionEvent.mLocalDeviceId = event.mLocalDeviceId;
            actionEvent.mModifiers = event.mModifiers;

            // Find First Accepted Binding
            for (const InputBindingData& binding : mBindings)
            {
                if (binding.GetDeviceType() == InputDeviceType::MOUSE
                    && AcceptKeyboard(binding.GetEventType())
                    && binding.GetInputCode() == event.mCode)
                {
                    UpdateActionState(InputDeviceType::MOUSE, actionEvent, binding);
                    break;
                }
            }
        } break;
        case InputBindingType::AXIS:
        {
            KeyboardEvent actionEvent;
            memcpy(actionEvent.mBinaryInputState, event.mBinaryInputState, sizeof(event.mBinaryInputState));
            actionEvent.mCode = event.mCode;
            actionEvent.mLocalDeviceId = event.mLocalDeviceId;
            actionEvent.mModifiers = event.mModifiers;

            for (const AxisBindingData& binding : mAxisBindings)
            {
                if (binding.mPositive.GetDeviceType() == InputDeviceType::MOUSE
                    && AcceptKeyboardAxis(binding.mPositive.GetEventType())
                    && binding.mPositive.GetInputCode() == event.mCode)
                {
                    UpdateAxisState(InputDeviceType::MOUSE, actionEvent, true, binding.mSettings, binding.mPositive);
                    break;
                }
                if (binding.mNegative.GetDeviceType() == InputDeviceType::MOUSE
                    && AcceptKeyboardAxis(binding.mNegative.GetEventType())
                    && binding.mNegative.GetInputCode() == event.mCode)
                {
                    UpdateAxisState(InputDeviceType::MOUSE, actionEvent, false, binding.mSettings, binding.mPositive);
                    break;
                }
            }
        } break;
        case InputBindingType::INDICATOR:
        {
            for (const InputBindingData& binding : mBindings)
            {
                if (binding.GetDeviceType() == InputDeviceType::MOUSE
                    && AcceptMouseIndicator(binding.GetEventType())
                    && binding.GetInputCode() == event.mCode)
                {
                    UpdateIndicatorState(InputDeviceType::MOUSE, event, binding);
                    break;
                }
            }
        } break;
        default:
            ReportBugMsg("Invalid state!");
            break;
    }
}

// for action: ignore
// for axis: ignore
// for cursor: modify value, trigger callback
// for indicator: modify value, trigger callback
void InputBinding::ProcessEvent(const MouseMoveEvent& event)
{
    if (mType != InputBindingType::CURSOR && mType != InputBindingType::INDICATOR)
    {
        return;
    }

    switch (mType)
    {
        case InputBindingType::CURSOR:
        case InputBindingType::INDICATOR:
        {
            for (const InputBindingData& binding : mBindings)
            {
                if (binding.GetDeviceType() == InputDeviceType::MOUSE
                    && AcceptMouseCursor(binding.GetEventType())
                    && IsCursor(binding.GetInputCode(), event.mCode))
                {
                    UpdateCursorState(InputDeviceType::MOUSE, event, binding);
                    break;
                }
            }

        } break;
        default:
            ReportBugMsg("Invalid state!");
            break;
    }
}

void InputBinding::Update(Float32 delta)
{
    switch (mType)
    {
        case InputBindingType::ACTION:
        {
            ActionState oldAction = mAction;

            mAction.mPressed = false;
            mAction.mReleased = false;

            InputEvent event;
            event.mDeviceType = mLastEventData.mDeviceType;
            event.mInputType = InputType::BINARY; // actions are binary
            event.mInputCode = mLastEventData.mCode;
            event.mLocalDeviceId = mLastEventData.mDeviceId;

            // 
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::DOWN)] = mAction.mDown;
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::PRESSED)] = mAction.mPressed;
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::UP)] = !mAction.mDown;
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !mAction.mPressed;

            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::DOWN)] = oldAction.mDown;
            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::PRESSED)] = oldAction.mPressed;
            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::UP)] = !oldAction.mDown;
            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !oldAction.mPressed;

            event.mModifiers = mLastEventData.mModifiers;
            event.mBindingType = mType;
            event.mTriggeringBinding = mLastEventData.mBinding;
            event.mFilterScope = mFilterScope;
            
            DispatchEvent(mAction.mDown ? InputEventType::BUTTON_DOWN : InputEventType::BUTTON_UP, event);
        } break;
        case InputBindingType::AXIS:
        {
            Float32 oldValue = mAxis.mValue;
            mAxis.mValue = CalculateAxisValue(delta, mAxis);
            if (!ApproxEquals(oldValue, mAxis.mValue))
            {
                InputEvent event;
                // TODO: Could be optimized on input event filtering.
                event.mDeviceType = mLastEventData.mDeviceType;
                event.mInputType = InputType::AXIS;
                event.mInputCode = mLastEventData.mCode;
                event.mLocalDeviceId = mLastEventData.mDeviceId;

                event.mAxisInputValue.mCurrentValue = mAxis.mValue;
                event.mAxisInputValue.mOldValue = oldValue;

                event.mModifiers = mLastEventData.mModifiers;
                event.mBindingType = mType;
                event.mTriggeringBinding = mLastEventData.mBinding;
                event.mFilterScope = mFilterScope;

                DispatchEvent(InputEventType::DATA_CHANGED, event);
            }
        } break;
        case InputBindingType::INDICATOR:
        {
            IndicatorState oldState = mIndicator[0];
            IndicatorState& current = mIndicator[0];

            current.mPressed = false;
            current.mReleased = false;

            // TODO: Could be optimized on input event filtering.
            InputEvent event;
            event.mDeviceType = mLastEventData.mDeviceType;
            event.mInputType = InputType::BINARY; // actions are binary
            event.mInputCode = mLastEventData.mCode;
            event.mLocalDeviceId = mLastEventData.mDeviceId;

            // Binary Input State
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::DOWN)] = current.mDown;
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::PRESSED)] = current.mPressed;
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::UP)] = !current.mDown;
            event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !current.mPressed;

            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::DOWN)] = oldState.mDown;
            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::PRESSED)] = oldState.mPressed;
            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::UP)] = !oldState.mDown;
            event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !oldState.mPressed;

            // Indicator
            event.mIndicatorInputValue.mCursorX = current.mCursorX;
            event.mIndicatorInputValue.mCursorY = current.mCursorY;
            event.mIndicatorInputValue.mWindowData.mAtomicWindow = mLastEventData.mAtomicWindow;
            event.mIndicatorInputValue.mWindowData.mWindow= mLastEventData.mWindow;
            event.mIndicatorInputValue.mWindowData.mCursorX = mLastEventData.mWindowCursorX;
            event.mIndicatorInputValue.mWindowData.mCursorY = mLastEventData.mWindowCursorY;

            event.mModifiers = mLastEventData.mModifiers;
            event.mBindingType = mType;
            event.mTriggeringBinding = mLastEventData.mBinding;
            event.mFilterScope = mFilterScope;

            DispatchEvent(current.mDown ? InputEventType::BUTTON_DOWN : InputEventType::BUTTON_UP, event);
        } break;
        default:
            break;
    }
}

void InputBinding::OnEvent(const InputEventCallback& callback)
{
    if (callback.IsValid())
    {
        mListeners.push_back(callback);
    }
}

void InputBinding::UpdateActionState(InputDeviceType deviceType, const KeyboardEvent& event, const InputBindingData& binding)
{
    Assert(mType == InputBindingType::ACTION);
    bool isDown = event.mBinaryInputState[EnumValue(BinaryInputState::DOWN)] || event.mBinaryInputState[EnumValue(BinaryInputState::PRESSED)];
    bool isUp = event.mBinaryInputState[EnumValue(BinaryInputState::UP)] || event.mBinaryInputState[EnumValue(BinaryInputState::RELEASED)];
    
    if (isDown == isUp)
    {
        return;
    }

    bool isPressed = event.mBinaryInputState[EnumValue(BinaryInputState::PRESSED)];
    bool isReleased = event.mBinaryInputState[EnumValue(BinaryInputState::RELEASED)];

    ActionState oldAction = mAction;

    bool dataChanged = false;
    if (oldAction.mDown && isReleased)
    {
        mAction.mReleased = true;
        mAction.mPressed = false;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_RELEASED, deviceType, event, binding, oldAction);
    }
    else if (!oldAction.mDown && isPressed)
    {
        mAction.mPressed = true;
        mAction.mReleased = false;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_PRESSED, deviceType, event, binding, oldAction);
    }

    if (oldAction.mDown && isUp)
    {
        mAction.mDown = false;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_UP, deviceType, event, binding, oldAction);
    }
    else if(!oldAction.mDown && isDown)
    {
        mAction.mDown = true;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_DOWN, deviceType, event, binding, oldAction);
    }
    
    if (dataChanged)
    {
        DispatchEvent(InputEventType::DATA_CHANGED, deviceType, event, binding, oldAction);
    }
}

void InputBinding::UpdateAxisState(InputDeviceType deviceType, const KeyboardEvent& event, bool positive, const BinaryAxisSettings& settings, const InputBindingData& binding)
{
    Assert(mType == InputBindingType::AXIS);
    const Float32 FRAME_DELTA = 0.016f; // TODO: We should store previous frame deltas and use that.

    bool isDown = event.mBinaryInputState[EnumValue(BinaryInputState::DOWN)] || event.mBinaryInputState[EnumValue(BinaryInputState::PRESSED)];
    bool isUp = event.mBinaryInputState[EnumValue(BinaryInputState::UP)] || event.mBinaryInputState[EnumValue(BinaryInputState::RELEASED)];
    if (isDown == isUp)
    {
        return;
    }

    if (positive)
    {
        if (isUp)
        {
            --mAxis.mPositive;
            mAxis.mPositiveSpeed -= settings.mPositiveIncrementDelta;
            DispatchEvent(InputEventType::BUTTON_UP, deviceType, event, binding, mAxis.mValue, CalculateAxisValue(FRAME_DELTA, mAxis));
        }
        else if(isDown)
        {
            ++mAxis.mPositive;
            mAxis.mPositiveSpeed += settings.mPositiveIncrementDelta;
            DispatchEvent(InputEventType::BUTTON_DOWN, deviceType, event, binding, mAxis.mValue, CalculateAxisValue(FRAME_DELTA, mAxis));
        }
    }
    else 
    {
        if (isUp)
        {
            --mAxis.mNegative;
            mAxis.mPositiveSpeed -= settings.mNegativeIncrementDelta;
            DispatchEvent(InputEventType::BUTTON_UP, deviceType, event, binding, mAxis.mValue, CalculateAxisValue(FRAME_DELTA, mAxis));
        }
        else if (isDown)
        {
            ++mAxis.mNegative;
            mAxis.mPositiveSpeed += settings.mNegativeIncrementDelta;
            DispatchEvent(InputEventType::BUTTON_DOWN, deviceType, event, binding, mAxis.mValue, CalculateAxisValue(FRAME_DELTA, mAxis));
        }
    }

    mAxis.mPositiveNormalizeSpeed = settings.mPositiveDecrementDelta;
    mAxis.mNegativeNormalizeSpeed = settings.mNegativeDecrementDelta;
    mAxis.mSnapDefault = settings.mSnapToDefault;
}

void InputBinding::UpdateIndicatorState(InputDeviceType deviceType, const MouseButtonEvent& event, const InputBindingData& binding)
{
    Assert(mType == InputBindingType::INDICATOR);
    const SizeT MOUSE_INDICATOR = 0; // By default mouse indicators are at index 0

    bool isDown = event.mBinaryInputState[EnumValue(BinaryInputState::DOWN)] || event.mBinaryInputState[EnumValue(BinaryInputState::PRESSED)];
    bool isUp = event.mBinaryInputState[EnumValue(BinaryInputState::UP)] || event.mBinaryInputState[EnumValue(BinaryInputState::RELEASED)];
    if (isDown && isUp)
    {
        return;
    }

    IndicatorState oldState = mIndicator[MOUSE_INDICATOR];
    IndicatorState& current = mIndicator[MOUSE_INDICATOR];

    bool isPressed = event.mBinaryInputState[EnumValue(BinaryInputState::PRESSED)];
    bool isReleased = event.mBinaryInputState[EnumValue(BinaryInputState::RELEASED)];

    bool dataChanged = false;
    if (oldState.mPressed && isReleased)
    {
        current.mReleased = true;
        current.mPressed = false;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_RELEASED, deviceType, event, binding, oldState, current);
    }
    else if (oldState.mReleased && isPressed)
    {
        mAction.mPressed = true;
        mAction.mReleased = false;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_PRESSED, deviceType, event, binding, oldState, current);
    }

    if (oldState.mDown && isUp)
    {
        oldState.mDown = false;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_UP, deviceType, event, binding, oldState, current);
    }
    else if (!oldState.mDown && isDown)
    {
        oldState.mDown = true;
        dataChanged = true;
        DispatchEvent(InputEventType::BUTTON_DOWN, deviceType, event, binding, oldState, current);
    }

    if (dataChanged)
    {
        DispatchEvent(InputEventType::DATA_CHANGED, deviceType, event, binding, oldState, current);
    }
}

void InputBinding::UpdateCursorState(InputDeviceType deviceType, const MouseMoveEvent& event, const InputBindingData& binding)
{
    Assert(mType == InputBindingType::INDICATOR || mType == InputBindingType::CURSOR);
    const SizeT MOUSE_INDICATOR = 0; // By default mouse indicators are at index 0
    
    IndicatorState oldState = mIndicator[MOUSE_INDICATOR];
    IndicatorState& current = mIndicator[MOUSE_INDICATOR];
    
    current.mCursorX = event.mCursorX;
    current.mCursorY = event.mCursorY;
    
    bool changed = oldState.mCursorX != current.mCursorX || oldState.mCursorY != current.mCursorY;
    if (changed)
    {
        DispatchEvent(InputEventType::DATA_CHANGED, deviceType, event, binding, oldState, current);
    }
}

void InputBinding::DispatchEvent(InputEventType eventType, const InputEvent& event)
{
    if (mBoundEventTypes[EnumValue(eventType)])
    {
        for (InputEventCallback& callback : mListeners)
        {
            if (callback.IsValid())
            {
                callback.Invoke(event);
            }
        }
    }
}

void InputBinding::DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const KeyboardEvent& keyboardEvent, const InputBindingData& binding, const ActionState& oldAction)
{
    ResetLastEvent();
    mLastEventData.mBinding = binding;
    mLastEventData.mCode = keyboardEvent.mCode;
    mLastEventData.mDeviceId = keyboardEvent.mLocalDeviceId;
    mLastEventData.mModifiers = keyboardEvent.mModifiers;
    mLastEventData.mDeviceType = deviceType;

    InputEvent event;
    event.mDeviceType = deviceType;
    event.mInputType = InputType::BINARY; // actions are binary
    event.mInputCode = keyboardEvent.mCode;
    event.mLocalDeviceId = keyboardEvent.mLocalDeviceId;

    // 
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::DOWN)] = mAction.mDown;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::PRESSED)] = mAction.mPressed;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::UP)] = !mAction.mDown;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !mAction.mPressed;

    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::DOWN)] = oldAction.mDown;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::PRESSED)] = oldAction.mPressed;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::UP)] = !oldAction.mDown;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !oldAction.mPressed;

    event.mModifiers = keyboardEvent.mModifiers;
    event.mBindingType = mType;
    event.mTriggeringBinding = binding;
    event.mFilterScope = mFilterScope;

    DispatchEvent(eventType, event);
}

void InputBinding::DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const KeyboardEvent& keyboardEvent, const InputBindingData& binding, Float32 oldValue, Float32 newValue)
{
    ResetLastEvent();
    mLastEventData.mBinding = binding;
    mLastEventData.mCode = keyboardEvent.mCode;
    mLastEventData.mDeviceId = keyboardEvent.mLocalDeviceId;
    mLastEventData.mModifiers = keyboardEvent.mModifiers;
    mLastEventData.mDeviceType = deviceType;

    InputEvent event;
    event.mDeviceType = deviceType;
    event.mInputType = InputType::AXIS;
    event.mInputCode = keyboardEvent.mCode;
    event.mLocalDeviceId = keyboardEvent.mLocalDeviceId;

    event.mAxisInputValue.mCurrentValue = newValue;
    event.mAxisInputValue.mOldValue = oldValue;

    event.mModifiers = keyboardEvent.mModifiers;
    event.mBindingType = mType;
    event.mTriggeringBinding = binding;
    event.mFilterScope = mFilterScope;

    DispatchEvent(eventType, event);
}

void InputBinding::DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const MouseButtonEvent& mouseEvent, const InputBindingData& binding, const IndicatorState& oldValue, const IndicatorState& newValue)
{
    mLastEventData.mBinding = binding;
    mLastEventData.mCode = mouseEvent.mCode;
    mLastEventData.mDeviceId = mouseEvent.mLocalDeviceId;
    mLastEventData.mModifiers = mouseEvent.mModifiers;
    mLastEventData.mDeviceType = InputDeviceType::KEYBOARD;

    mLastEventData.mAtomicWindow = mouseEvent.mWindowData.mAtomicWindow;
    mLastEventData.mWindow = mouseEvent.mWindowData.mWindow;
    mLastEventData.mWindowCursorX = mouseEvent.mWindowData.mCursorX;
    mLastEventData.mWindowCursorY = mouseEvent.mWindowData.mCursorY;


    InputEvent event;
    event.mDeviceType = deviceType;
    event.mInputType = InputType::BINARY;
    event.mInputCode = mouseEvent.mCode;
    event.mLocalDeviceId = mouseEvent.mLocalDeviceId;

    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::DOWN)] = newValue.mDown;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::PRESSED)] = newValue.mPressed;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::UP)] = !newValue.mDown;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !newValue.mPressed;

    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::DOWN)] = oldValue.mDown;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::PRESSED)] = oldValue.mPressed;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::UP)] = !oldValue.mDown;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !oldValue.mPressed;

    event.mIndicatorInputValue.mCursorX = mouseEvent.mCursorX;
    event.mIndicatorInputValue.mCursorY = mouseEvent.mCursorY;
    event.mIndicatorInputValue.mWindowData.mAtomicWindow = mouseEvent.mWindowData.mAtomicWindow;
    event.mIndicatorInputValue.mWindowData.mWindow = mouseEvent.mWindowData.mWindow;
    event.mIndicatorInputValue.mWindowData.mCursorX = mouseEvent.mWindowData.mCursorX;
    event.mIndicatorInputValue.mWindowData.mCursorY = mouseEvent.mWindowData.mCursorY;

    event.mModifiers = mouseEvent.mModifiers;
    event.mBindingType = mType;
    event.mTriggeringBinding = binding;
    event.mFilterScope = mFilterScope;

    DispatchEvent(eventType, event);
}

void InputBinding::DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const MouseMoveEvent& mouseEvent, const InputBindingData& binding, const IndicatorState& oldValue, const IndicatorState& newValue)
{
    InputEvent event;
    event.mDeviceType = deviceType;
    event.mInputType = InputType::CURSOR;
    event.mInputCode = mouseEvent.mCode;
    event.mLocalDeviceId = mouseEvent.mLocalDeviceId;

    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::DOWN)] = newValue.mDown;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::PRESSED)] = newValue.mPressed;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::UP)] = !newValue.mDown;
    event.mBinaryInputValue.mCurrentValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !newValue.mPressed;

    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::DOWN)] = oldValue.mDown;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::PRESSED)] = oldValue.mPressed;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::UP)] = !oldValue.mDown;
    event.mBinaryInputValue.mOldValue.mValue[EnumValue(BinaryInputState::RELEASED)] = !oldValue.mPressed;

    event.mIndicatorInputValue.mCursorX = mouseEvent.mCursorX;
    event.mIndicatorInputValue.mCursorY = mouseEvent.mCursorY;
    event.mIndicatorInputValue.mWindowData.mAtomicWindow = mouseEvent.mWindowData.mAtomicWindow;
    event.mIndicatorInputValue.mWindowData.mWindow = mouseEvent.mWindowData.mWindow;
    event.mIndicatorInputValue.mWindowData.mCursorX = mouseEvent.mWindowData.mCursorX;
    event.mIndicatorInputValue.mWindowData.mCursorY = mouseEvent.mWindowData.mCursorY;

    event.mModifiers = mouseEvent.mModifiers;
    event.mBindingType = mType;
    event.mTriggeringBinding = binding;
    event.mFilterScope = mFilterScope;

    DispatchEvent(eventType, event);
}

Float32 InputBinding::CalculateAxisValue(Float32 delta, const AxisState& axis) const
{
    Float32 value = axis.mValue;
    // If we have input, move in their direction


    if (axis.mNonBinaryInputs > 0)
    {
        // noop
    }
    else if (axis.mPositive == axis.mNegative)
    {
        // noop
    }
    else if (axis.mPositive > axis.mNegative)
    {
        value += axis.mPositiveSpeed * delta;
    }
    else if (axis.mNegative > axis.mPositive)
    {
        value -= axis.mNegativeSpeed * delta;
    }
    else // Otherwise try to normalize.
    {
        Float32 defaultValue = (axis.mMax + axis.mMin) / 2.0f;
        if (axis.mSnapDefault)
        {
            value = defaultValue;
        }
        else if (value > defaultValue)
        {
            value = Max(defaultValue, value - axis.mPositiveNormalizeSpeed);
        }
        else if (value < defaultValue)
        {
            value = Min(defaultValue, value + axis.mNegativeNormalizeSpeed);
        }
    }
    return Clamp(value, axis.mMin, axis.mMax);
}

void InputBinding::ResetLastEvent()
{
    mLastEventData.mDeviceType = InputDeviceType::INVALID_ENUM;
    mLastEventData.mDeviceId = INVALID32;
    mLastEventData.mBinding = InputBindingData();
    mLastEventData.mCode = InputCode::NONE;
    mLastEventData.mModifiers.resize(0);
    

    mLastEventData.mWindow = NULL_PTR;
    mLastEventData.mAtomicWindow = NULL_PTR;
    mLastEventData.mWindowCursorX = 0;
    mLastEventData.mWindowCursorY = 0;
}

// TODO:
// GamepadButtonEvent
// {
//      InputCode: code
//      BinaryInputState: state
//      LocalDeviceId
// }


} // namespace lf
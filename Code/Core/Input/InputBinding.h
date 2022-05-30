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

#include "InputTypes.h"
#include "Core/Input/InputBindingData.h"
#include "Core/String/Token.h"
#include "Core/Utility/APIResult.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Utility/SmartCallback.h"

namespace lf 
{

using InputEventCallback = TCallback<void, const InputEvent&>;
class Object;
using ObjectAtomicWPtr = TAtomicWeakPointer<Object>;
using ObjectWPtr = TWeakPointer<Object>;

// TODO: Support reset/re-initialize.


// ********************************************************************
// InputBinding:
// Summary: 
//      InputBindings are a way to register for input event callbacks from the InputMgr (rather than polling constantly)
// 
// Usage:
//    Creator:
//      Call one of the Initialize functions to initialize the binding type. (Only call once)
//      Call the matching Create functions to define the event to respond to. (Call many times with different bindings, eg bind to 2 keys to one action)
//      Call InputMgr::RegisterBinding to register the binding for events
//    Creator/Event Listener:
//      Call OnEvent providing a callback to handle the input event. (Can store multiple callbacks)
// 
// Event Listeners can use InputMgr::GetInputBinding to find an input binding of a name and scope then call OnEvent
// 
// ** DATA STREAMING [Coming Soon] **
//   Should you need to know the 'current state' of an input and you don't care for lost/missed messages you can query the device and read the input every frame instead.
//   An example would be using the mouse cursor position.
// 
// See also:
//      Core/Input/InputBindingData.h
//      Core/Input/InputEvents.h
// ********************************************************************
class LF_CORE_API InputBinding : public TAtomicWeakPointerConvertible<InputBinding>
{
    using InterfaceType = TAtomicWeakPointerConvertible<InputBinding>;
public:
    InputBinding();
    ~InputBinding();

    APIResult<bool> InitializeAction(const Token& filterScope, bool defaultDown = false);
    APIResult<bool> InitializeAxis(const Token& filterScope);
    APIResult<bool> InitializeIndicator(const Token& filterScope, SizeT numIndicators = 1);
    APIResult<bool> InitializeCursor(const Token& filterScope, SizeT numIndicators = 1);

    APIResult<bool> CreateAxis(const InputBindingData& axis, const AxisSettings& settings);
    APIResult<bool> CreateBinaryAxis(const InputBindingData& positiveAxis, const InputBindingData& negativeAxis, const BinaryAxisSettings& settings);
    APIResult<bool> CreateAction(const InputBindingData& action);
    APIResult<bool> CreateIndicator(const InputBindingData& indicator);
    APIResult<bool> CreateCursor(const InputBindingData& cursor);

    void ProcessEvent(const KeyboardEvent& event);
    void ProcessEvent(const MouseButtonEvent& event);
    void ProcessEvent(const MouseMoveEvent& event);

    void Update(Float32 delta);

    void OnEvent(const InputEventCallback& callback);
    template<typename LambdaT>
    void OnEvent(const LambdaT& lambda) { OnEvent(InputEventCallback::Make(lambda)); }

private:
    struct TriggeringEventData
    {
        InputDeviceType         mDeviceType;
        InputCode               mCode;
        TVector<InputModifier>   mModifiers;
        InputDeviceId           mDeviceId;
        InputBindingData        mBinding;

        ObjectAtomicWPtr        mAtomicWindow;
        ObjectWPtr              mWindow;
        Int32                   mWindowCursorX;
        Int32                   mWindowCursorY;
    };

    struct ActionState
    {
        bool mDown;
        bool mPressed;
        bool mReleased;
    };

    struct AxisState
    {
        SizeT mPositive;
        SizeT mNegative;
        SizeT mNonBinaryInputs;

        Float32 mPositiveSpeed;
        Float32 mNegativeSpeed;

        Float32 mValue;
        Float32 mMin;
        Float32 mMax;

        Float32 mPositiveNormalizeSpeed;
        Float32 mNegativeNormalizeSpeed;
        bool    mSnapDefault;
    };

    struct AxisBindingData
    {
        bool mBinary;
        BinaryAxisSettings mSettings;
        InputBindingData mPositive;
        InputBindingData mNegative;
    };

    struct IndicatorState
    {
        LF_INLINE IndicatorState()
        : mCursorX(0)
        , mCursorY(0)
        , mDown(false)
        , mPressed(false)
        , mReleased(false)
        {}

        Int32 mCursorX;
        Int32 mCursorY;

        bool  mDown;
        bool  mPressed;
        bool  mReleased;
    };

    void UpdateActionState(InputDeviceType deviceType, const KeyboardEvent& event, const InputBindingData& binding);
    void UpdateAxisState(InputDeviceType deviceType, const KeyboardEvent& event, bool positive, const BinaryAxisSettings& settings, const InputBindingData& binding);
    void UpdateIndicatorState(InputDeviceType deviceType, const MouseButtonEvent& event, const InputBindingData& binding);
    void UpdateCursorState(InputDeviceType deviceType, const MouseMoveEvent& event, const InputBindingData& binding);


    void DispatchEvent(InputEventType eventType, const InputEvent& event);
    void DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const KeyboardEvent& keyboardEvent, const InputBindingData& binding, const ActionState& oldAction);
    void DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const KeyboardEvent& keyboardEvent, const InputBindingData& binding, Float32 oldValue, Float32 newValue);
    void DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const MouseButtonEvent& mouseEvent, const InputBindingData& binding, const IndicatorState& oldValue, const IndicatorState& newValue);
    void DispatchEvent(InputEventType eventType, InputDeviceType deviceType, const MouseMoveEvent& mouseEvent, const InputBindingData& binding, const IndicatorState& oldValue, const IndicatorState& newValue);


    Float32 CalculateAxisValue(Float32 delta, const AxisState& state) const;
    void ResetLastEvent();

    InputBindingType            mType;
    Token                       mFilterScope;
    TVector<InputBindingData>    mBindings;
    TVector<AxisBindingData>     mAxisBindings;
    TVector<InputEventCallback>  mListeners;

    bool                        mBoundEventTypes[ENUM_SIZE(InputEventType)];

    TriggeringEventData             mLastEventData;
    ActionState                     mAction;
    AxisState                       mAxis;
    TStackVector<IndicatorState,2>  mIndicator;

};

} // namespace lf
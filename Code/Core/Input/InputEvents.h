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
#include "Core/Input/InputTypes.h"
#include "Core/Input/InputBindingData.h"
#include "Core/String/Token.h"

namespace lf 
{
class Object;
using ObjectAtomicWPtr = TAtomicWeakPointer<Object>;
using ObjectWPtr = TWeakPointer<Object>;



struct InputEvent
{
    struct BinaryInputValue
    {
        bool mValue[ENUM_SIZE(BinaryInputState)];
    };

    struct BinaryInputSubEvent
    {
        BinaryInputValue mCurrentValue;
        BinaryInputValue mOldValue;
    };

    struct AxisInputSubEvent
    {
        Float32 mCurrentValue;
        Float32 mOldValue;
    };

    struct IndicatorWindowData
    {
        ObjectAtomicWPtr mAtomicWindow;
        ObjectWPtr       mWindow;

        Int32            mCursorX;
        Int32            mCursorY;
    };

    struct IndicatorSubEvent
    {
        Int32 mCursorX;
        Int32 mCursorY;
        IndicatorWindowData mWindowData;
    };

    struct CursorSubEvent
    {
        struct
        {
            Int32 mCursorX;
            Int32 mCursorY;
        } mCurrentValue;
        struct
        {
            Int32 mCursorX;
            Int32 mCursorY;
        } mOldValue;
    };

    InputDeviceType         mDeviceType;
    InputType               mInputType;
    InputCode               mInputCode;
    InputDeviceId           mLocalDeviceId;

    // Valid If InputType==Binary
    BinaryInputSubEvent     mBinaryInputValue;
    // Valid If InputType==Axis
    AxisInputSubEvent       mAxisInputValue;
    // Valid If InputType==Binary && BindingType==Indicator
    IndicatorSubEvent       mIndicatorInputValue;
    // Valid If InputType==Cursor && BindingType==Cursor
    CursorSubEvent          mCursorInputValue;

    TVector<InputModifier>   mModifiers;

    InputBindingType        mBindingType;
    InputBindingData        mTriggeringBinding;
    Token                   mFilterScope;
};


} // namespace lf
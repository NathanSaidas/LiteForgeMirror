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
#include "Core/Utility/StdVector.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Memory/SmartPointer.h"

namespace lf
{
class Object;
using ObjectAtomicWPtr = TAtomicWeakPointer<Object>;
using ObjectWPtr = TWeakPointer<Object>;

struct MouseWindowData
{
    ObjectAtomicWPtr mAtomicWindow;
    ObjectWPtr       mWindow;

    Int32            mCursorX;
    Int32            mCursorY;
};


// ** Represents a native mouse button event sent from input manager.
struct MouseButtonEvent
{
    InputCode             mCode;
    bool                  mBinaryInputState[ENUM_SIZE(BinaryInputState)];
    TVector<InputModifier> mModifiers;
    MouseWindowData       mWindowData;
    Int32                 mCursorX;
    Int32                 mCursorY;
    InputDeviceId         mLocalDeviceId;
};

// ** Represents a native mouse move event sent from input manager.
struct MouseMoveEvent
{
    InputCode             mCode;
    TVector<InputModifier> mModifiers;
    MouseWindowData       mWindowData;
    Int32                 mCursorX;
    Int32                 mCursorY;
    InputDeviceId         mLocalDeviceId;
};

} // namespace lf

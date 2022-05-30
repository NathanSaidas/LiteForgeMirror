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
#include "Core/Input/InputTypes.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/String/Token.h"
#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

class InputMgr;

class LF_ABSTRACT_ENGINE_API InputDevice : public Object, public TAtomicWeakPointerConvertible<InputDevice>
{
    DECLARE_CLASS(InputDevice, Object);
public:
    InputDevice();
    virtual ~InputDevice();

    bool Connect(InputMgr* inputManager);
    void Disconnect();

    // ** Called each frame by input manager
    virtual void Update();

    void SetDeviceName(const Token& value) { mDeviceName = value; }
    const Token& GetDeviceName() const { return mDeviceName; }
    InputDeviceId GetLocalDeviceId() const { return mLocalDeviceId; }
protected:
    InputMgr* GetInputService() { return mInputService; }
    const InputMgr* GetInputService() const { return mInputService; }
private:
    // **********************************
    // The name of the device
    // **********************************
    Token     mDeviceName;

    InputDeviceId mLocalDeviceId;
    // **********************************
    // Pointer to the input manager service to report inputs to
    // **********************************
    InputMgr* mInputService; 
};

} // namespace lf

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
#include "AbstractEngine/Input/InputMgr.h"
#include "Core/Input/InputBinding.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/UniqueNumber.h"
#include "AbstractEngine/Input/InputDevice.h"

namespace lf
{

class AppService;
DECLARE_ATOMIC_PTR(InputDevice);

// TODO: Text processing might want to know...
// VK_CAPITAL
// eg; Modifier { InputCode::CAPS_LOCK, DOWN } 
class LF_ENGINE_API Win32InputMgr : public InputMgr
{
    using BindingPtr = TAtomicStrongPointer<InputBinding>;
    DECLARE_CLASS(Win32InputMgr, InputMgr);
public:
    // Service API
    APIResult<ServiceResult::Value> OnStart() override;
    APIResult<ServiceResult::Value> OnBeginFrame() override;

    // InputMgr API
    APIResult<bool> RegisterBinding(const Token& name, const Token& filterScope, InputBinding* binding) override;
    APIResult<bool> UnregisterBinding(const Token& name, const Token& filterScope, InputBinding* binding) override;
    InputBinding* GetInputBinding(const Token& name, const Token& filterScope) override;

    InputDeviceId RegisterInputDevice(InputDevice* device) override;
    void UnregisterInputDevice(InputDevice* device) override;

    void PushInputFilter(const Token& filter, bool additive = false);
    void PopInputFilter();

    void ReportInput(const KeyboardEvent& event) override;
    void ReportInput(const MouseButtonEvent& event) override;
    void ReportInput(const MouseMoveEvent& event) override;

    InputDeviceAtomicPtr FindInputDevice(const Token& name) const override;

private:
    using BindingMap = TMap<Token, BindingPtr>;
    using InputFilter = TMap<Token, BindingMap>;
    
    using DeviceMap = TMap<InputDeviceId, InputDeviceAtomicPtr>;

    using FilterMask = TArray<InputFilter::iterator>;
    
    Token mDefaultFilter;

    InputFilter mFilters;
    TArray<FilterMask> mFilterMask;

    DeviceMap mDevices;
    UniqueNumber<InputDeviceId, 16> mDeviceIdGen;

    AppService* mAppService;
    
};

} // namespace lf

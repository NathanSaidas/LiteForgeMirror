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
#include "Engine/PCH.h"
#include "Win32InputMgr.h"
#include "Core/Reflection/Type.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Error.h"

#include "Core/Input/MouseEvents.h"
#include "Core/Input/KeyboardEvents.h"

#include "AbstractEngine/App/AppService.h"

namespace lf 
{

DEFINE_CLASS(lf::Win32InputMgr) { NO_REFLECTION; }

APIResult<ServiceResult::Value> Win32InputMgr::OnStart()
{
    auto superResult = Super::OnBeginFrame();
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }
    mAppService = GetServices()->GetService<AppService>();
    if (!mAppService)
    {
        gSysLog.Warning(LogMessage("Win32InputMgr could not locate AppService. A constant frame delta will be used instead."));
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> Win32InputMgr::OnBeginFrame() 
{
    auto superResult = Super::OnBeginFrame();
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }

    for (auto& pair : mDevices)
    {
        InputDeviceAtomicPtr device = pair.second;
        if (device)
        {
            device->Update();
        }
    }

    Float32 delta = mAppService ? mAppService->GetLastFrameDelta() : ToSeconds(TimeTypes::Milliseconds(16.0f)).mValue;
    for (auto& pair : mFilters)
    {
        for (auto& bindings : pair.second)
        {
            auto& binding = bindings.second;
            binding->Update(delta);
        }
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<bool> Win32InputMgr::RegisterBinding(const Token& name, const Token& filterScope, InputBinding* binding)
{
    // TODO: Thread check

    // persistentAction = (binding->IsAction() && (binding->EventType == ButtonDown || binding->EventType == ButtonUp))

    if (name.Empty())
    {
        return ReportError(false, InvalidArgumentError, "name", "The name of the binding cannot be empty.");
    }
    if (filterScope.Empty())
    {
        return ReportError(false, InvalidArgumentError, "filterScope", "The name of the filter scope cannot be empty. Use the default scope.");
    }
    auto bindingPtr = GetAtomicPointer(binding);
    if (!binding)
    {
        return ReportError(false, InvalidArgumentError, "binding", "The binding cannot be null and must be initialized as convertible atomic.");
    }
    auto& filter = mFilters[filterScope];
    auto& inputBinding = filter[name];
    if (inputBinding)
    {
        return ReportError(false, OperationFailureError, "Failed to register input binding. One with that name already exists.", name.CStr());
    }
    inputBinding = GetAtomicPointer(binding);
    return APIResult<bool>(true);
}
APIResult<bool> Win32InputMgr::UnregisterBinding(const Token& name, const Token& filterScope, InputBinding* binding)
{
    // TODO: Thread Check

    if (name.Empty())
    {
        return ReportError(false, InvalidArgumentError, "name", "The name of the binding cannot be empty.");
    }
    if (filterScope.Empty())
    {
        return ReportError(false, InvalidArgumentError, "filterScope", "The name of the filter scope cannot be empty. Use the default scope.");
    }
    auto bindingPtr = GetAtomicPointer(binding);
    if (!binding)
    {
        return ReportError(false, InvalidArgumentError, "binding", "The binding cannot be null and must be initialized as convertible atomic.");
    }

    auto filter = mFilters.find(filterScope);
    if (filter == mFilters.end())
    {
        return APIResult<bool>(false);
    }
    auto inputBinding = filter->second.find(name);
    if (inputBinding == filter->second.end())
    {
        return APIResult<bool>(false);
    }

    filter->second.erase(inputBinding);
    if (filter->second.empty())
    {
        mFilters.erase(filter);
    }
    return APIResult<bool>(true);
}
InputBinding* Win32InputMgr::GetInputBinding(const Token& name, const Token& filterScope)
{
    // TODO: Thread Check

    if (name.Empty() || filterScope.Empty())
    {
        return nullptr;
    }

    auto filter = mFilters.find(filterScope);
    if (filter == mFilters.end())
    {
        return nullptr;
    }
    auto inputBinding = filter->second.find(name);
    if (inputBinding == filter->second.end())
    {
        return nullptr;
    }
    return inputBinding->second;
}

InputDeviceId Win32InputMgr::RegisterInputDevice(InputDevice* device)
{
    InputDeviceAtomicPtr atomic = GetAtomicPointer(device);
    if (!atomic)
    {
        return INVALID_INPUT_DEVICE_ID;
    }

    // TODO: Thread Asset
    InputDeviceId id = mDeviceIdGen.Allocate();
    InputDeviceAtomicPtr& mappedDevice = mDevices[id];
    CriticalAssert(mappedDevice == NULL_PTR); // If this trips, unique number generate not generating unique numbers!
    mappedDevice = atomic;

    gSysLog.Info(LogMessage("Registering input device ") << device->GetType()->GetFullName() << " with id " << id);

    return id;

}
void Win32InputMgr::UnregisterInputDevice(InputDevice* device)
{
    // TODO: Thread Asset
    if (!device || Invalid(device->GetLocalDeviceId()))  
    {
        return;
    }

    auto iter = mDevices.find(device->GetLocalDeviceId());
    if (iter != mDevices.end())
    {
        mDeviceIdGen.Free(device->GetLocalDeviceId());
        mDevices.erase(iter);
    }
}

void Win32InputMgr::PushInputFilter(const Token& filter, bool additive)
{
    auto iter = mFilters.find(filter);
    if (iter != mFilters.end())
    {
        if (additive && !mFilterMask.Empty())
        {
            auto mask = mFilterMask.GetLast();
            mask.Add(iter);
            mFilterMask.Add(mask);
        }
        else
        {
            mFilterMask.Add({ iter });
        }
    }
}
void Win32InputMgr::PopInputFilter()
{
    if (!mFilterMask.Empty())
    {
        mFilterMask.Remove(mFilterMask.rbegin().GetBase());
    }

    // try to keep a default
    if (mFilterMask.Empty() && !mFilters.empty())
    {
        auto defaultMask = mFilters.find(mDefaultFilter);
        if (defaultMask == mFilters.end())
        {
            defaultMask = mFilters.begin();
        }
        mFilterMask.Add({ defaultMask });
    }
}

void Win32InputMgr::ReportInput(const KeyboardEvent& event)
{
    if (BinaryInputPressed(event.mBinaryInputState))
    {
        gSysLog.Debug(LogMessage("ReportInput.KeyboardEvent ") << TInputCode::GetString(event.mCode));
    }

    // TODO: Thread assert
    if (mFilterMask.Empty())
    {
        return;
    }

    auto& mask = mFilterMask.GetLast();
    for (InputFilter::iterator& filter : mask) // Usually 1, if not additive
    {
        for (auto& bindingPair : filter->second)
        {
            BindingPtr& binding = bindingPair.second;
            binding->ProcessEvent(event);
        }
    }
}
void Win32InputMgr::ReportInput(const MouseButtonEvent& event)
{
    gSysLog.Debug(LogMessage("ReportInput.MouseButtonEvent ") << TInputCode::GetString(event.mCode));

    // TODO: Thread assert
    if (mFilterMask.Empty())
    {
        return;
    }
    // TODO: We might cull out persistent 'down' or 'up' events unless they are explicitly listened for.

    auto& mask = mFilterMask.GetLast();
    for (InputFilter::iterator& filter : mask) // Usually 1, if not additive
    {
        for (auto& bindingPair : filter->second)
        {
            BindingPtr& binding = bindingPair.second;
            binding->ProcessEvent(event);
        }
    }
}
void Win32InputMgr::ReportInput(const MouseMoveEvent& event)
{
    gSysLog.Debug(LogMessage("ReportInput.MouseMoveEvent ") << TInputCode::GetString(event.mCode) << " " << event.mCursorX << " " << event.mCursorY);
    if (mFilterMask.Empty())
    {
        return;
    }

    auto& mask = mFilterMask.GetLast();
    for (InputFilter::iterator& filter : mask) // Usually 1, if not additive
    {
        for (auto& bindingPair : filter->second)
        {
            BindingPtr& binding = bindingPair.second;
            binding->ProcessEvent(event);
        }
    }
}

InputDeviceAtomicPtr Win32InputMgr::FindInputDevice(const Token& name) const
{
    for (const auto& pair : mDevices)
    {
        if (pair.second->GetDeviceName() == name)
        {
            return pair.second;
        }
    }

    return InputDeviceAtomicPtr();
}


} // namespace lf
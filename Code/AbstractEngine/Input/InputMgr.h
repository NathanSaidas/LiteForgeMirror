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
#include "Core/Common/API.h"
#include "Core/Common/Types.h"
#include "Core/Input/InputTypes.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Utility/APIResult.h"
#include "Runtime/Service/Service.h"


namespace lf 
{

class Token;
class InputBinding;
class InputDevice;
struct InputEvent;
struct KeyboardEvent;
struct MouseButtonEvent;
struct MouseMoveEvent;
DECLARE_ATOMIC_PTR(InputDevice);


// ********************************************************************
// The manager for 'System' input layer.
// 
// ** InputFiltering & Scopes **
//     Input Filtering applies to game input and allows for a way to
//     turn on groups of input bindings based on the context of the game.
// 
// ** System Inputs **
//     Not all types of things will respect input filtering. Code that 
//     access the input manager for raw input events are 'global' input
//     and they cannot be remapped.
//
// ** Input Remapping **
//     Input remapping is not managed by the input mgr. If you wish to 
//     remap an input binding then get access to it and 'reset' and
//     'initialize' it with the new mapped inputs.
// 
// ********************************************************************
class LF_ABSTRACT_ENGINE_API InputMgr : public Service
{
    DECLARE_CLASS(InputMgr, Service);
public:
    InputMgr();
    virtual ~InputMgr();
    // ********************************************************************
    // Register an input binding with a given name/filter scope.
    // Two bindings with same name & scope cannot exist.
    // The lifetime of the input binding is managed by the input manager
    // callers should hold onto the WeakPtr if they want it.
    // 
    // @thread=[MainThread/InputThread]
    // ********************************************************************
    virtual APIResult<bool> RegisterBinding(const Token& name, const Token& filterScope, InputBinding* binding) = 0;
    // ********************************************************************
    // Unregister an input binding by name/filter.
    //
    // @thread=[MainThread/InputThread]
    // ********************************************************************
    virtual APIResult<bool> UnregisterBinding(const Token& name, const Token& filterScope, InputBinding* binding) = 0;
    // ********************************************************************
    // Retrieve input binding by name/filter.
    //
    // @thread=[MainThread/InputThread]
    // ********************************************************************
    virtual InputBinding* GetInputBinding(const Token& name, const Token& filterScope) = 0;

    virtual InputDeviceId RegisterInputDevice(InputDevice* device) = 0;
    virtual void UnregisterInputDevice(InputDevice* device) = 0;

    virtual void PushInputFilter(const Token& filter, bool additive = false) = 0;
    virtual void PopInputFilter() = 0;

    virtual void ReportInput(const KeyboardEvent& event) = 0;
    virtual void ReportInput(const MouseButtonEvent& event) = 0;
    virtual void ReportInput(const MouseMoveEvent& event) = 0;

    virtual InputDeviceAtomicPtr FindInputDevice(const Token& name) const = 0;
};


} // namespace lf
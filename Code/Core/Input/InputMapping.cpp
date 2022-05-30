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
#include "Core/Input/InputMapping.h"
#include "Core/Utility/Error.h"
namespace lf 
{

InputMapping::InputMapping()
: mName()
, mScope()
, mBindings()
{}
InputMapping::InputMapping(const Token& name, const Token& scope)
: mName(name)
, mScope(scope)
, mBindings()
{}

void InputMapping::Serialize(Stream& s)
{
    SERIALIZE(s, mName, "");
    SERIALIZE(s, mScope, "");
    SERIALIZE_STRUCT_ARRAY(s, mBindings, "");
}
APIResult<bool> InputMapping::Register(const InputBindingData& binding)
{
    bool primary = binding.GetConfigFlag(InputConfigFlags::PRIMARY);
    bool secondary = binding.GetConfigFlag(InputConfigFlags::SECONDARY);
    if ((!primary && !secondary) || (primary == secondary))
    {
        return ReportError(false, InvalidArgumentError, "binding.ConfigFlags", "Input binding must have PRIMARY or SECONDARY flag set.");
    }

    InputConfigFlags searchFlag = primary ? InputConfigFlags::PRIMARY : InputConfigFlags::SECONDARY;

    for (const InputBindingData& current : mBindings)
    {
        if (current.GetDeviceType() == binding.GetDeviceType() && current.GetConfigFlag(searchFlag))
        {
            return ReportError(false, InvalidArgumentError, "binding", "InputMapping already has primary binding for device.", TInputDeviceType::GetString(binding.GetDeviceType()));
        }
    }

    mBindings.push_back(binding);
    return APIResult<bool>(true);
}
void InputMapping::Clear()
{
    mName.Clear();
    mScope.Clear();
    mBindings.clear();
}

const InputBindingData& InputMapping::GetPrimary(InputDeviceType inputDevice) const
{
    const InputBindingData* data = nullptr;
    for (const InputBindingData& binding : mBindings)
    {
        if (binding.GetDeviceType() == inputDevice && binding.GetConfigFlag(InputConfigFlags::PRIMARY))
        {
            data = &binding;
            break;
        }
    }
    CriticalAssert(data != nullptr);
    return *data;
}
const InputBindingData& InputMapping::GetSecondary(InputDeviceType inputDevice) const
{
    const InputBindingData* data = nullptr;
    for (const InputBindingData& binding : mBindings)
    {
        if (binding.GetDeviceType() == inputDevice && binding.GetConfigFlag(InputConfigFlags::SECONDARY))
        {
            data = &binding;
            break;
        }
    }
    CriticalAssert(data != nullptr);
    return *data;
}

}
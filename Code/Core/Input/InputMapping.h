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
#include "Core/Input/InputBindingData.h"
#include "Core/String/Token.h"
#include "Core/Utility/APIResult.h"
#include "Core/Utility/StdVector.h"

namespace lf 
{
    
class LF_CORE_API InputMapping
{
public:
    InputMapping();
    InputMapping(const Token& name, const Token& scope);

    void Serialize(Stream& s);
    APIResult<bool> Register(const InputBindingData& binding);
    void Clear();

    const InputBindingData& GetPrimary(InputDeviceType inputDevice) const;
    const InputBindingData& GetSecondary(InputDeviceType inputDevice) const;

    LF_INLINE SizeT Size() const { return mBindings.size(); }
    LF_INLINE const InputBindingData& GetBinding(SizeT index) { return mBindings[index]; }
    LF_INLINE const Token& GetName() const { return mName; }
    LF_INLINE const Token& GetScope() const { return mScope; }
    LF_INLINE void SetName(const Token& value) { mName = value; }
    LF_INLINE void SetScope(const Token& value) { mScope = value; }
private:
    Token mName;
    Token mScope;
    TVector<InputBindingData> mBindings;
};
LF_INLINE Stream& operator<<(Stream& s, InputMapping& o)
{
    o.Serialize(s);
    return s;
}

} // namespace lf
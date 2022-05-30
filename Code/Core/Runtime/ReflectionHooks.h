// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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

namespace lf {

class Type;
class Token;
class TypeData;

namespace InternalHooks {

using FindTypeCallback = const Type*(*)(const Token&);

LF_CORE_API extern FindTypeCallback gFindType;


struct TypeRegistrationInfo
{
    using TypeConstructor = void(*)(void*);
    using TypeDestructor = void(*)(void*);
    using TypeRegister = void(*)(TypeData*);


    const char*     mName;
    const Type**    mType;
    const Type**    mSuper;
    TypeConstructor mConstructor;
    TypeDestructor  mDestructor;
    TypeRegister    mRegisterCallback;
    SizeT           mSize;
    SizeT           mAlignment;

    bool            mAbstract;
    
};
using RegisterTypeCallback = void(*)(const TypeRegistrationInfo& info);

LF_CORE_API void RegisterCoreTypes(RegisterTypeCallback registerType);

} // namespace InternalHooks
} // namespace lf
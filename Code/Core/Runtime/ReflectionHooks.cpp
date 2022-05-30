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
#include "Core/PCH.h"
#include "ReflectionHooks.h"

namespace lf {

class AutoTypeRegister
{
public:
    AutoTypeRegister(InternalHooks::RegisterTypeCallback callback)
        : mCallback(callback)
    {}

    template<typename T>
    void AddClass
    (
        const char* name,
        InternalHooks::TypeRegistrationInfo::TypeRegister registerCallback
    )
    {
        InternalHooks::TypeRegistrationInfo info;
        info.mName = name;
        info.mType = &T::sClassType;
        info.mSuper = &T::Super::sClassType;
        info.mConstructor = [](void* pointer) -> void { T* instance = new(pointer)T(); Assert(instance == pointer); };
        info.mDestructor = [](void* pointer) -> void { T* instance = static_cast<T*>(pointer); instance->~T(); };
        info.mRegisterCallback = registerCallback;
        info.mSize = sizeof(T);
        info.mAlignment = alignof(T);
        info.mAbstract = false;
        mCallback(info);
    }

    template<typename T>
    void AddAbstractClass
    (
        const char* name,
        InternalHooks::TypeRegistrationInfo::TypeRegister registerCallback
    )
    {
        InternalHooks::TypeRegistrationInfo info;
        info.mName = name;
        info.mType = &T::sClassType;
        info.mSuper = &T::Super::sClassType;
        info.mRegisterCallback = registerCallback;
        info.mAbstract = true;

        mCallback(info);
    }

    void RegisterAllTypes();

    InternalHooks::RegisterTypeCallback mCallback;
};

namespace InternalHooks {

static const Type* DefaultHookFindType(const Token& ) { return nullptr; }

FindTypeCallback gFindType = DefaultHookFindType;

void RegisterCoreTypes(RegisterTypeCallback registerType)
{
    AutoTypeRegister reg(registerType);
    reg.RegisterAllTypes();
}
} // namespace InternalHooks

void AutoTypeRegister::RegisterAllTypes()
{
    // AddClass<NetRequest>("lf::NetRequest", NetRequest::DefineTypeData);
    // AddClass<NetRequest>("lf::NetRequestHandler", NetRequestHandler::DefineTypeData);
}

} // namespace lf
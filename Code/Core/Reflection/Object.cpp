// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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

#include "Object.h"
#include "Core/Common/Assert.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Reflection/Type.h"

namespace lf {

// Implemented in ReflectionMgr
void Object::DefineTypeData(TypeData*) { }
void Object::InternalTypeInitializer(ProgramContext*) { }
SafeStaticCallback Object::sInternalTypeInitializer(Object::InternalTypeInitializer, 1000, SafeStaticCallback::INIT);
const Type* Object::sClassType = nullptr;

Object::Object() :
    mPointer(),
    mRuntimeType(nullptr)
{

}

Object::~Object()
{

}

bool Object::IsA(const Type* type) const
{
    return mRuntimeType->IsA(type);
}

void Object::Clone(const Object& obj)
{
    AssertError(obj.IsA(GetType()), LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    OnClone(obj);
}

void Object::Serialize(Stream&)
{

}

void Object::OnClone(const Object&)
{

}

} // namespace lf
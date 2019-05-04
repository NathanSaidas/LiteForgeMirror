// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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
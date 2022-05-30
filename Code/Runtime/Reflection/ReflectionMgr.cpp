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
#include "Runtime/PCH.h"
#include "ReflectionMgr.h"

#include "Core/Utility/Log.h"
#include "Core/Utility/StaticCallback.h"

#include "Core/Utility/CmdLine.h"
#include "Core/String/StringCommon.h"

#include "Core/Math/Vector.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"

#include "Core/Runtime/ReflectionHooks.h"

#include <algorithm>

#define BUILD_NATIVE_TYPE(list, type, id, index)                                                            \
    {                                                                                                       \
        TypeInitializerCallback ctor(internal_sys::PtrConstructor<type>);                                       \
        TypeInitializerCallback dtor(internal_sys::PtrDestroy<type>);                                           \
        BuildNativeType(list[index], #type, sizeof(type), alignof(type), id++, static_cast<SizeT>(index), ctor, dtor);     \
    }                                                                                                       \

#define MAP_NATIVE_TYPE(src,dest) NativeMapType(Token(#src),Token(#dest))


#define MAKE_NATIVE_NAME(name) name
#define REGISTER_NATIVE(reg, type) reg.RegisterNativeEx(#type, &MAKE_NATIVE_NAME(mType##type), 0, 0)

namespace lf
{
static ReflectionMgr sReflectionMgr;
static const Type* FindTypeHook(const Token& name)
{
    return GetReflectionMgr().FindType(name);
}

ReflectionMgr& GetReflectionMgr()
{
    return sReflectionMgr;
}

ReflectionMgr::ReflectionMgr() :
    mTypes(),
    mTypeUInt8(nullptr),
    mTypeUInt16(nullptr),
    mTypeUInt32(nullptr),
    mTypeUInt64(nullptr),
    mTypeInt8(nullptr),
    mTypeInt16(nullptr),
    mTypeInt32(nullptr),
    mTypeInt64(nullptr),
    mTypeFloat32(nullptr),
    mTypeFloat64(nullptr),
    mTypeString(nullptr),
    mTypeToken(nullptr),
    mTypeBool(nullptr),
    mTypeVector(nullptr),
    mTypeVector4(nullptr),
    mTypeVector3(nullptr),
    mTypeVector2(nullptr),
    mTypeVoid(nullptr),
    mSilenceLog(false),
    mSilenceWarning(false)
{}
ReflectionMgr::~ReflectionMgr()
{

}

enum LoggingLevel
{
    LL_DEBUG,
    LL_INFO,
    LL_WARNING,
    LL_ERROR
};

void ReflectionMgr::BuildTypes()
{
    String logLevelStr;
    if (!CmdLine::GetArgOption("reflection", "logLevel", logLevelStr))
    {
        logLevelStr = "warning";
    }
    logLevelStr = StrToLower(logLevelStr);

    LoggingLevel logLevel = LL_WARNING;
    if (logLevelStr == "debug")
    {
        logLevel = LL_DEBUG;
    }
    else if (logLevelStr == "info")
    {
        logLevel = LL_INFO;
    }
    else if (logLevelStr == "warning")
    {
        logLevel = LL_WARNING;
    }
    else if (logLevelStr == "error")
    {
        logLevel = LL_ERROR;
    }
    else
    {
        gSysLog.Error(LogMessage("Invalid argument in commandline arg '-reflection /logLevel=") << logLevelStr << "' Acceptable values are 'debug' 'info' 'warning' 'error'");
    }
    
    mSilenceLog = logLevel > LL_INFO;
    mSilenceWarning = logLevel > LL_WARNING;

    StaticTypeRegistry& reg = GetTypeRegistry();
    reg.AddClassEx
    (
        "lf::Object",
        Object::GetClassType(),
        nullptr,
        internal_sys::ConstructInstance<Object>,
        internal_sys::DestroyInstance<Object>,
        Object::DefineTypeData,
        sizeof(Object),
        alignof(Object)
    );

    InternalHooks::RegisterCoreTypes([](const InternalHooks::TypeRegistrationInfo& info)
    {
        if (info.mAbstract)
        {
            GetTypeRegistry().AddAbstractClassEx(info.mName, info.mType, info.mSuper, info.mRegisterCallback);
        }
        else
        {
            GetTypeRegistry().AddClassEx(info.mName, info.mType, info.mSuper, info.mConstructor, info.mDestructor, info.mRegisterCallback, info.mSize, info.mAlignment);
        }
    });

    // reg.

    REGISTER_NATIVE(reg, UInt8);
    REGISTER_NATIVE(reg, UInt16);
    REGISTER_NATIVE(reg, UInt32);
    REGISTER_NATIVE(reg, UInt64);
    REGISTER_NATIVE(reg, Int8);
    REGISTER_NATIVE(reg, Int16);
    REGISTER_NATIVE(reg, Int32);
    REGISTER_NATIVE(reg, Int64);
    REGISTER_NATIVE(reg, Float32);
    REGISTER_NATIVE(reg, Float64);
    REGISTER_NATIVE(reg, String);
    REGISTER_NATIVE(reg, Token);
    reg.RegisterNativeEx("bool", &mTypeBool, sizeof(bool), alignof(bool));
    REGISTER_NATIVE(reg, Vector);
    REGISTER_NATIVE(reg, Vector4);
    REGISTER_NATIVE(reg, Vector3);
    REGISTER_NATIVE(reg, Vector2);
    reg.RegisterNativeEx("void", &mTypeVoid, 0, 0);
    TVector<TypeInfo>& typeInfos = reg.mTypes;
    mTypes.reserve(typeInfos.size());
    mTypes.resize(typeInfos.size());

    // Setup sStaticType and data
    for (SizeT i = 0, numTypes = typeInfos.size(); i < numTypes; ++i)
    {
        TypeInfo& info = typeInfos[i];
        if (!info.mType)
        {
            continue;
        }

        *info.mType = &mTypes[i];
        Type& type = mTypes[i];
        type.mSuper = nullptr;
        type.mFullName = Token(info.mName); // todo: I think its safe to not copy
        type.mSize = info.mSize;
        type.mAlignment = info.mAlignment;
        type.mConstructor = info.mConstructor;
        type.mDestructor = info.mDestructor;
        type.mFlags = 0;
        if (info.mIsAbstract)
        {
            type.mFlags |= Type::TF_ABSTRACT;
        }
        if (info.mIsNative)
        {
            type.mFlags |= Type::TF_NATIVE;
        }
        if (info.mIsEnum)
        {
            type.mFlags |= Type::TF_ENUM;
        }

        String nameStr(info.mName, COPY_ON_WRITE);
        String name;
        SizeT  namespaceChar = nameStr.FindLast(':');
        if (Valid(namespaceChar))
        {
            nameStr.SubString(namespaceChar + 1, name);
            type.mName = Token(name);
        }
        else
        {
            type.mName = type.mFullName;
        }

        if (!mSilenceLog)
        {
            gSysLog.Info(LogMessage("Registered type ") << type.GetFullName());
        }
    }

    // Link Child => Parent
    for (SizeT i = 0, numTypes = typeInfos.size(); i < numTypes; ++i)
    {
        if (!typeInfos[i].mType || !typeInfos[i].mSuper)
        {
            continue;
        }
        mTypes[i].mSuper = *typeInfos[i].mSuper;
    }


    // Query extra data: (find aliased types)
    for (TypeInfo& info : typeInfos)
    {
        if (!info.mType || !info.mRegisterCallback)
        {
            continue;
        }
        TypeData data;
        info.mRegisterCallback(&data);

        // todo: actually handle this case:

    }

    // Query extra data: (create and load)
    for (TypeInfo& info : typeInfos)
    {
        if (!info.mType || !info.mRegisterCallback)
        {
            continue;
        }
        TypeData data;
        info.mRegisterCallback(&data);
        // todo: Build the linked function/method data.
    }
    typeInfos.clear();

    InternalHooks::gFindType = FindTypeHook;
}
void ReflectionMgr::ReleaseTypes()
{
    mTypes.clear();
}

const Type* ReflectionMgr::FindType(const Token& name) const
{
    // todo: When we get 1000's of types we may want to precalculate this with a map
    //       eg. return mTypesByName[name];

    if (name.Empty())
    {
        return nullptr;
    }

    for (const Type& type : mTypes)
    {
        if (type.GetFullName() == name)
        {
            return &type;
        }
    }
    for (const Type& type : mTypes)
    {
        if (type.GetName() == name)
        {
            return &type;
        }
    }
    return nullptr;
}

TVector<const Type*> ReflectionMgr::FindAll(const Type* base, bool includeAbstract) const
{
    TVector<const Type*> types;
    if (!base)
    {
        ReportBugMsgEx("Invalid argument 'base'", LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return types;
    }
    for (const Type& type : mTypes)
    {
        bool passAbstract = includeAbstract || !type.IsAbstract();
        if (passAbstract && type.IsA(base))
        {
            types.push_back(&type);
        }
    }
    return types;
}

ObjectPtr ReflectionMgr::CreateObject(const Type* type) const
{
    if (!type)
    {
        ReportBugMsgEx("Invalid argument 'type'", LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return NULL_PTR;
    }

    
    if (type->IsAbstract())
    {
        gSysLog.Error(LogMessage("Failed to create type, it's abstract. Type=") << type->GetFullName());
        ReportBugMsgEx("Failed to create abstract type", LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return NULL_PTR;
    }

    if (type->IsNative())
    {
        gSysLog.Error(LogMessage("Failed to create type, it's native. Type=") << type->GetFullName());
        ReportBugMsgEx("Failed to create native type", LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return NULL_PTR;
    }

    if (type->IsEnum())
    {
        gSysLog.Error(LogMessage("Failed to create type, it's an enum. Type=") << type->GetFullName());
        ReportBugMsgEx("Failed to create native type", LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return NULL_PTR;
    }

    // Only accept those that are Objects!
    AssertEx(type->IsA(typeof(Object)), LF_ERROR_BAD_STATE, ERROR_API_RUNTIME);

    // Allocate memory for the object using type information
    void* pointer = LFAlloc(type->GetSize(), type->GetAlignment());
    // Call the constructor to setup the object and more importantly the V-table
    type->GetConstructor()(pointer);

    // Create a smart pointer with pointer assignment.
    ObjectPtr obj(static_cast<Object*>(pointer));
    // obj->SetPointer(obj);
    obj->SetType(type);
    return obj;
}

Object* ReflectionMgr::CreateObjectUnsafe(const Type* type) const
{
    if (!type)
    {
        ReportBugMsgEx("Failed to create type", LF_ERROR_INVALID_ARGUMENT, ERROR_API_RUNTIME);
        return nullptr;
    }

    if (type->IsAbstract())
    {
        gSysLog.Error(LogMessage("Failed to create type, it's abstract. Type=") << type->GetFullName());
        ReportBugMsgEx("Failed to create abstract type", LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return nullptr;
    }

    if (type->IsNative())
    {
        gSysLog.Error(LogMessage("Failed to create type, it's native. Type=") << type->GetFullName());
        ReportBugMsgEx("Failed to create native type", LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return nullptr;
    }

    if (type->IsEnum())
    {
        gSysLog.Error(LogMessage("Failed to create type, it's an enum. Type=") << type->GetFullName());
        ReportBugMsgEx("Failed to create native type", LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME);
        return nullptr;
    }

    // Only accept those that are Objects!
    AssertEx(type->IsA(typeof(Object)), LF_ERROR_BAD_STATE, ERROR_API_RUNTIME);
    // Allocate memory for the object using type information
    void* pointer = LFAlloc(type->GetSize(), type->GetAlignment());
    // Call the constructor to setup the object and more importantly the V-table
    type->GetConstructor()(pointer);

    // Create a smart pointer with pointer assignment.
    Object* obj(static_cast<Object*>(pointer));
    obj->SetType(type);
    return obj;
}

void ReflectionMgr::Inherit(Type* target, Type* source)
{
    // Members..
    // Methods
    // Functions..
    if (target != nullptr && source != nullptr)
    {
        target->mMethods.insert(target->mMethods.end(), source->mMethods.begin(), source->mMethods.end());
        target->mFunctions.insert(target->mFunctions.end(), source->mFunctions.begin(), source->mFunctions.end());
        target->mMembers.insert(target->mMembers.end(), source->mMembers.begin(), source->mMembers.end());
    }
}

}
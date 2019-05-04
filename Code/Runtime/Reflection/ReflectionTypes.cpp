// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************

#include "ReflectionTypes.h"

namespace lf
{

    TypeInfo::TypeInfo() :
        mName(nullptr),
        mType(nullptr),
        mSuper(nullptr),
        mConstructor(nullptr),
        mDestructor(nullptr),
        mRegisterCallback(nullptr),
        mSize(0),
        mAlignment(0),
        mIsAbstract(false),
        mIsEnum(false),
        mIsNative(false)
    {

    }

    TypeData::TypeData() :
        mCurrentAccess(AccessSpecifier::AS_PRIVATE),
        mLastMethod(nullptr),
        mLastFunction(nullptr),
        mMethodDatas(),
        mMemberDatas(),
        mFunctionDatas()
    {

    }

    StaticTypeRegistry::StaticTypeRegistry(UInt32 startSize) :
        mTypes()
    {
        mTypes.Reserve(startSize);
    }
    void StaticTypeRegistry::AddClassEx
    (
        const char* name,
        const Type** type,
        const Type** super,
        internal_sys::TypeConstructor constructor,
        internal_sys::TypeDestructor destructor,
        internal_sys::TypeRegister registerCallback,
        SizeT size,
        SizeT alignment
    )
    {
        mTypes.Add(TypeInfo());
        TypeInfo& info = mTypes.GetLast();
        info.mName = name;
        info.mType = type;
        info.mSuper = super;
        info.mConstructor = constructor;
        info.mDestructor = destructor;
        info.mRegisterCallback = registerCallback;
        info.mSize = size;
        info.mAlignment = alignment;
    }

    void StaticTypeRegistry::AddAbstractClassEx
    (
        const char* name,
        const Type** type,
        const Type** super,
        internal_sys::TypeRegister registerCallback
    )
    {
        mTypes.Add(TypeInfo());
        TypeInfo& info = mTypes.GetLast();
        info.mName = name;
        info.mType = type;
        info.mSuper = super;
        info.mRegisterCallback = registerCallback;
        info.mIsAbstract = true;
    }

    void StaticTypeRegistry::RegisterNativeEx
    (
        const char* name,
        const Type** type,
        SizeT size,
        SizeT alignment
    )
    {
        mTypes.Add(TypeInfo());
        TypeInfo& info = mTypes.GetLast();
        info.mName = name;
        info.mType = type;
        info.mSize = size;
        info.mAlignment = alignment;
        info.mIsNative = true;
    }

    StaticTypeRegistry& GetTypeRegistry()
    {
        static StaticTypeRegistry typeRegistry(1000);
        return typeRegistry;
    }
} // namespace lf
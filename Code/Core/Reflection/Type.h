// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_TYPE_H
#define LF_CORE_TYPE_H

#include "Core/Reflection/AccessSpecifier.h"
#include "Core/Reflection/MemberInfo.h"
#include "Core/Reflection/MethodInfo.h"
#include "Core/Reflection/FunctionInfo.h"

namespace lf {

class Type;

using MemberInfoArray = TArray<MemberInfo>;
using MethodInfoArray = TArray<MethodInfo>;
using FunctionInfoArray = TArray<FunctionInfo>;

class LF_CORE_API Type
{
    friend class ReflectionMgr;
public:
    enum TypeFlags
    {
        TF_ABSTRACT = 1 << 0,
        TF_ENUM     = 1 << 1,
        TF_NATIVE   = 1 << 2
    };
    using Constructor = void(*)(void*);
    using Destructor = void(*)(void*);

    Type();

    bool IsA(const Type* other) const;
    LF_INLINE bool operator==(const Type& other) const { return mTypeID == other.mTypeID; }
    LF_INLINE bool operator!=(const Type& other) const { return mTypeID != other.mTypeID; }

    LF_INLINE const Token& GetName() const { return mName; }
    LF_INLINE const Token& GetFullName() const { return mFullName; }
    LF_INLINE const Type*  GetSuper() const { return mSuper; }
    LF_INLINE SizeT GetSize() const { return mSize; }
    LF_INLINE SizeT GetAlignment() const { return mAlignment; }
    LF_INLINE SizeT GetTypeID() const { return mTypeID; }
    LF_INLINE UInt8 GetFlags() const { return mFlags; }
    LF_INLINE Constructor GetConstructor() const { return mConstructor; }
    LF_INLINE Destructor  GetDestructor() const { return mDestructor; }
    LF_INLINE const MemberInfoArray& GetMembers() const { return mMembers; }
    LF_INLINE const MethodInfoArray& GetMethods() const { return mMethods; }
    LF_INLINE const FunctionInfoArray& GetFunctions() const { return mFunctions; }
    LF_INLINE bool IsAbstract() const { return (mFlags & TF_ABSTRACT) > 0; }
    LF_INLINE bool IsEnum() const { return (mFlags & TF_ENUM) > 0; }
    LF_INLINE bool IsNative() const { return (mFlags & TF_NATIVE) > 0; }
private:
    Token               mName;
    Token               mFullName;
    const Type*         mSuper;
    SizeT               mSize;
    SizeT               mAlignment;
    SizeT               mTypeID;
    UInt8               mFlags;
    Constructor         mConstructor;
    Destructor          mDestructor;
    MemberInfoArray     mMembers;
    MethodInfoArray     mMethods;
    FunctionInfoArray   mFunctions;
};

} // namespace lf

#endif // LF_CORE_TYPE_H
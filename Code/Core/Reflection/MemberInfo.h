// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_MEMBER_INFO_H
#define LF_CORE_MEMBER_INFO_H

#include "Core/Common/API.h"
#include "Core/String/Token.h"

#include "Core/Reflection/AccessSpecifier.h"

namespace lf {

class Type;

class LF_CORE_API MemberInfo
{
public:
    MemberInfo() :
        mName(),
        mType(nullptr),
        mMemberOffset(0),
        mAccessSpecifier(AccessSpecifier::AS_PUBLIC)
    {}
    MemberInfo(const Token& name, const Type* type, SizeT memberOffset, AccessSpecifier accessSpecifier) :
        mName(name),
        mType(type),
        mMemberOffset(memberOffset),
        mAccessSpecifier(accessSpecifier)
    {}
    LF_INLINE const Token& GetName() const { return mName; }
    LF_INLINE const Type* GetType() const { return mType; }
    LF_INLINE SizeT GetMemberOffset() const { return mMemberOffset; }
    LF_INLINE AccessSpecifier GetAccessSpecifier() const { return mAccessSpecifier; }

    template<typename R, typename T>
    LF_INLINE R* OffsetInstance(T* instance) const
    {
        return reinterpret_cast<R*>(reinterpret_cast<UIntPtrT>(instance) + static_cast<UIntPtrT>(mMemberOffset));
    }
private:
    Token           mName;
    const Type*     mType;
    SizeT           mMemberOffset;
    AccessSpecifier mAccessSpecifier;
};

}

#endif // LF_CORE_MEMBER_INFO_H
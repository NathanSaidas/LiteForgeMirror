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
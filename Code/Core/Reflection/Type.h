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

#include "Core/Reflection/AccessSpecifier.h"
#include "Core/Reflection/MemberInfo.h"
#include "Core/Reflection/MethodInfo.h"
#include "Core/Reflection/FunctionInfo.h"

namespace lf {

class Type;

using MemberInfoArray = TVector<MemberInfo>;
using MethodInfoArray = TVector<MethodInfo>;
using FunctionInfoArray = TVector<FunctionInfo>;

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
    // ********************************************************************
    // Returns the inheritence distance between two types.
    // 
    // class A;
    // class B : A;
    // class C : B;
    // class D : A;
    //
    // A.Distance(A) == 0
    // A.Distance(B) == INVALID (A is not B)
    // B.Distance(A) == 1
    // C.Distance(A) == 2
    // D.Distance(B) == INVALID (D is not B)
    // ********************************************************************
    SizeT Distance(const Type* other) const;
    LF_INLINE bool operator==(const Type& other) const { return mFullName == other.mFullName; }
    LF_INLINE bool operator!=(const Type& other) const { return mFullName != other.mFullName; }

    LF_INLINE const Token& GetName() const { return mName; }
    LF_INLINE const Token& GetFullName() const { return mFullName; }
    LF_INLINE const Type*  GetSuper() const { return mSuper; }
    LF_INLINE SizeT GetSize() const { return mSize; }
    LF_INLINE SizeT GetAlignment() const { return mAlignment; }
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
    UInt8               mFlags;
    Constructor         mConstructor;
    Destructor          mDestructor;
    MemberInfoArray     mMembers;
    MethodInfoArray     mMethods;
    FunctionInfoArray   mFunctions;
};

} // namespace lf
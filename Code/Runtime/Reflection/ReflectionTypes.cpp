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
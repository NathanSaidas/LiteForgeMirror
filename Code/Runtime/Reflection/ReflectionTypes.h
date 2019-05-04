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
#ifndef LF_RUNTIME_REFLECTION_TYPES_H
#define LF_RUNTIME_REFLECTION_TYPES_H

#include "Core/Common/Types.h"
#include "Core/Common/Enum.h"
#include "Core/String/String.h"
#include "Core/String/StringCommon.h"
#include "Core/String/Token.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/SmartCallback.h"

#include "Core/Reflection/AccessSpecifier.h"
#include "Core/Reflection/FunctionInfo.h"
#include "Core/Reflection/MemberInfo.h"
#include "Core/Reflection/MethodInfo.h"
#include "Core/Reflection/ParamInfo.h"

namespace lf
{

    // Use NO_REFLECTION if there is no reflection in type initialization.
#define LF_REFLECTION_PARAM use_NO_REFLECTION_to_remove_warning

    // Still creates a type, but instructs the static initializer not to generate extra data.
#define NO_REFLECTION (LF_REFLECTION_PARAM ); return
#define OFFSET_OF(type,member) offsetof(type,member)
    // Internal:
#define LF_REFLECT_CALLBACK(name, returnType, ...) \
    TCallback<returnType, __VA_ARGS__>(&name, TWeakPointer<ClassType>())

#define typeof(type) type::sClassType

    class Type;
    /**
    * Use this inside the class. (Note, it always makes the next members private.)
    * @param type = The type this class is
    * @param super = The super type of this class. (There must always be a super)
    */
#define DECLARE_CLASS(type,super)                                           \
private:                                                                    \
    static ::lf::SafeStaticCallback sInternalTypeInitializer;               \
    static void InternalTypeInitializer(::lf::ProgramContext*);            \
    static void DefineTypeData(::lf::TypeData*);                           \
    using ClassType = type;                                                 \
public:                                                                     \
    using Super = super;                                                    \
    static const ::lf::Type* sClassType;                                   \
    LF_INLINE const ::lf::TWeakPointer<type>& GetPointer() const                  \
    { return lf::StaticCast<::lf::TWeakPointer<type>>(mPointer); }                    \
private:    

    /**
    * Use this in the class source file.
    * If there is no reflection just use NO_REFLECTION in the function body.
    * @param type = The full name of the class.
    */
#define DEFINE_CLASS(type)                                                          \
void type::InternalTypeInitializer(::lf::ProgramContext*)                                            \
{                                                                               \
    ::lf::GetTypeRegistry().AddClass<type>(#type, type::DefineTypeData);  \
}                                                                               \
::lf::SafeStaticCallback type::sInternalTypeInitializer(InternalTypeInitializer, 1000, ::lf::SafeStaticCallback::INIT);  \
const ::lf::Type* type::sClassType = nullptr; \
void type::DefineTypeData(::lf::TypeData* LF_REFLECTION_PARAM)                  \


#define DEFINE_ABSTRACT_CLASS(type)                                                 \
void type::InternalTypeInitializer(ProgramContext*)                           \
{                                                                               \
    ::lf::GetTypeRegistry().AddAbstractClass<type>(#type, type::DefineTypeData);\
}                                                                               \
::lf::SafeStaticCallback type::sInternalTypeInitializer(InternalTypeInitializer, 1000, ::lf::SafeStaticCallback::INIT);    \
const ::lf::Type* type::sClassType = nullptr;                                  \
void type::DefineTypeData(::lf::TypeData* LF_REFLECTION_PARAM)                  \



    // Add function to reflection data, see LF_REFLECT_ARGUMENT to add arguments
#define LF_REFLECT_FUNCTION(name, returnType, ...) \
    LF_REFLECTION_PARAM->AddFunctionData<returnType, __VA_ARGS__>(#name, name, #returnType)

    // Add method to reflection data , see LF_REFLECT_ARGUMENT to add arguments
#define LF_REFLECT_METHOD(name, returnType, ...)\
    LF_REFLECTION_PARAM->AddMethodData<ClassType, returnType, __VA_ARGS__>(#name, LF_REFLECT_CALLBACK(name, returnType, __VA_ARGS__), #returnType)

    // Add field/member to reflection data
#define LF_REFLECT_FIELD(name, type)\
    LF_REFLECTION_PARAM->AddMemberData(#name, #type, OFFSET_OF(ClassType, name))

    // Adds parameter reflection info to the last added function/method
#define LF_REFLECT_ARGUMENT(name, type)\
    LF_REFLECTION_PARAM->AddParameter(#name, #type)

using MemberOffset = SizeT;
struct TypeInfo;     // done:
struct MethodData;   // done
struct MemberData;   // done
struct FunctionData; // done
class TypeData;      // done
class StaticTypeRegistry; // done


// todo: Move out of namespace or rename? eg internal_runtime
namespace internal_sys {
    using TypeConstructor = void(*)(void*);
    using TypeDestructor = void(*)(void*);
    using TypeRegister = void(*)(TypeData*);

    template<typename T>
    void ConstructInstance(void* pointer)
    {
        T* instance = new(pointer)T();
        // If this trips, you allocated with wrong alignment.
        Assert(instance == pointer);
    }
    template<typename T>
    void DestroyInstance(void* pointer)
    {
        T* instance = static_cast<T*>(pointer);
        instance->~T();
    }
} // namespace internal_sys



struct LF_RUNTIME_API TypeInfo
{
    TypeInfo();

    const char*  mName;
    const Type** mType;
    const Type** mSuper;
    internal_sys::TypeConstructor mConstructor;
    internal_sys::TypeDestructor  mDestructor;
    internal_sys::TypeRegister    mRegisterCallback;
    SizeT mSize;
    SizeT mAlignment;
    bool  mIsAbstract;
    bool  mIsEnum;
    bool  mIsNative;
};

struct TypeArgument
{
    const char* mName;
    const char* mType;
};
using TypeArgumentArray = TStaticArray<TypeArgument, 4>;

struct LF_RUNTIME_API MethodData
{
    const char* mName;
    const char* mReturnType;
    TypeArgumentArray mArguments;
    CallbackHandle    mCallback;
    AccessSpecifier   mAccessSpecifier;
};

struct LF_RUNTIME_API MemberData
{
    const char*  mName;
    const char*  mType;
    MemberOffset mOffset;
    AccessSpecifier mAccessSpecifier;
};

struct LF_RUNTIME_API FunctionData
{
    const char* mName;
    const char* mReturnType;
    TypeArgumentArray mArguments;
    CallbackHandle    mCallback;
    AccessSpecifier   mAccessSpecifier;
};

class LF_RUNTIME_API TypeData
{
public:
    TypeData();

    void SetAccess(AccessSpecifier access) { mCurrentAccess = access; }

    template<typename T, typename R, typename ... ARGS>
    void AddMethodData
    (
        const char* name,
        const TCallback<R, ARGS...>& callback,
        const char* returnType
    )
    {
        Reset();
        mMethodDatas.Add(MethodData());
        MethodData& method = mMethodDatas.GetLast();
        method.mName = name;
        method.mAccessSpecifier = mCurrentAccess;
        method.mCallback.Assign(callback);
        method.mReturnType = returnType;
        mLastMethod = &method;
    }

    template<typename R, typename ... ARGS>
    void AddFunctionData
    (
        const char* name,
        const TCallback<R, ARGS...>& callback,
        const char* returnType
    )
    {
        Reset();
        mFunctionDatas.Add(FunctionData());
        FunctionData& function = mFunctionDatas.GetLast();
        function.mName = name;
        function.mAccessSpecifier = mCurrentAccess;
        function.mCallback.Assign(callback);
        function.mReturnType = returnType;
        mLastFunction = &function;
    }


    void AddMemberData(const char* name, const char* typeName, MemberOffset offset)
    {
        Reset();
        mMemberDatas.Add(MemberData());
        MemberData& member = mMemberDatas.GetLast();
        member.mName = name;
        member.mOffset = offset;
        member.mAccessSpecifier = mCurrentAccess;
        member.mType = typeName;
        mMemberDatas.Add(member);
    }

    void AddParameter(const char* name, const char* type)
    {
        if (mLastMethod)
        {
            mLastMethod->mArguments.Add(TypeArgument());
            TypeArgument& arg = mLastMethod->mArguments.GetLast();
            arg.mName = name;
            arg.mType = type;
        }
        else if (mLastFunction)
        {
            mLastFunction->mArguments.Add(TypeArgument());
            TypeArgument& arg = mLastFunction->mArguments.GetLast();
            arg.mName = name;
            arg.mType = type;
        }
    }

private:
    void Reset()
    {
        mLastFunction = nullptr;
        mLastMethod = nullptr;
    }
    AccessSpecifier mCurrentAccess;
    MethodData*     mLastMethod;
    FunctionData*   mLastFunction;

    TArray<MethodData> mMethodDatas;
    TArray<MemberData> mMemberDatas;
    TArray<FunctionData> mFunctionDatas;

    friend class ReflectionMgr;
};

// Container for all types to register at "static init" time.
class LF_RUNTIME_API StaticTypeRegistry
{
    friend class ReflectionMgr;
public:
    StaticTypeRegistry(UInt32 startSize);
    void AddClassEx
    (
        const char* name,
        const Type** type,
        const Type** super,
        internal_sys::TypeConstructor constructor,
        internal_sys::TypeDestructor destructor,
        internal_sys::TypeRegister registerCallback,
        SizeT size,
        SizeT alignment
    );

    void AddAbstractClassEx
    (
        const char* name,
        const Type** type,
        const Type** super,
        internal_sys::TypeRegister registerCallback
    );

    void RegisterNativeEx
    (
        const char* name,
        const Type** type,
        SizeT size,
        SizeT alignment
    );

    template<typename T>
    void AddClass
    (
        const char* name,
        internal_sys::TypeRegister registerCallback
    )
    {
        AddClassEx
        (
            name,
            &T::sClassType,
            &T::Super::sClassType,
            internal_sys::ConstructInstance<T>,
            internal_sys::DestroyInstance<T>,
            registerCallback,
            sizeof(T),
            alignof(T)
        );
    }

    template<typename T>
    void AddAbstractClass
    (
        const char* name,
        internal_sys::TypeRegister registerCallback
    )
    {
        AddAbstractClassEx
        (
            name,
            &T::sClassType,
            &T::Super::sClassType,
            registerCallback
        );
    }

    template<typename T>
    void RegisterNative
    (
        const char* name,
        const Type** type
    )
    {
        RegisterNativeEx(name, type, sizeof(T), alignof(T));
    }

    void Clear()
    {
        mTypes.Clear();
    }
private:
    TArray<TypeInfo> mTypes;
};

LF_RUNTIME_API StaticTypeRegistry& GetTypeRegistry();
} // namespace lf

#endif // LF_RUNTIME_REFLECTION_TYPES_H
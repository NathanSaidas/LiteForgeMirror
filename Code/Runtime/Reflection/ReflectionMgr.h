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
#ifndef LF_RUNTIME_REFLECTION_MGR_H
#define LF_RUNTIME_REFLECTION_MGR_H

#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Reflection/Type.h"
#include "Core/Reflection/Object.h"
#include "Core/Utility/StdVector.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

class Type;
class MemberInfo;
class MethodInfo;
using FunctionInfo = MethodInfo;

using TypeIterator = TVector<Type>::const_iterator; // todo; Is Enumerator a better idea for this..?

class LF_RUNTIME_API ReflectionMgr
{
public:
    ReflectionMgr();
    ~ReflectionMgr();

    // **********************************
    // This function should be called before anything else. It's a heavy function as it will first analyze all
    // the types that registered with the AutoTypeInitializer and then invoke the callback to gather extra meta
    // data about the type. TypeID's are generated at static initialization time. They can also represent the
    // order in which the type was statically initialized.
    //
    // - Register (Creating TypeDatas)
    // - Build (Creating Types)
    // - Linking (Linking all the 'TypeReference' objects to the types. eg Figuring out the Super location and id.)
    // **********************************
    void BuildTypes();

    // **********************************
    // This function is heavy but lighter than BuildTypes, It releases all the memory for the Types.
    // **********************************
    void ReleaseTypes();

    // **********************************
    // Search up a type by name only.
    //
    // @param name -- The name (fullname or name) of the type you're looking for
    // **********************************
    const Type* FindType(const Token& name) const;
    // **********************************
    // Search for all types that are IsA(base)
    //
    // @param base -- The type to search for
    // **********************************
    TVector<const Type*> FindAll(const Type* base, bool includeAbstract = true) const;

    // **********************************
    // Allocates memory for the specified type and initializes using reflection
    // to invoke the constructor.
    //
    // note: Native types cannot be allocated via this interface
    //
    // @param type   -- The type you wish to construct
    // @returns      -- The fully constructed object wrapped in a smart pointer.
    // **********************************
    ObjectPtr CreateObject(const Type* type) const;
    // **********************************
    // Allocates memory for the specified type and initializes using reflection
    // to invoke the constructor.
    //
    // note: Native types cannot be allocated via this interface
    // note: This method is considered 'unsafe' meaning you must manage the pointer returned.
    //       Should you later wrap it in a smart pointer don't forget to call SetPointer on
    //       the object to complete the link.
    //
    // @param type   -- The type you wish to construct
    // @returns      -- The fully constructed object (don't forget to release the memory)
    // **********************************
    Object* CreateObjectUnsafe(const Type* type) const;

    // **********************************
    // Allocates memory for the specified type and initializes using reflection
    // to invoke the constructor.
    //
    // note: Native types cannot be allocated via this interface
    //
    // @param type -- The type you wish to construct
    // @returns    -- The fully constructed object wrapped in a smart pointer. 
    // **********************************
    template<typename T>
    TStrongPointer<T> Create(const Type* type = nullptr) const
    {
        if (!type)
        {
            type = typeof(T);
        }
        if (type->IsAbstract() || !type->IsA(typeof(T)))
        {
            return NULL_PTR;
        }
        TStrongPointer<T> object = StaticCast<TStrongPointer<T>>(CreateObject(type));
        InitializeConvertible(object, typename T::PointerConvertible());
        return object;
    }
    // **********************************
    // Allocates memory for the specified type and initializes using reflection
    // to invoke the constructor.
    //
    // note: Native types cannot be allocated via this interface
    //
    // @param type -- The type you wish to construct
    // @returns    -- The fully constructed object wrapped in a smart pointer. 
    // **********************************
    template<typename T>
    TAtomicStrongPointer<T> CreateAtomic(const Type* type = nullptr) const
    {
        if (!type)
        {
            type = typeof(T);
        }
        if (type->IsAbstract() || !type->IsA(typeof(T)))
        {
            return NULL_PTR;
        }
        TAtomicStrongPointer<T> object = StaticCast<TAtomicStrongPointer<T>>(CreateObject(type));
        InitializeConvertible(object, typename T::PointerConvertible());
        return object;
    }

    // **********************************
    // Allocates memory for the specified type and initializes using reflection
    // to invoke the constructor.
    //
    // note: Native types cannot be allocated via this interface
    // note: This method is considered 'unsafe' meaning you must manage the pointer returned.
    //       Should you later wrap it in a smart pointer don't forget to call SetPointer on
    //       the object to complete the link.
    //
    // @param type -- The type you wish to construct
    // @returns    -- The fully constructed object (don't forget to release the memory)
    // **********************************
    template<typename T>
    T* CreateUnsafe(const Type* type = nullptr) const
    {
        if (!type)
        {
            type = typeof(T);
        }

        if (type->IsAbstract() || !type->IsA(typeof(T)))
        {
            return nullptr;
        }
        return static_cast<T*>(CreateObjectUnsafe(type));
    }

    TypeIterator GetTypesBegin() const { return mTypes.begin(); }
    TypeIterator GetTypesEnd() const { return mTypes.end(); }

private:
    void Inherit(Type* target, Type* source);

    template<typename T>
    void InitializeConvertible(TAtomicStrongPointer<T>& object, PointerConvertibleType) const
    {
        object->GetWeakPointer() = object;
    }

    template<typename T>
    void InitializeConvertible(TAtomicStrongPointer<T>& , void*) const
    {
    }

    template<typename T>
    void InitializeConvertible(TStrongPointer<T>& object, PointerConvertibleType) const
    {
        object->GetWeakPointer() = object;
    }

    template<typename T>
    void InitializeConvertible(TStrongPointer<T>&, void*) const
    {
    }

    TVector<Type> mTypes;

    // Native types:
    const Type* mTypeUInt8;
    const Type* mTypeUInt16;
    const Type* mTypeUInt32;
    const Type* mTypeUInt64;
    const Type* mTypeInt8;
    const Type* mTypeInt16;
    const Type* mTypeInt32;
    const Type* mTypeInt64;
    const Type* mTypeFloat32;
    const Type* mTypeFloat64;
    const Type* mTypeString;
    const Type* mTypeToken;
    const Type* mTypeBool;
    const Type* mTypeVector;
    const Type* mTypeVector4;
    const Type* mTypeVector3;
    const Type* mTypeVector2;
    const Type* mTypeVoid;

    bool mSilenceLog;
    bool mSilenceWarning;
};
LF_RUNTIME_API ReflectionMgr& GetReflectionMgr();


} // namespace lf

#endif // LF_RUNTIME_REFLECTION_MGR_H
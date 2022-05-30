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

#include "Core/Utility/StaticCallback.h"
#include "Core/Memory/SmartPointer.h"

namespace lf {

class Object;
using ObjectPtr = TStrongPointer<Object>;
using ObjectWPtr = TWeakPointer<Object>;
class TypeData;
class Type;
class Stream;

class LF_CORE_API Object
{
private:
    friend class ReflectionMgr;
    // **********************************
    // Callback for defining reflection data on the type.
    // **********************************
    static void DefineTypeData(TypeData*);
    // **********************************
    // Callback to register the define callback in safe-static initialization order
    // **********************************
    static void InternalTypeInitializer(ProgramContext*);
    // **********************************
    // Callback handle
    // **********************************
    static SafeStaticCallback sInternalTypeInitializer;
    using ClassType = Object;
    // **********************************
    // Static reference to the processed reflection data.
    // **********************************
    static const Type* sClassType;
public:
    using PointerConvertible = void*;
    static const Type** GetClassType();

    // **********************************
    // Object Constructor
    // **********************************
    Object();
    // **********************************
    // Object Destructor
    // **********************************
    virtual ~Object();

    // **********************************
    // Object Accessors
    // **********************************
    // LF_INLINE const ObjectWPtr& GetPointer() const { return mPointer; }
    // LF_INLINE void SetPointer(const ObjectWPtr& pointer) { mPointer = pointer; }

    bool IsA(const Type* type) const;
    LF_INLINE void SetType(const Type* type) { mRuntimeType = type; }
    LF_INLINE const Type* GetType() const { return mRuntimeType; }

    // **********************************
    // Invoke to clone properties of the 'object' into 'this'
    // See OnClone for overriding details
    // **********************************
    void Clone(const Object& obj);
    // **********************************
    // Invoke with Stream to serialize/deserialize an object.
    // **********************************
    virtual void Serialize(Stream& stream);
protected:
    // **********************************
    // Invoke with Stream to serialize/deserialize an object.
    // **********************************
    virtual void OnClone(const Object& obj);

    // ObjectWPtr  mPointer;
    const Type* mRuntimeType;
};

} // namespace lf
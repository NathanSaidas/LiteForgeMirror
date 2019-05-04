// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_RUNTIME_OBJECT_H
#define LF_RUNTIME_OBJECT_H

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
public:
    // **********************************
    // Static reference to the processed reflection data.
    // **********************************
    static const Type* sClassType;

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
    LF_INLINE const ObjectWPtr& GetPointer() const { return mPointer; }
    LF_INLINE void SetPointer(const ObjectWPtr& pointer) { mPointer = pointer; }

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

    ObjectWPtr  mPointer;
    const Type* mRuntimeType;
};

} // namespace lf

#endif // LF_RUNTIME_OBJECT_H
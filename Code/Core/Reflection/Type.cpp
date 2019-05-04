// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "Type.h"

namespace lf {

Type::Type() : 
mName(),
mFullName(),
mSuper(nullptr),
mSize(0),
mAlignment(0),
mTypeID(0),
mFlags(static_cast<TypeFlags>(0)),
mConstructor(nullptr),
mDestructor(nullptr),
mMembers(),
mMethods(),
mFunctions()
{}

bool Type::IsA(const Type* other) const
{
    const Type* iter = this;
    while (iter)
    {
        if (iter == other)
        {
            return true;
        }
        iter = iter->mSuper;
    }
    return false;
}

} // namespace lf
// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_PARAM_INFO_H
#define LF_CORE_PARAM_INFO_H

#include "Core/Common/API.h"
#include "Core/String/Token.h"

namespace lf {

class Type;

class LF_CORE_API ParamInfo
{
public:
    ParamInfo() : mName(), mType(nullptr) {}
    ParamInfo(const Token& name, const Type* type) : mName(name), mType(type) {}
    LF_INLINE const Token& GetName() const { return mName; }
    LF_INLINE const Type* GetType() const { return mType; }
private:
    Token mName;
    const Type* mType;
};

} // namespace lf

#endif // LF_CORE_PARAM_INFO_H
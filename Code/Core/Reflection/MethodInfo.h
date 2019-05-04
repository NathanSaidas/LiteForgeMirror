// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_METHOD_INFO_H
#define LF_CORE_METHOD_INFO_H

#include "Core/Common/API.h"
#include "Core/String/Token.h"
#include "Core/Utility/Array.h"

#include "Core/Reflection/AccessSpecifier.h"
#include "Core/Reflection/ParamInfo.h"

namespace lf {

class Type;
using ParamInfoArray = TArray<ParamInfo>;

class LF_CORE_API MethodInfo
{
public:
    MethodInfo() : 
        mCallback(),
        mParamInfos(),
        mName(),
        mReturnType(nullptr),
        mAccessSpecifier()
    {}
    MethodInfo(void* callback, const ParamInfoArray& paramInfos, const Token& name, const Type* returnType, AccessSpecifier accessSpecifier) :
        mCallback(callback),
        mParamInfos(paramInfos),
        mName(name),
        mReturnType(returnType),
        mAccessSpecifier(accessSpecifier)
    {}
    LF_INLINE void* GetCallback() const { return mCallback; }
    LF_INLINE const ParamInfoArray& GetParamInfos() const { return mParamInfos; }
    LF_INLINE const Token& GetName() const { return mName; }
    LF_INLINE const Type* GetReturnType() const { return mReturnType; }
    LF_INLINE AccessSpecifier GetAccessSpecifier() const { return mAccessSpecifier; }
private:
    void*           mCallback;
    ParamInfoArray  mParamInfos;
    Token           mName;
    const Type*     mReturnType;
    AccessSpecifier mAccessSpecifier;
};

} // namespace lf

#endif // LF_CORE_METHOD_INFO_H
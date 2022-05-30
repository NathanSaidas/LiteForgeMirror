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

#include "Core/Common/API.h"
#include "Core/String/Token.h"
#include "Core/Utility/Array.h"

#include "Core/Reflection/AccessSpecifier.h"
#include "Core/Reflection/ParamInfo.h"

namespace lf {

class Type;
using ParamInfoArray = TVector<ParamInfo>;

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
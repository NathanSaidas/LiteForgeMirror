// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Core/Utility/APIResult.h"

// This file serves the purpose of providing standardize error messages
// This isn't the only file that will define error types, there might be
// other more specific cases
namespace lf {
 
class Type;

// ********************************************************************
// The standard API error for null arguments. Use this when an argument
// is null and the API expects it to not be null.
// 
// @param argument -- The name of the argument
// ********************************************************************
namespace ArgumentNullError
{
    LF_CORE_API ErrorBase* Create(const ErrorBase::Info& info, const char* argument);
}
// ********************************************************************
// The standard API error for invalid arguments. Use this when an 
// argument is considered not valid.
// 
// @param argument -- The name of the argument
// @param reason -- The reason the argument is not valid
// ********************************************************************
namespace InvalidArgumentError
{
    
    LF_CORE_API ErrorBase* Create(const ErrorBase::Info& info, const char* argument, const char* reason, const char* context = nullptr);
}
// ********************************************************************
// The standard API error for invalid argument types. Use this when
// you receive an argument that is not the expected type
// 
// foo->IsA(bar) => InvalidTypeArgumentError("foo", bar, foo)
// 
// @param argument -- The name of the argument
// @param expected -- The expected type
// @param got -- The type the function received.
// ********************************************************************
namespace InvalidTypeArgumentError
{
    LF_CORE_API ErrorBase* Create(const ErrorBase::Info& info, const char* argument, const Type* expected, const Type* got);
}
namespace InvalidOperationError
{
    // ********************************************************************
    // The standard API error for invalid operation. (This usually means
    // you're trying to do something.
    // 
    // TODO:
    // ********************************************************************
    // LF_CORE_API ErrorBase* Create(const ErrorBase::Info& info, const char* context, const char* message);
}
// ********************************************************************
// The standard API error for when an operation fails to.
// 
// @param [const char*] message -- The message of the failing operation.
// @param [const char*] context -- The contextual object failing in the operation.
// ********************************************************************
namespace OperationFailureError
{
    LF_CORE_API ErrorBase* Create(const ErrorBase::Info& info, const char* message, const char* context);
}
// ********************************************************************
// The standard API error for when an operation fails to create an object
// because the type is abstract.
// 
// @param type -- The type that failed to be created.
// ********************************************************************
namespace OperationFailureAbstractTypeError
{
    LF_CORE_API ErrorBase* Create(const ErrorBase::Info& info, const Type* type);
}


} // namespace lf


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
#include "Core/PCH.h"
#include "Error.h"
#include "Core/String/String.h"
#include "Core/String/StaticString.h"
#include "Core/Utility/StandardError.h"
#include "Core/Reflection/Type.h"
#include <cstdio>

namespace lf {

class ArgumentNullErrorType : public ErrorBase
{
public:
    ArgumentNullErrorType(const char* argumentName);
    const String& GetErrorMessage() const override { return mErrorMessage; }
    String mErrorMessage;
};
ErrorBase* ArgumentNullError::Create(const ErrorBase::Info& info, const char* argument)
{
    return ErrorUtil::MakeError<ArgumentNullErrorType>(info, argument);
}
class InvalidArgumentErrorType : public StandardError
{
public:
    InvalidArgumentErrorType(const char* argumentName, const char* reason, const char* context);
};
ErrorBase* InvalidArgumentError::Create(const ErrorBase::Info& info, const char* argument, const char* reason, const char* context)
{
    return ErrorUtil::MakeError<InvalidArgumentErrorType>(info, argument, reason, context);
}

class InvalidTypeArgumentErrorType : public StandardError
{
public:
    InvalidTypeArgumentErrorType(const char* argument, const Type* expected, const Type* got);
};
ErrorBase* InvalidTypeArgumentError::Create(const ErrorBase::Info& info, const char* argument, const Type* expected, const Type* got)
{
    return ErrorUtil::MakeError<InvalidTypeArgumentErrorType>(info, argument, expected, got);
}

class OperationFailureErrorType : public StandardError
{
public:
    OperationFailureErrorType(const char* message, const char* context);
};
ErrorBase* OperationFailureError::Create(const ErrorBase::Info& info, const char* message, const char* context)
{
    return ErrorUtil::MakeError<OperationFailureErrorType>(info, message, context);
}


class OperationFailureAbstractTypeErrorType : public StandardError
{
public:
    OperationFailureAbstractTypeErrorType(const Type* type);
};
ErrorBase* OperationFailureAbstractTypeError::Create(const ErrorBase::Info& info, const Type* type)
{
    return ErrorUtil::MakeError<OperationFailureAbstractTypeErrorType>(info, type);
}




ArgumentNullErrorType::ArgumentNullErrorType(const char* argumentName)
: ErrorBase(), mErrorMessage()
{
    // Function called with invalid argument. "%s"
    static const auto errorMessage = StaticString("Function called with null argument. ");
    static const auto formatBuffer = StaticString(" \"\"");
    const SizeT stringSize =
        errorMessage.Size()     // Original Error Message
        + StrLen(argumentName)  // Argument String
        + formatBuffer.Size()   // Format
        + 1;                    // Null Terminator

    char* memory = reinterpret_cast<char*>(Allocate(stringSize, 1));
    if (memory == nullptr)
    {
        mErrorMessage.Assign(GetUnknownErrorString(), COPY_ON_WRITE);
    }
    else
    {
        snprintf(memory, stringSize, "%s \"%s\"", errorMessage.CStr(), argumentName);
        mErrorMessage.Assign(memory, COPY_ON_WRITE);
    }

}

InvalidArgumentErrorType::InvalidArgumentErrorType(const char* argumentName, const char* reason, const char* context)
: StandardError()
{
    // Function called with invalid argument. "%s", "%s"
    static const auto errorMessage = StaticString("Function called with invalid argument. ");
    static const auto formatBuffer = StaticString(" \"\" ");
    static const auto contextMsg = StaticString(" Context=");
    const SizeT contextSize = context ? contextMsg.Size() + StrLen(context) : 0;
    const SizeT stringSize =
        errorMessage.Size()     // Original Error Message
        + StrLen(argumentName)  // Argument String
        + StrLen(reason)        // Reason String
        + formatBuffer.Size()   // Format
        + contextSize           // Context
        + 1 + 3;                // Null Terminator


    if (context)
    {
        PrintError(stringSize, "%s \"%s\" %s%s%s", errorMessage.CStr(), argumentName, reason, contextMsg.CStr(), context);
    }
    else
    {
        PrintError(stringSize, "%s \"%s\" %s", errorMessage.CStr(), argumentName, reason);
    }
}

InvalidTypeArgumentErrorType::InvalidTypeArgumentErrorType(const char* argument, const Type* expected, const Type* got)
: StandardError()
{
    CriticalAssert(expected != nullptr);
    CriticalAssert(got != nullptr);
    // Invalid argument "argument name". Expected type "expected" but got "type"
    // 
    static const auto msgA = StaticString("Invalid argument ");
    static const auto msgB = StaticString(". Expected type ");
    static const auto msgC = StaticString(" but got ");
    const char* expectedName = expected->GetFullName().CStr();
    const char* gotName = got->GetFullName().CStr();
    const SizeT stringSize =
        msgA.Size() + msgB.Size() + msgC.Size()                         // Error Message
        + StrLen(argument)                                              // Argument name
        + expected->GetFullName().Size() + got->GetFullName().Size()    // Argument types
        + 6                                                             // Quotes
        + 1;                                                            // Null terminator
    PrintError(stringSize, "%s\"%s\"%s\"%s\"%s\"%s\"",
        // "Invalid argument <argument name>."
        msgA.CStr(),
        argument,
        // "Expected type <expected> " 
        msgB.CStr(),
        expectedName,
        // "but got <type>"
        msgC.CStr(),
        gotName);
}

OperationFailureErrorType::OperationFailureErrorType(const char* message, const char* context)
: StandardError()
{
    static const auto errorMessage = StaticString("Operation failed! Reason=");
    static const auto contextMsg = StaticString(" Context=");
    const SizeT contextSize = context ? contextMsg.Size() + StrLen(context) : 0;
    const SizeT stringSize =
        errorMessage.Size()
        + StrLen(message)
        + contextSize
        + 1;

    if (context)
    {
        PrintError(stringSize, "%s%s%s%s", errorMessage.CStr(), message, contextMsg.CStr(), context);
    }
    else
    {
        PrintError(stringSize, "%s%s", errorMessage.CStr(), message);
    }
}

OperationFailureAbstractTypeErrorType::OperationFailureAbstractTypeErrorType(const Type* type)
: StandardError()
{
    CriticalAssert(type != nullptr);
    static const auto errorMessage = StaticString("Operation failed to create instance of type because it was abstract. Type=");
    const SizeT stringSize =
        errorMessage.Size()
        + StrLen(type->GetFullName().CStr())
        + 1;
    PrintError(stringSize, "%s%s", errorMessage.CStr(), type->GetFullName().CStr());
}

}
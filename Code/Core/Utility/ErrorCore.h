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
#ifndef LF_CORE_ERROR_CORE_H
#define LF_CORE_ERROR_CORE_H

#include "Core/Common/Types.h"
#include "Core/Common/Assert.h"

namespace lf {

// The error is very-low level and hard to describe, a developer can read the
// source code/comments that might describe the issue and how to fix it.
const ErrorCode LF_ERROR_INTERNAL = 1;
// User code is attempting an operation but the internal state will not allow it
const ErrorCode LF_ERROR_INVALID_OPERATION = 2;
// Internal code expected an object to be a certain way but the data may be malformed
const ErrorCode LF_ERROR_BAD_STATE = 3;
// User code is attempting to access data that is outside the range of a collection
const ErrorCode LF_ERROR_OUT_OF_RANGE = 4;
// User code is attempting to allocate a resource but there is no more space left
const ErrorCode LF_ERROR_OUT_OF_MEMORY = 5;
// User code executed an operation but gave it invalid arguments
const ErrorCode LF_ERROR_INVALID_ARGUMENT = 6;
// A deadlock has occured between multiple threads
const ErrorCode LF_ERROR_DEADLOCK = 7;
// A resource was expected to be released but was detected to still be 'alive'
const ErrorCode LF_ERROR_RESOURCE_LEAK = 8;
// For code paths in development, used to indicate an implementation needs to be done.
const ErrorCode LF_ERROR_MISSING_IMPLEMENTATION = 9;


} // namespace lf


#endif // LF_CORE_ERROR_CORE_H
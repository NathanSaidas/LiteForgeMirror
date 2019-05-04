// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
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
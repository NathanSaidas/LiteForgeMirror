// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "Async.h"
#include "Runtime/Common/RuntimeGlobals.h"

namespace lf {

Async& GetAsync() { AssertError(gAsync, LF_ERROR_INVALID_OPERATION, ERROR_API_RUNTIME); return *gAsync; }

}
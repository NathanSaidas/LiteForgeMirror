// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_TIME_H
#define LF_CORE_TIME_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"

namespace lf {

LF_CORE_API Int64 GetClockFrequency();
LF_CORE_API Int64 GetClockTime();

}

#endif // LF_CORE_TIME_H
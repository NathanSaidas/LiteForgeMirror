// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "ReflectionHooks.h"

namespace lf {
namespace InternalHooks {

static const Type* DefaultHookFindType(const Token& ) { return nullptr; }

FindTypeCallback gFindType = DefaultHookFindType;

} // namespace InternalHooks
} // namespace lf
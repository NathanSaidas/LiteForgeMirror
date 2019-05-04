// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_REFLECTION_HOOKS_H
#define LF_CORE_REFLECTION_HOOKS_H

#include "Core/Common/API.h"

namespace lf {

class Type;
class Token;

namespace InternalHooks {

using FindTypeCallback = const Type*(*)(const Token&);

LF_CORE_API extern FindTypeCallback gFindType;

} // namespace InternalHooks
} // namespace lf

#endif // LF_CORE_REFLECTION_HOOKS_H
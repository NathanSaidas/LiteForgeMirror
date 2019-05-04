// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_ACCESS_SPECIFIER_H
#define LF_CORE_ACCESS_SPECIFIER_H

namespace lf {

enum class AccessSpecifier
{
    AS_PUBLIC,
    AS_PROTECTED,
    AS_PRIVATE,
    MAX_VALUE,
    INVALID_ENUM = MAX_VALUE
};

} // namespace lf

#endif // LF_CORE_ACCESS_SPECIFIER_H
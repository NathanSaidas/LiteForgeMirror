// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_CORE_GUID_H
#define LF_CORE_GUID_H

#include "Core/String/StringUtil.h"

namespace lf {

    class String;

    // Helpful conversion from Byte to HexString and back.
    LF_CORE_API bool ToGuid(const String& id, ByteT* data, const SizeT dataSize);
    LF_CORE_API String ToString(const ByteT* data, const SizeT dataSize);
} // namespace util

#endif // LF_CORE_GUID_H
// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#ifndef LF_RUNTIME_ASSET_COMMON_H
#define LF_RUNTIME_ASSET_COMMON_H

#include "Core/Common/API.h"
#include "Core/String/String.h"

namespace lf {

class Type;

namespace AssetUtil {

LF_RUNTIME_API String GetConcreteAssetName(const Type* type);
LF_RUNTIME_API String GetConcreteAssetName(const String& fullname);

}


} // namespace lf

#endif // LF_RUNTIME_ASSET_COMMON_H
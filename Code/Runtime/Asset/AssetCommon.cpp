// ********************************************************************
// Copyright (c) Nathan Hanlan, All rights reserved
// ********************************************************************
#include "AssetCommon.h"
#include "Core/Reflection/Type.h"

namespace lf {

String AssetUtil::GetConcreteAssetName(const Type* type)
{
    String name(type->GetFullName().CStr());
    name.Replace("::", "/");
    name = "Engine/Types/" + name;
    return name;
}

String AssetUtil::GetConcreteAssetName(const String& fullname)
{
    String name(fullname);
    name.Replace("::", "/");
    name = "Engine/Types/" + name;
    return name;
}


}
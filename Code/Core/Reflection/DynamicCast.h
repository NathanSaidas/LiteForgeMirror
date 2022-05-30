// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#pragma once
#include "Core/Common/Types.h"
#include "Core/Memory/Memory.h"

namespace lf {


// Type trait to verify types are dynamic castable!
// Asset types are not!
template<typename T>
struct DynamicCastableTrait
{
    static const bool value = true;
};

template<typename Dest, typename Src>
const Dest& DynamicCast(const Src& src)
{
    // If you got this error your types are not related.
    LF_STATIC_IS_A(typename Dest::ValueType, typename Src::ValueType);
    // If you got this error you're likely trying to cast an AssetReference / AssetTypeReference
    // use DynamicCastAsset instead.
    LF_STATIC_ASSERT(DynamicCastableTrait<Dest>::value == true, "Type is not dynamic castable.");
    LF_STATIC_ASSERT(DynamicCastableTrait<Src>::value == true, "Type is not dynamic castable.");
    if (src && src->IsA(*Dest::ValueType::GetClassType()))
    {
        return *reinterpret_cast<const Dest*>(&src);
    }
    return *reinterpret_cast<const Dest*>(&NULL_PTR);
}

template<typename Dest, typename Src>
Dest* DynamicCast(Src* source)
{
    // If you got this error your types are not related.
    LF_STATIC_IS_A(Dest, Src);
    // If you got this error you're likely trying to cast an AssetReference / AssetTypeReference
    // use DynamicCastAsset instead.
    LF_STATIC_ASSERT(DynamicCastableTrait<Dest>::value == true, "Type is not dynamic castable.");
    LF_STATIC_ASSERT(DynamicCastableTrait<Src>::value == true, "Type is not dynamic castable.");
    if (source && source->IsA(*Dest::GetClassType()))
    {
        return reinterpret_cast<Dest*>(source);
    }
    return nullptr;
}

template<typename Dest, typename Src>
const Dest* DynamicCast(const Src* source)
{
    // If you got this error your types are not related.
    LF_STATIC_IS_A(Dest, Src);
    // If you got this error you're likely trying to cast an AssetReference / AssetTypeReference
    // use DynamicCastAsset instead.
    LF_STATIC_ASSERT(DynamicCastableTrait<Dest>::value == true, "Type is not dynamic castable.");
    LF_STATIC_ASSERT(DynamicCastableTrait<Src>::value == true, "Type is not dynamic castable.");
    if (source && source->IsA(*Dest::GetClassType()))
    {
        return reinterpret_cast<const Dest*>(source);
    }
    return nullptr;
}

}

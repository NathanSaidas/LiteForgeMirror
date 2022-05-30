// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#ifndef LF_RUNTIME_ASSET_OBJECT_H
#define LF_RUNTIME_ASSET_OBJECT_H

#include "Core/Reflection/Object.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Runtime/Asset/AssetTypeInfo.h"
#include "Runtime/Reflection/ReflectionTypes.h"

namespace lf {

DECLARE_MANAGED_CPTR(AssetTypeInfo);
class AssetPath;

class LF_RUNTIME_API AssetObject : public Object, public TAtomicWeakPointerConvertible<AssetObject>
{
    DECLARE_CLASS(AssetObject, Object);
public:
    using PointerConvertible = PointerConvertibleType;
    AssetObject();
    virtual ~AssetObject();

    void SetAssetType(const AssetTypeInfo* type) { mAssetType = AssetTypeInfoCPtr(type); }
    const AssetTypeInfoCPtr& GetAssetType() const { return mAssetType; }

    const AssetPath& GetAssetPath() const;
private:
    AssetTypeInfoCPtr mAssetType;
};

}

#endif // LF_RUNTIME_ASSET_OBJECT_H
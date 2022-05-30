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
#include "AbstractEngine/PCH.h"
#include "GfxMaterialProperty.h"

namespace lf {
namespace Gfx {

MaterialPropertyContainer::MaterialPropertyContainer()
: mProperties()
, mTextureProperties()
, mTextureAssetProperties()
, mPropertyBufferSize(0)
{}

void MaterialPropertyContainer::AddProperty(const Token& name, UInt8 type, UInt8 size, UInt16 offset)
{
    mProperties.push_back({name, type, size, offset });
    mPropertyBufferSize += static_cast<SizeT>(size);
}
void MaterialPropertyContainer::AddTexture(const Token& name, UInt8 type, UInt16 tileIndex, UInt32 index, const TStrongPointer<GfxResourceObject>& texture)
{
    mTextureProperties.push_back(MaterialTextureProperty());
    auto& property = mTextureProperties.back();
    property.mName = name;
    property.mType = type;
    property.mSize = 0; // unused
    property.mOffset = tileIndex;
    property.mIndex = index;
    property.mTexture = texture;
}
void MaterialPropertyContainer::AddTextureAsset(const Token& name, UInt8 type, UInt16 tileIndex, UInt32 index, const TAssetType<GfxResourceObject>& texture)
{
    mTextureAssetProperties.push_back(MaterialTextureAssetProperty());
    auto& property = mTextureAssetProperties.back();
    property.mName = name;
    property.mType = type;
    property.mSize = 0; // unused
    property.mOffset = tileIndex;
    property.mIndex = index;
    property.mTexture = texture;
}
void MaterialPropertyContainer::Clear()
{
    mProperties.clear();
    mTextureProperties.clear();
    mTextureAssetProperties.clear();
}

MaterialPropertyId MaterialPropertyContainer::FindPropertyID(const Token& name) const
{
    auto propsIter = std::find_if(mProperties.begin(), mProperties.end(), [&name](const MaterialProperty& prop) { return prop.mName == name; });
    if (propsIter != mProperties.end())
    {
        return ToID(propsIter - mProperties.begin(), IndexType::Property);
    }

    auto texturePtrIter = std::find_if(mTextureProperties.begin(), mTextureProperties.end(), [&name](const MaterialTextureProperty& prop) { return prop.mName == name; });
    if (texturePtrIter != mTextureProperties.end())
    {
        return ToID(texturePtrIter - mTextureProperties.begin(), IndexType::Texture);
    }

    auto textureAssetIter = std::find_if(mTextureAssetProperties.begin(), mTextureAssetProperties.end(), [&name](const MaterialTextureAssetProperty& prop) { return prop.mName == name; });
    if(textureAssetIter != mTextureAssetProperties.end())
    {
        return ToID(textureAssetIter - mTextureAssetProperties.begin(), IndexType::TextureAsset);
    }
    return INVALID_MATERIAL_PROPERTY_ID;
}
TUnsafePtr<MaterialProperty> MaterialPropertyContainer::FindProperty(const Token& name)
{
    return FindProperty(FindPropertyID(name));
}
TUnsafePtr<const MaterialProperty> MaterialPropertyContainer::FindProperty(const Token& name) const
{
    return FindProperty(FindPropertyID(name));
}
TUnsafePtr<MaterialProperty> MaterialPropertyContainer::FindProperty(MaterialPropertyId id)
{
    UInt32 indexType;
    SizeT localIndex;
    FromID(id, localIndex, indexType);

    switch (indexType)
    {
    case IndexType::Property: return localIndex < mProperties.size() ? &mProperties[localIndex] : nullptr;
    case IndexType::Texture: return localIndex < mTextureProperties.size() ? &mTextureProperties[localIndex] : nullptr;
    case IndexType::TextureAsset: return localIndex < mTextureAssetProperties.size() ? &mTextureAssetProperties[localIndex] : nullptr;
    default: return nullptr;
    }
}
TUnsafePtr<const MaterialProperty> MaterialPropertyContainer::FindProperty(MaterialPropertyId id) const
{
    UInt32 indexType;
    SizeT localIndex;
    FromID(id, localIndex, indexType);

    switch (indexType)
    {
    case IndexType::Property: return localIndex < mProperties.size() ? &mProperties[localIndex] : nullptr;
    case IndexType::Texture: return localIndex < mTextureProperties.size() ? &mTextureProperties[localIndex] : nullptr;
    case IndexType::TextureAsset: return localIndex < mTextureAssetProperties.size() ? &mTextureAssetProperties[localIndex] : nullptr;
    default: return nullptr;
    }
}

MaterialPropertyId MaterialPropertyContainer::ToID(SizeT localIndex, UInt32 type)
{
    UInt32 localIndex32 = static_cast<UInt32>(localIndex) & 0xFFFF;
    return MaterialPropertyId((localIndex32 << 16) | (type & 0xFFFF));
}

void MaterialPropertyContainer::FromID(MaterialPropertyId index, SizeT& outLocalIndex, UInt32& outType)
{
    outType = index & 0xFFFF;
    outLocalIndex = (index >> 16) & 0xFFFF;
}

} // namespace Gfx
} // namespace lf
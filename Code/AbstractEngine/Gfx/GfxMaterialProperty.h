#pragma once
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
#include "Core/String/Token.h"
#include "Core/Utility/StdVector.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxBase.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"

namespace lf {

namespace Gfx {

struct MaterialTextureProperty : public MaterialProperty
{
    UInt32                     mIndex; // Texture Index
    TStrongPointer<GfxResourceObject>  mTexture; // TStrongPointer<Texture>
};

struct MaterialTextureAssetProperty : public MaterialProperty
{
    UInt32                  mIndex; // Texture Index
    TAssetType<GfxResourceObject>   mTexture; // NOTE: When manipulating the texture we should lock this resource as 'Gfx'
};

// A container to mana property access
class LF_ABSTRACT_ENGINE_API MaterialPropertyContainer
{
public:
    MaterialPropertyContainer();
    ~MaterialPropertyContainer() = default;

    // ********************************************************************
    // Adds a property to the container, the design purpose of this method
    // is to initialize a container at a single point in time and then 
    // use it to query property info/memory address.
    // 
    // @param name -- The name of the property in the SHADER_PROPERTIES cbuffer
    // @param type -- The type of the property.
    // @param size -- The size of the property in bytes.
    // @param offset -- The offset of the property in memory.
    // ********************************************************************
    void AddProperty(const Token& name, UInt8 type, UInt8 size, UInt16 offset);
    // ********************************************************************
    // Adds a property to the container, the design purpose of this method
    // is to initialize a container at a single point in time and then 
    // use it to query property infoassigned texture pointer.
    // 
    // @param name -- The name of the property in the SHADER_PROPERTIES cbuffer
    // @param type -- The type of the property.
    // @param tileIndex -- A tile index used for font rendering, non-font 
    //                     textures should use 0.
    // @param index -- Register index
    // @param texture -- A pointer to a texture
    // ********************************************************************
    void AddTexture(const Token& name, UInt8 type, UInt16 tileIndex, UInt32 index, const TStrongPointer<GfxResourceObject>& texture);
    // ********************************************************************
    // Adds a property to the container, the design purpose of this method
    // is to initialize a container at a single point in time and then 
    // use it to query property info/assign texture reference.
    // 
    // @param name -- The name of the property in the SHADER_PROPERTIES cbuffer
    // @param type -- The type of the property.
    // @param tileIndex -- A tile index used for font rendering, non-font 
    //                     textures should use 0.
    // @param index -- Register index
    // @param texture -- A pointer to a texture 
    // ********************************************************************
    void AddTextureAsset(const Token& name, UInt8 type, UInt16 tileIndex, UInt32 index, const TAssetType<GfxResourceObject>& texture);
    // ********************************************************************
    // Clear out all property info.
    // ********************************************************************
    void Clear();

    // ********************************************************************
    // Finds a property ID for a name.
    // ********************************************************************
    MaterialPropertyId FindPropertyID(const Token& name) const;
    // ********************************************************************
    // Finds a property by name.
    // ********************************************************************
    TUnsafePtr<MaterialProperty> FindProperty(const Token& name);
    // ********************************************************************
    // Finds a property by name.
    // ********************************************************************
    TUnsafePtr<const MaterialProperty> FindProperty(const Token& name) const;
    // ********************************************************************
    // Finds a property by id.
    // ********************************************************************
    TUnsafePtr<MaterialProperty> FindProperty(MaterialPropertyId id);
    // ********************************************************************
    // Finds a property by 
    // ********************************************************************
    TUnsafePtr<const MaterialProperty> FindProperty(MaterialPropertyId id) const;
    // ********************************************************************
    // Returns the size of the property buffer (Excluding Textures)
    // ********************************************************************
    SizeT GetPropertyBufferSize() const { return mPropertyBufferSize; }

    // If we go to set a texture and 
    // a) CurrentTexture == TAssetType && Texture == TStrongPointer
    //      then insert new texture in StrongPointer array with same properties
    //      then remove current texture from Asset array
    //      then invalidate property ids (textures only)
    //
    // b) CurrentTexture == TStrongPointer
    //      then insert new texture in Asset array with same properties
    //      then remove current texture from StrongPointer array
    //      then invalidate property ids (textures only)
    //
    // SetTexture(MaterialPropertyId texture, TStrongPointer<Texture>)
    // SetTexture(MaterialPropertyId texture, TAssetType<Texture>)

    
    // void AllocateConstantBufferCPU(...)
    // void AllocateConstantBufferGPU(...)
    // void WriteCPUToGPU()
    // void WriteGPUToCPU()

private:
    enum IndexType : UInt16
    {
        Property,
        Texture,
        TextureAsset
    };

    static MaterialPropertyId ToID(SizeT localIndex, UInt32 type);
    static void FromID(MaterialPropertyId id, SizeT& outLocalIndex, UInt32& outType);

    // How indexing should work:
    // Indicies are 'versioned' meaning that if a shader recompiles and you're 
    // reading/writing properties by index (which you should be doing) then
    // your index will be invalidated on revision!
    // 
    // Index = (Index << 16 | Version)
    TVector<MaterialProperty>             mProperties;
    // Index = ((Index - mProperties.size()) | Version)
    TVector<MaterialTextureProperty>      mTextureProperties;
    // Index = ((Index - (mProperties.size() + mTextureProperties.size())) | Version)
    TVector<MaterialTextureAssetProperty> mTextureAssetProperties;

    SizeT mPropertyBufferSize;
};

} // namespace Gfx

}

// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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

#include "Runtime/Asset/AssetProcessor.h"
#include "AbstractEngine/Gfx/GfxTextureBinary.h"

namespace lf {

class LF_ABSTRACT_ENGINE_API GfxTextureBinaryProcessor : public AssetProcessor
{
public:
    using Super = AssetProcessor;

    GfxTextureBinaryProcessor(Gfx::TextureFileFormat targetFormat);

    const Type* GetTargetType() const override;
    SizeT GetCacheBlockScore(CacheBlockType::Value cacheBlock) const override;

    bool AcceptImportPath(const AssetPath& path) const override;
    const Type* GetPrototypeType(const Type* inputType) const override;
    const Type* GetConcreteType(const Type* inputType) const override;
    AssetImportResult Import(const AssetPath& assetPath) const override;
    AssetDataType::Value Export(AssetObject* object, MemoryBuffer& buffer, bool cache, AssetDataType::Value dataTypeHint = AssetDataType::INVALID_ENUM) const override;
    
    void OnCreatePrototype(AssetObject* object) const override;
    void OnDestroyPrototype(AssetObject* object) const override;
    bool PrepareAsset(AssetObject* object, const MemoryBuffer& buffer, AssetLoadFlags::Value loadFlags) const override;
    void OnLoadAsset(AssetObject* object) const override;
    void OnUnloadAsset(AssetObject* object) const override;

private:
    Gfx::TextureFileFormat mTargetFormat;
};

} // namespace lf

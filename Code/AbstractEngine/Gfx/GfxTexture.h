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
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"
#include "AbstractEngine/Gfx/GfxTypes.h"

namespace lf {
DECLARE_ASSET(GfxTextureBinary);
// Usage:
// 
// GfxTextureAtomicPtr texture = ...;
// GfxTextureBinary binary;
//    
// texture->SetBinary(binary);
// texture->SetResourceFormat(format);
// texture->Commit();
class LF_ABSTRACT_ENGINE_API GfxTexture : public GfxResourceObject
{
    DECLARE_CLASS(GfxTexture, GfxResourceObject);
public:
    virtual void SetBinary(const GfxTextureBinaryAsset& binary) = 0;
    virtual void SetResourceFormat(Gfx::ResourceFormat format) = 0;


};

} // namespace lf
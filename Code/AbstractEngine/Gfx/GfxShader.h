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
#include "AbstractEngine/Gfx/GfxBase.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxShaderText.h"
#include "Runtime/Asset/AssetReferenceTypes.h"

namespace lf {
DECLARE_ASSET(GfxShaderText);

class LF_ABSTRACT_ENGINE_API GfxShader : public AssetObject
{
    DECLARE_CLASS(GfxShader, AssetObject);
public:

    void Serialize(Stream& s) override;

    bool SupportsApi(Gfx::GraphicsApi api) const
    {
        CriticalAssert(ValidEnum(api));
        return mText[EnumValue(api)] != NULL_PTR;
    }

    void SetText(Gfx::GraphicsApi api, const GfxShaderTextAsset& value)
    {
        CriticalAssert(ValidEnum(api));
        mText[EnumValue(api)] = value;
    }

    const GfxShaderTextAsset& GetText(Gfx::GraphicsApi api) const
    {
        CriticalAssert(ValidEnum(api));
        return mText[EnumValue(api)];
    }
private:
    GfxShaderTextAsset mText[ENUM_SIZE(Gfx::GraphicsApi)];
};

} // namespace lf
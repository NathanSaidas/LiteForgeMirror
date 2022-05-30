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

#include "AbstractEngine/PCH.h"
#include "GfxShaderTextProcessor.h"
#include "Core/Reflection/DynamicCast.h"
#include "AbstractEngine/Gfx/GfxShaderText.h"

namespace lf {
    
    GfxShaderTextProcessor::GfxShaderTextProcessor() : Super()
    {}
    GfxShaderTextProcessor::~GfxShaderTextProcessor()
    {}


    const Type* GfxShaderTextProcessor::GetTargetType() const
    {
        return typeof(GfxShaderText);
    }
    bool GfxShaderTextProcessor::AcceptImportPath(const AssetPath& path) const
    {
        const String extension = path.GetExtension();
        return extension == "hlsl" || extension == "shader" || extension == "glsl";
    }

    void GfxShaderTextProcessor::ReadText(AssetObject* object, const String& text) const
    {
        GfxShaderText* asset = DynamicCast<GfxShaderText>(object);
        if (asset != nullptr)
        {
            asset->SetText(text);
        }
    }
    void GfxShaderTextProcessor::WriteText(AssetObject* object, String& text) const
    {
        GfxShaderText* asset = DynamicCast<GfxShaderText>(object);
        if (asset != nullptr)
        {
            text = asset->GetText();
        }
    }

}
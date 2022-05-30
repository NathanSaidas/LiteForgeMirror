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
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "Runtime/Asset/AssetPath.h"
#include "Core/Common/API.h"
#include "Core/Common/Types.h"

namespace lf {
DECLARE_ASSET(GfxShader);
class GfxShaderBinaryBundle;

class LF_ABSTRACT_ENGINE_API GfxShaderManager
{
public:
    virtual ~GfxShaderManager();

    using HashKey = Gfx::ShaderHash;
    using Key = Token;

    // ComputeHashKey = Gfx::ComputeHash(...)
    // ComputeKey = Token(Gfx::ComputePath(...)) 

    // When a GfxMaterial is writing to the cache, we should create the necessary shader assets.
    // (Check current cache data for new 'shaderHash') use Gfx::ComputeShaderHash to determine if recompile is necessary.
    virtual GfxShaderBinaryBundle CreateShaderAssets(
        const AssetTypeInfoCPtr& creatorType
        , const GfxShaderAsset& shader
        , const TVector<Token>& defines
        , Gfx::ShaderType shaderType) = 0;

    // When a GfxMaterial is writing to the cache, we should delete old shader assets (if shader hash is not a match)
    // When a GfxMaterial is deleted we should delete the old shader assets.
    virtual void DestroyShaderAssets(const AssetTypeInfoCPtr& creatorType, const GfxShaderBinaryBundle& bundle) = 0;

    

};

} // namespace lf
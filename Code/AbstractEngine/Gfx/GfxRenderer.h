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
#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxTypes.h"

namespace lf {
class GfxDependencyContext;
class GfxDevice;
class GfxCommandContext;
class MemoryBuffer;
DECLARE_ATOMIC_PTR(GfxModelRenderer);
DECLARE_ATOMIC_PTR(GfxTexture);
DECLARE_ATOMIC_PTR(GfxPipelineState);
DECLARE_ASSET(GfxTextureBinary);

class LF_ENGINE_API DebugAssetProvider
{
public:
    virtual ~DebugAssetProvider() {}
    virtual String GetShaderText(const String& assetName) = 0;
    virtual bool GetShaderBinary(Gfx::ShaderType shaderType, const String& text, const TVector<Token>& defines, MemoryBuffer& outputBuffer) = 0;
    virtual GfxTextureBinaryAsset GetTexture(const String& assetName) = 0;
};
DECLARE_PTR(DebugAssetProvider);

class LF_ABSTRACT_ENGINE_API GfxRenderer : public Object, public TAtomicWeakPointerConvertible<GfxRenderer>
{
    DECLARE_CLASS(GfxRenderer, Object);
public:
    enum DebugShaderType
    {
        SIMPLE_MESH,
        TEXTURE_MESH,
        STANDARD_MESH,

        DebugShaderType_MAX_VALUE
    };
    enum DebugTextureType
    {
        RED,
        GREEN,
        PURPLE,

        DebugTextureType_MAX_VALUE
    };

    using PointerConvertible = PointerConvertibleType;

    virtual bool Initialize(GfxDependencyContext& context) = 0;
    virtual void Shutdown() = 0;
    virtual GfxModelRendererAtomicPtr CreateModelRendererOfType(const Type* type) = 0;
    virtual GfxDevice& Device() = 0;

    template<typename T>
    TAtomicStrongPointer<T> CreateModelRenderer()
    {
        LF_STATIC_IS_A(T, GfxModelRenderer);
        return StaticCast<TAtomicStrongPointer<T>>(CreateModelRendererOfType(typeof(T)));
    }

    virtual void SetupResource(GfxDevice& device, GfxCommandContext& context);
    virtual void SetupFrame();

    virtual void OnBeginFrame();
    virtual void OnEndFrame();
    virtual void OnUpdate();

    virtual void OnRender(GfxDevice& device, GfxCommandContext& context); // Render Frame

    virtual GfxPipelineStateAtomicPtr GetDebugShader(DebugShaderType type);
    virtual GfxTextureAtomicPtr GetDebugTexture(DebugTextureType);

};


} // namespace lf
// ********************************************************************
// Copyright (c) 2022 Nathan Hanlan
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

#include "Core/Concurrent/Task.h"
#include "Core/Math/Vector4.h"
#include "Core/Math/Vector2.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxModelRenderer.h"
#include "AbstractEngine/Gfx/GfxVertexBuffer.h"
#include "AbstractEngine/Gfx/GfxIndexBuffer.h"
#include "AbstractEngine/Gfx/GfxPipelineState.h"

namespace lf
{

DECLARE_ATOMIC_PTR(GfxVertexBuffer);
DECLARE_ATOMIC_PTR(GfxIndexBuffer);
DECLARE_ATOMIC_PTR(GfxPipelineState);
DECLARE_ATOMIC_PTR(GfxTexture);
DECLARE_ASSET(GfxTextureBinary);

class LF_ENGINE_API ProceduralModelRenderer : public GfxModelRenderer
{
    DECLARE_CLASS(ProceduralModelRenderer, GfxModelRenderer);
public:
    struct Vertex
    {
        Vector4 mPosition;
    };
    struct VertexUV
    {
        Vector4 mPosition;
        Vector2 mTexCoord;
    };

    ProceduralModelRenderer();
    ~ProceduralModelRenderer() override;
    void SetupResource(GfxDevice& device, GfxCommandContext& context) override;
    void OnRender(GfxDevice& device, GfxCommandContext& context) override;

    // **********************************
    // Checks if the ModelRenderer has allocated it's resources and is accepting data.
    // 
    // NOTE: When a ModelRenderer is first created it will attempt to allocate the resources necessary. This step is deferred though until after the renderer updates.
    // So setting data without it being allocated does nothing.
    // **********************************
    bool IsAllocated() const;
    bool IsGPUReady() const;

    void SetData(const TVector<Vertex>& vertices, const TVector<UInt16>& indices, const MemoryBuffer& vertexShaderByteCode, const MemoryBuffer& pixelShaderByteCode);

    void SetTexture(const GfxTextureBinaryAsset& textureBinary);
private:
    enum class DirtyFlags : Int32
    {
        None                = 0,
        VertexBuffer        = 1 << 0,
        IndexBuffer         = 1 << 1,
        PipelineState       = 1 << 2,
        Texture             = 1 << 3
    };
    ENUM_BITWISE_OPERATORS_NESTED(DirtyFlags);

    void SetDirty(DirtyFlags flags)
    {
        mDirtyFlags |= flags;
    }

    void ClearDirty()
    {
        mDirtyFlags = DirtyFlags::None;
    }

    bool IsDirty(DirtyFlags flag)
    {
        return (mDirtyFlags & flag) != DirtyFlags::None;
    }
    SpinLock                  mLock;
    // flags
    DirtyFlags                mDirtyFlags;

    // resource
    GfxVertexBufferAtomicPtr  mVertexBuffer;
    GfxIndexBufferAtomicPtr   mIndexBuffer;
    GfxPipelineStateAtomicPtr mPSO;
    GfxTextureAtomicPtr       mTexture;
};

}

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
#include "Core/Utility/APIResult.h"
#include "Core/Math/Viewport.h"
#include "Core/Math/Rect.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"

namespace lf {
class GfxDependencyContext;
class GfxVertexBuffer;
class GfxIndexBuffer;
class GfxTexture;
class GfxRenderTexture;
class GfxSwapChain;
class GfxPipelineState;
DECLARE_ATOMIC_PTR(GfxUploadBuffer);

class LF_ABSTRACT_ENGINE_API GfxCommandContext : public GfxResourceObject
{
    DECLARE_CLASS(GfxCommandContext, GfxResourceObject);
public:

    virtual void BeginRecord(Gfx::FrameCountType currentFrame) = 0;
    virtual void EndRecord() = 0;
    virtual void SetRenderTarget(GfxSwapChain* target, SizeT frame) = 0;
    virtual void BindRenderTarget(GfxRenderTexture* target) = 0;
    virtual void UnbindRenderTarget(GfxRenderTexture* target) = 0;
    virtual void SetPresentSwapChainState(GfxSwapChain* target, SizeT frame) = 0;

    virtual void SetPipelineState(const GfxPipelineState* state) = 0;
    virtual void CopyDataImpl(GfxUploadBufferAtomicPtr& buffer, Gfx::UploadBufferType uploadBufferType, const ByteT* data, SizeT dataByteCount) = 0 ;
    virtual void SetViewport(const ViewportF& viewport) = 0;
    virtual void SetScissorRect(const RectI& rect) = 0;
    virtual void ClearColor(GfxSwapChain* target, SizeT frame, const Color& color) = 0;
    virtual void ClearDepth(float value) = 0;
    virtual void ClearColor(const GfxRenderTexture* texture, const Color& color) = 0;
    virtual void SetTexture(Gfx::ShaderParamID index, const GfxTexture* texture) = 0;
    virtual void SetConstantBuffer(Gfx::ShaderParamID index, const GfxUploadBuffer* constantBuffer) = 0;
    virtual void SetStructureBuffer(Gfx::ShaderParamID index, const GfxUploadBuffer* structureBuffer) = 0;
    virtual void SetVertexBuffer(const GfxVertexBuffer* vertexBuffer) = 0;
    virtual void SetVertexBuffers(SizeT startIndex, SizeT numBuffers, const GfxVertexBuffer** vertexBuffers) = 0;
    virtual void SetIndexBuffer(const GfxIndexBuffer* indexBuffer) = 0;
    virtual void SetTopology(Gfx::RenderMode topology) = 0;


    virtual void Draw(SizeT vertexCount, SizeT vertexOffset = 0) = 0;
    virtual void DrawIndexed(SizeT indexCount, SizeT indexOffset = 0, SizeT vertexOffset = 0) = 0;

    template<typename T>
    void CopyConstantData(GfxUploadBufferAtomicPtr& buffer, const T& dataStruct)
    {
        CopyDataImpl(buffer, Gfx::UploadBufferType::Constant, reinterpret_cast<const ByteT*>(&dataStruct), sizeof(T));
    }

    template<typename T>
    void CopyStructureData(GfxUploadBufferAtomicPtr& buffer, const T& dataStruct)
    {
        CopyDataImpl(buffer, Gfx::UploadBufferType::Structured, reinterpret_cast<const ByteT*>(&dataStruct), sizeof(T));
    }
};

} // namespace lf
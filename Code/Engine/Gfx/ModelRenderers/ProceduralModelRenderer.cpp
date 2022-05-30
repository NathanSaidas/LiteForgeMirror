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

#include "Engine/PCH.h"
#include "ProceduralModelRenderer.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxCommandContext.h"
#include "AbstractEngine/Gfx/GfxRenderer.h"
#include "AbstractEngine/Gfx/GfxTexture.h"
#include "Core/Utility/Log.h"

namespace lf {

DEFINE_CLASS(lf::ProceduralModelRenderer) { NO_REFLECTION; }

ProceduralModelRenderer::ProceduralModelRenderer()
: Super()
, mLock()
, mDirtyFlags(DirtyFlags::None)
, mVertexBuffer()
, mIndexBuffer()
, mPSO()
, mTexture()
{

}
ProceduralModelRenderer::~ProceduralModelRenderer()
{}

void ProceduralModelRenderer::SetupResource(GfxDevice& device, GfxCommandContext& context)
{
    ScopeLock lock(mLock);
    if (IsDirty(DirtyFlags::VertexBuffer) && mVertexBuffer)
    {
        mVertexBuffer->Commit(device, context);
    }
    if (IsDirty(DirtyFlags::IndexBuffer) && mIndexBuffer)
    {
        mIndexBuffer->Commit(device, context);
    }
    if (IsDirty(DirtyFlags::PipelineState) && mPSO)
    {
        mPSO->Commit(device, context);
    }
    if (IsDirty(DirtyFlags::Texture) && mTexture)
    {
        mTexture->Commit(device, context);
    }
    ClearDirty();
}

void ProceduralModelRenderer::OnRender(GfxDevice& , GfxCommandContext& context)
{
    if (!IsGPUReady())
    {
        return;
    }

    ScopeLock lock(mLock);
    context.SetVertexBuffer(mVertexBuffer);
    context.SetIndexBuffer(mIndexBuffer);
    context.SetPipelineState(mPSO);
    context.SetTopology(Gfx::RenderMode::TRIANGLES);
    context.DrawIndexed(mIndexBuffer->GetNumElements());
}

bool ProceduralModelRenderer::IsAllocated() const
{
    return mVertexBuffer && mIndexBuffer && mPSO;
}

bool ProceduralModelRenderer::IsGPUReady() const
{
    return IsAllocated() && mVertexBuffer->IsGPUReady() && mIndexBuffer->IsGPUReady() && mPSO->IsGPUReady();
}

void ProceduralModelRenderer::SetData(const TVector<Vertex>& vertices, const TVector<UInt16>& indices, const MemoryBuffer& vertexShaderByteCode, const MemoryBuffer& pixelShaderByteCode)
{
    auto& device = Renderer().Device();
    mVertexBuffer = device.CreateResource<GfxVertexBuffer>();
    mIndexBuffer = device.CreateResource<GfxIndexBuffer>();
    mPSO = device.CreateResource<GfxPipelineState>();

    if (!IsAllocated())
    {
        LF_LOG_WARN(gGfxLog, "Failed to set the data on the ModelRenderer. ModelRenderer has not yet allocated it's data!");
        return;
    }

    mVertexBuffer->SetUsage(Gfx::BufferUsage::STATIC);
    mVertexBuffer->SetVertices(vertices);

    mIndexBuffer->SetUsage(Gfx::BufferUsage::STATIC);
    mIndexBuffer->SetIndices(indices);

    TStackVector<Gfx::VertexInputElement, 8> inputLayout;
    inputLayout.push_back(Gfx::VertexInputElement());
    inputLayout.back().mSemanticIndex = 0;
    inputLayout.back().mSemanticName = Token("SV_POSITION");
    inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
    inputLayout.back().mInputSlot = 0;
    inputLayout.back().mPerVertexData = true;
    inputLayout.back().mInstanceDataStepRate = 0;
    inputLayout.back().mAlignedByteOffset = 0;
    /*inputLayout.push_back(Gfx::VertexInputElement());
    inputLayout.back().mSemanticIndex = 0;
    inputLayout.back().mSemanticName = Token("TEXCOORD");
    inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32_FLOAT;
    inputLayout.back().mInputSlot = 0;
    inputLayout.back().mPerVertexData = true;
    inputLayout.back().mInstanceDataStepRate = 0;
    inputLayout.back().mAlignedByteOffset = 16;*/

    mPSO->SetShaderByteCode(Gfx::ShaderType::VERTEX, vertexShaderByteCode);
    mPSO->SetShaderByteCode(Gfx::ShaderType::PIXEL, pixelShaderByteCode);
    mPSO->SetInputLayout(inputLayout);

    SetDirty(DirtyFlags::VertexBuffer | DirtyFlags::IndexBuffer | DirtyFlags::PipelineState);
}

void ProceduralModelRenderer::SetTexture(const GfxTextureBinaryAsset& textureBinary)
{
    ScopeLock lock(mLock);
    if (!textureBinary)
    {
        mTexture = NULL_PTR;
        return;
    }

    auto& device = Renderer().Device();
    mTexture = device.CreateResource<GfxTexture>();
    if (!mTexture)
    {
        LF_LOG_WARN(gGfxLog, "Failed to set the texture on the ModelRenderer. Texture creation failure");
        return;
    }
    mTexture->SetBinary(textureBinary);
    SetDirty(DirtyFlags::Texture);
}

} // namespace lf
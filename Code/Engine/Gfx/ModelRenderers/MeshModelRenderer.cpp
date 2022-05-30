#include "Engine/PCH.h"
#include "MeshModelRenderer.h"
#include "Core/Utility/Log.h"
#include "AbstractEngine/Gfx/GfxRenderer.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxIndexBuffer.h"
#include "AbstractEngine/Gfx/GfxVertexBuffer.h"
#include "AbstractEngine/Gfx/GfxTexture.h"
#include "AbstractEngine/Gfx/GfxPipelineState.h"
#include "AbstractEngine/Gfx/GfxCommandContext.h"
#include "AbstractEngine/Gfx/GfxUploadBuffer.h"

namespace lf {
DEFINE_CLASS(lf::MeshModelRenderer) { NO_REFLECTION; }

static const char* MeshModelRenderer_VertexType[]
{
    "VT_NONE",
    "VT_SIMPLE",
    "VT_TEXTURE",
    "VT_STANDARD"
};
LF_STATIC_ASSERT(LF_ARRAY_SIZE(MeshModelRenderer_VertexType) == ENUM_SIZE(MeshModelRenderer::VertexType));
static const char* ToString(MeshModelRenderer::VertexType type)
{
    return MeshModelRenderer_VertexType[EnumValue(type)];
}

MeshModelRenderer::MeshModelRenderer()
    : Super()
    , mLock()
    , mDirtyFlags(DF_NONE)
    , mVertexType(VT_NONE)
    , mVertexBuffer()
    , mIndexBuffer()
    , mPSO()
    , mTextures()
{}
MeshModelRenderer::~MeshModelRenderer()
{}

void MeshModelRenderer::SetupResource(GfxDevice& device, GfxCommandContext& context)
{
    ScopeLock lock(mLock);
    PerObject perObject;
    perObject.mWorldPosition = Vector3(0.0f, 1.0f, 1.0f);
    context.CopyConstantData(mConstantBuffer, perObject);
    perObject.mWorldPosition = Vector3(1.0f, 1.0f, 1.0f);
    context.CopyStructureData(mStructureBuffer, perObject);
    if (mDirtyFlags == DF_NONE)
    {
        return;
    }


    auto Commit = [&](bool dirty, GfxResourceObject* resource)
    {
        if (dirty && resource)
        {
            resource->Commit(device, context);
        }
    };

    Commit(IsDirty(DF_PIPELINE_STATE), mPSO);
    Commit(IsDirty(DF_VERTEX_BUFFER), mVertexBuffer);
    Commit(IsDirty(DF_INDEX_BUFFER), mIndexBuffer);
    if (IsDirty(DF_TEXTURE))
    {
        for (SizeT i = 0; i < LF_ARRAY_SIZE(mTextures); ++i)
        {
            if (mTextures[i])
            {
                mTextures[i]->Commit(device, context);
            }
        }
    }
    ClearDirty();
}

void MeshModelRenderer::OnRender(GfxDevice& , GfxCommandContext& context)
{
    ScopeLock lock(mLock);
    const bool gpuReady = mPSO && mPSO->IsGPUReady() && mVertexBuffer && mVertexBuffer->IsGPUReady() && mIndexBuffer && mIndexBuffer->IsGPUReady();
    if (!gpuReady)
    {
        return;
    }
    CriticalAssert(mVertexBuffer->GetNumElements() == mIndexBuffer->GetNumElements());

    context.SetPipelineState(mPSO);

    // TODO: We need to find a MUCH nicer way of binding data, as the MeshModelRenderer is supposed to be ignorant for a lot of this data...
    const bool useTexture = GetVertexType() != VT_SIMPLE;
    if (useTexture)
    {
        if (mTextures[0]) {
            auto paramID = mPSO->FindParam(Token("gTextures0"));
            if (paramID.IsValid()) {
                context.SetTexture(paramID, mTextures[0]);
            }
        }

        if (mTextures[1]) {
            auto paramID = mPSO->FindParam(Token("gTextures1"));
            if (paramID.IsValid()) {
                context.SetTexture(paramID, mTextures[1]);
            }
        }

        if (mTextures[2]) {
            auto paramID = mPSO->FindParam(Token("gTextures2"));
            if (paramID.IsValid()) {
                context.SetTexture(paramID, mTextures[2]);
            }
        }
    }


    {
        auto paramID = mPSO->FindParam(Token("gPerObject"));
        if (paramID.IsValid()) {
            context.SetConstantBuffer(paramID, mConstantBuffer);
        }
    }

    {
        auto paramID = mPSO->FindParam(Token("gPerObjectStruct"));
        if (paramID.IsValid()) {
            context.SetStructureBuffer(paramID, mStructureBuffer);
        }
    }

    context.SetVertexBuffer(mVertexBuffer);
    context.SetIndexBuffer(mIndexBuffer);
    context.SetTopology(Gfx::RenderMode::TRIANGLES);
    context.DrawIndexed(mIndexBuffer->GetNumElements());
}

void MeshModelRenderer::SetPipeline(VertexType vertexType, const MemoryBuffer& vertexByteCode, const MemoryBuffer& pixelByteCode)
{
    const bool validVertexType = EnumValue(vertexType) > EnumValue(VT_NONE) && EnumValue(vertexType) < EnumValue(VertexType::MAX_VALUE);
    if (!validVertexType)
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetPipeline failed, invalid vertex type provided. Value=" << EnumValue(vertexType));
        return;
    }

    if (GetVertexType() != VT_NONE && GetVertexType() != vertexType)
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetPipeline failed, unable to switch vertex type. You must clear the existing data first. Src=" << ToString(GetVertexType()) << ", Dest=" << ToString(vertexType));
        return;
    }


    UInt32 byteOffset = 0;
         
    TStackVector<Gfx::VertexInputElement, 8> inputLayout;
    inputLayout.push_back(Gfx::VertexInputElement());
    inputLayout.back().mSemanticIndex = 0;
    inputLayout.back().mSemanticName = Token("SV_POSITION");
    inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
    inputLayout.back().mInputSlot = 0;
    inputLayout.back().mPerVertexData = true;
    inputLayout.back().mInstanceDataStepRate = 0;
    inputLayout.back().mAlignedByteOffset = byteOffset;
    byteOffset += sizeof(Vector4);


    if (vertexType == VT_TEXTURE || vertexType == VT_STANDARD)
    {
        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("COLOR");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = byteOffset;
        byteOffset += sizeof(Vector4);

        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("TEXCOORD");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = byteOffset;
        byteOffset += sizeof(Vector2);
    }

    if (vertexType == VT_STANDARD)
    {
        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("NORMAL");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = byteOffset;
        byteOffset += sizeof(Vector3);
    }

    mVertexType = vertexType;
    mPSO = Renderer().Device().CreateResource<GfxPipelineState>();
    mPSO->SetShaderByteCode(Gfx::ShaderType::VERTEX, vertexByteCode);
    mPSO->SetShaderByteCode(Gfx::ShaderType::PIXEL, pixelByteCode);
    mPSO->SetInputLayout(inputLayout);
    SetDirty(DF_PIPELINE_STATE);
}

void MeshModelRenderer::SetPipeline(VertexType vertexType, const GfxPipelineStateAtomicPtr& pipelineState)
{
    const bool validVertexType = EnumValue(vertexType) > EnumValue(VT_NONE) && EnumValue(vertexType) < EnumValue(VertexType::MAX_VALUE);
    if (!validVertexType)
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetPipeline failed, invalid vertex type provided. Value=" << EnumValue(vertexType));
        return;
    }

    if (GetVertexType() != VT_NONE && GetVertexType() != vertexType)
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetPipeline failed, unable to switch vertex type. You must clear the existing data first. Src=" << ToString(GetVertexType()) << ", Dest=" << ToString(vertexType));
        return;
    }

    mVertexType = vertexType;
    mPSO = pipelineState;
}

void MeshModelRenderer::SetIndices(const TVector<UInt16>& indices)
{
    ScopeLock lock(mLock);
    if (mVertexBuffer && mVertexBuffer->GetNumElements() != indices.size())
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetIndices failed, invalid index count. Expected=" << mVertexBuffer->GetNumElements() << ", Actual=" << indices.size());
        return;
    }
    mIndexBuffer = Renderer().Device().CreateResource<GfxIndexBuffer>();
    mIndexBuffer->SetUsage(Gfx::BufferUsage::STATIC);
    mIndexBuffer->SetIndices(indices);
    SetDirty(DF_INDEX_BUFFER);
}
void MeshModelRenderer::SetVertices(const TVector<VertexSimple>& vertices)
{
    ScopeLock lock(mLock);
    if (GetVertexType() != VT_SIMPLE && GetVertexType() != VT_NONE)
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetVertices(Simple) failed, invalid vertex type. Src=" << ToString(GetVertexType()) << ", Dest=" << ToString(VT_SIMPLE));
        return;
    }
    if (mIndexBuffer && mIndexBuffer->GetNumElements() != vertices.size())
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetVertices(Simple) failed, invalid vertex count. Expected=" << mIndexBuffer->GetNumElements() << ", Actual=" << vertices.size());
        return;
    }
    mVertexType = VT_SIMPLE;
    mVertexBuffer = Renderer().Device().CreateResource<GfxVertexBuffer>();
    mVertexBuffer->SetUsage(Gfx::BufferUsage::STATIC);
    mVertexBuffer->SetVertices(vertices);
    SetDirty(DF_VERTEX_BUFFER);
}
void MeshModelRenderer::SetVertices(const TVector<VertexTexture>& vertices)
{
    ScopeLock lock(mLock);
    if (GetVertexType() != VT_SIMPLE && GetVertexType() != VT_NONE)
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetVertices(Texture) failed, invalid vertex type. Src=" << ToString(GetVertexType()) << ", Dest=" << ToString(VT_TEXTURE));
        return;
    }
    if (mIndexBuffer && mIndexBuffer->GetNumElements() != vertices.size())
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetVertices(Texture) failed, invalid vertex count. Expected=" << mIndexBuffer->GetNumElements() << ", Actual=" << vertices.size());
        return;
    }
    mVertexType = VT_TEXTURE;
    mVertexBuffer = Renderer().Device().CreateResource<GfxVertexBuffer>();
    mVertexBuffer->SetUsage(Gfx::BufferUsage::STATIC);
    mVertexBuffer->SetVertices(vertices);
    SetDirty(DF_VERTEX_BUFFER);
}
void MeshModelRenderer::SetVertices(const TVector<VertexStandard>& vertices)
{
    ScopeLock lock(mLock);
    if (GetVertexType() != VT_SIMPLE && GetVertexType() != VT_NONE)
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetVertices(Standard) failed, invalid vertex type. Src=" << ToString(GetVertexType()) << ", Dest=" << ToString(VT_STANDARD));
        return;
    }
    if (mIndexBuffer && mIndexBuffer->GetNumElements() != vertices.size())
    {
        LF_LOG_ERROR(gGfxLog, "MeshModelRenderer::SetVertices(Standard) failed, invalid vertex count. Expected=" << mIndexBuffer->GetNumElements() << ", Actual=" << vertices.size());
        return;
    }
    mVertexType = VT_STANDARD;
    mVertexBuffer = Renderer().Device().CreateResource<GfxVertexBuffer>();
    mVertexBuffer->SetUsage(Gfx::BufferUsage::STATIC);
    mVertexBuffer->SetVertices(vertices);
    SetDirty(DF_VERTEX_BUFFER);
}

void MeshModelRenderer::SetTexture(const GfxTextureBinaryAsset& textureBinary)
{
    SetTexture(0, textureBinary);
}
void MeshModelRenderer::SetTexture(const GfxTextureAtomicPtr& texture)
{
    SetTexture(0, texture);
}

void MeshModelRenderer::SetTexture(SizeT index, const GfxTextureBinaryAsset& textureBinary)
{
    if (index > LF_ARRAY_SIZE(mTextures))
    {
        return;
    }

    ScopeLock lock(mLock);
    mTextures[index] = Renderer().Device().CreateResource<GfxTexture>();
    mTextures[index]->SetBinary(textureBinary);
    SetDirty(DF_TEXTURE);

    const bool allowTexture = GetVertexType() != VT_SIMPLE;
    if (!allowTexture)
    {
        LF_LOG_WARN(gGfxLog, "MeshModelRenderer::SetTexture called on a renderer that does not support texture rendering.");
    }
}
void MeshModelRenderer::SetTexture(SizeT index, const GfxTextureAtomicPtr& texture)
{
    if (index > LF_ARRAY_SIZE(mTextures))
    {
        return;
    }

    // TODO: Decide how we handle commits, the texture is being set here but we're assuming it has already been setup/commited externally.
    ScopeLock lock(mLock);
    mTextures[index] = texture;
    const bool allowTexture = GetVertexType() != VT_SIMPLE;
    if (!allowTexture)
    {
        LF_LOG_WARN(gGfxLog, "MeshModelRenderer::SetTexture called on a renderer that does not support texture rendering.");
    }
}

} // namespace lf
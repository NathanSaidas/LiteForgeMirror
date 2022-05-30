#pragma once
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Platform/SpinLock.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxModelRenderer.h"

namespace lf {
DECLARE_ATOMIC_PTR(GfxVertexBuffer);
DECLARE_ATOMIC_PTR(GfxIndexBuffer);
DECLARE_ATOMIC_PTR(GfxPipelineState);
DECLARE_ATOMIC_PTR(GfxTexture);
DECLARE_ATOMIC_PTR(GfxUploadBuffer);
DECLARE_ASSET(GfxTextureBinary);
class GfxResourceObject;

class MeshModelRenderer : public GfxModelRenderer
{
    DECLARE_CLASS(MeshModelRenderer, GfxModelRenderer);
public:
    enum VertexType : Int32
    {
        VT_NONE,
        VT_SIMPLE,
        VT_TEXTURE,
        VT_STANDARD,

        MAX_VALUE
    };

    struct VertexSimple
    {
        Vector4 mPosition;
    };

    struct VertexTexture
    {
        Vector4 mPosition;
        Vector4 mTintColor;
        Vector2 mTexCoord;
    };

    struct VertexStandard
    {
        Vector4 mPosition;
        Vector4 mTintColor;
        Vector2 mTexCoord;
        Vector3 mNormal;
    };

    struct PerObject
    {
        Vector3 mWorldPosition;
    };

    MeshModelRenderer();
    ~MeshModelRenderer() override;

    void SetupResource(GfxDevice& device, GfxCommandContext& context) override;
    void OnRender(GfxDevice& device, GfxCommandContext& context) override;

    void SetPipeline(VertexType vertexType, const MemoryBuffer& vertexByteCode, const MemoryBuffer& pixelByteCode);
    void SetPipeline(VertexType vertexType, const GfxPipelineStateAtomicPtr& pipelineState);
    void SetIndices(const TVector<UInt16>& indices);
    void SetVertices(const TVector<VertexSimple>& vertices);
    void SetVertices(const TVector<VertexTexture>& vertices);
    void SetVertices(const TVector<VertexStandard>& vertices);
    void SetTexture(const GfxTextureBinaryAsset& textureBinary);
    void SetTexture(const GfxTextureAtomicPtr& texture);
    void SetTexture(SizeT index, const GfxTextureBinaryAsset& textureBinary);
    void SetTexture(SizeT index, const GfxTextureAtomicPtr& texture);

    VertexType GetVertexType() const { return mVertexType; }
private:
    enum DirtyFlags : Int32
    {
        DF_NONE             = 0,
        DF_VERTEX_BUFFER    = 1 << 0,
        DF_INDEX_BUFFER     = 1 << 1,
        DF_PIPELINE_STATE   = 1 << 2,
        DF_TEXTURE          = 1 << 3
    };
    ENUM_BITWISE_OPERATORS_NESTED(DirtyFlags);
    void SetDirty(DirtyFlags flags) { mDirtyFlags |= flags; }
    void ClearDirty() { mDirtyFlags = DF_NONE; }
    bool IsDirty(DirtyFlags flag) const { return (mDirtyFlags & flag) != DF_NONE; }

    SpinLock mLock;
    DirtyFlags mDirtyFlags;
    VertexType mVertexType;

    GfxVertexBufferAtomicPtr    mVertexBuffer;
    GfxIndexBufferAtomicPtr     mIndexBuffer;
    GfxPipelineStateAtomicPtr   mPSO;
    GfxTextureAtomicPtr         mTextures[8];
    GfxUploadBufferAtomicPtr    mConstantBuffer;
    GfxUploadBufferAtomicPtr    mStructureBuffer;

};

} // namespace lf

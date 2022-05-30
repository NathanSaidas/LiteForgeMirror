#include "AbstractEngine/PCH.h"
#include "GfxInputLayout.h"

namespace lf {
DEFINE_CLASS(lf::GfxInputLayout) { NO_REFLECTION; }

GfxInputLayout::Builder& GfxInputLayout::Builder::AddPosition(UInt32 semanticIndex)
{
    CriticalAssert(mLayout != nullptr);
    mLayout->push_back(Gfx::VertexInputElement());
    auto& element = mLayout->back();
    element.mSemanticIndex = semanticIndex;
    element.mSemanticName = Token("SV_POSITION");
    element.mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
    element.mInputSlot = 0;
    element.mPerVertexData = true;
    element.mInstanceDataStepRate = 0;
    element.mAlignedByteOffset = static_cast<UInt32>(mByteOffset);

    mByteOffset += sizeof(Float32) * 4;
    return *this;
}

GfxInputLayout::Builder& GfxInputLayout::Builder::AddColor(UInt32 semanticIndex)
{
    CriticalAssert(mLayout != nullptr);
    mLayout->push_back(Gfx::VertexInputElement());
    auto& element = mLayout->back();
    element.mSemanticIndex = semanticIndex;
    element.mSemanticName = Token("COLOR");
    element.mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
    element.mInputSlot = 0;
    element.mPerVertexData = true;
    element.mInstanceDataStepRate = 0;
    element.mAlignedByteOffset = static_cast<UInt32>(mByteOffset);

    mByteOffset += sizeof(Float32) * 4;
    return *this;
}

GfxInputLayout::Builder& GfxInputLayout::Builder::AddNormal(UInt32 semanticIndex)
{
    CriticalAssert(mLayout != nullptr);
    mLayout->push_back(Gfx::VertexInputElement());
    auto& element = mLayout->back();
    element.mSemanticIndex = semanticIndex;
    element.mSemanticName = Token("NORMAL");
    element.mFormat = Gfx::ResourceFormat::R32G32B32_FLOAT;
    element.mInputSlot = 0;
    element.mPerVertexData = true;
    element.mInstanceDataStepRate = 0;
    element.mAlignedByteOffset = static_cast<UInt32>(mByteOffset);

    mByteOffset += sizeof(Float32) * 3;
    return *this;
}

GfxInputLayout::Builder& GfxInputLayout::Builder::AddTexture2D(UInt32 semanticIndex)
{
    CriticalAssert(mLayout != nullptr);
    mLayout->push_back(Gfx::VertexInputElement());
    auto& element = mLayout->back();
    element.mSemanticIndex = semanticIndex;
    element.mSemanticName = Token("TEXCOORD");
    element.mFormat = Gfx::ResourceFormat::R32G32_FLOAT;
    element.mInputSlot = 0;
    element.mPerVertexData = true;
    element.mInstanceDataStepRate = 0;
    element.mAlignedByteOffset = static_cast<UInt32>(mByteOffset);

    mByteOffset += sizeof(Float32) * 2;
    return *this;
}

void GfxInputLayout::Serialize(Stream& s)
{
    Super::Serialize(s);

    SERIALIZE_STRUCT_ARRAY(s, mElements, "");
}

}
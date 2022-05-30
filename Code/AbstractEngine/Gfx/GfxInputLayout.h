#pragma once

#include "Runtime/Asset/AssetObject.h"
#include "AbstractEngine/Gfx/GfxTypes.h"

namespace lf {

class LF_ABSTRACT_ENGINE_API GfxInputLayout : public AssetObject
{
    DECLARE_CLASS(GfxInputLayout, AssetObject);
public:
    struct LF_ABSTRACT_ENGINE_API Builder
    {
        Builder(TVector<Gfx::VertexInputElement>* layout) : mLayout(layout), mByteOffset(0) {}

        Builder& AddPosition(UInt32 semanticIndex = 0);
        Builder& AddColor(UInt32 semanticIndex = 0);
        Builder& AddNormal(UInt32 semanticIndex = 0);
        Builder& AddTexture2D(UInt32 semanticIndex = 0);

        SizeT mByteOffset;
        TVector<Gfx::VertexInputElement>* mLayout;
    };

    void Serialize(Stream& s) override;

    Builder Build(bool clear = true) 
    { 
        if (clear)
        {
            mElements.clear();
        }
        return Builder(&mElements); 
    }

    void SetDefine(const Token& value) { mDefine = value; }
    const Token& GetDefine() const { return mDefine; }

    void SetElements(const TVector<Gfx::VertexInputElement>& value) { mElements = value; }
    const TVector<Gfx::VertexInputElement>& GetElements() { return mElements; }

private:
    // Represents the define in the shader code to select the correct vertex input
    Token                            mDefine;
    TVector<Gfx::VertexInputElement> mElements;
};

}
#pragma once
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "Engine/Gfx/ModelRenderers/MeshModelRenderer.h"

namespace lf {
struct MeshTextureComponentData : public ComponentData
{
    using VertexType = MeshModelRenderer::VertexTexture;

    TVector<VertexType> mVertices;
    TVector<UInt16>     mIndices;
};

// Exclusive Mesh Component:
class LF_ENGINE_API MeshTextureComponent : public Component
{
    DECLARE_COMPONENT(MeshTextureComponent);
    // Features  - Position
};

} // namespace lf
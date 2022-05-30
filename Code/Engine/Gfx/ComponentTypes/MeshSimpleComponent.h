#pragma once
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "Engine/Gfx/ModelRenderers/MeshModelRenderer.h"

namespace lf {

struct MeshSimpleComponentData : public ComponentData
{
    using VertexType = MeshModelRenderer::VertexSimple;

    TVector<VertexType> mVertices;
    TVector<UInt16>     mIndices;
};

// Exclusive Mesh Component:
class LF_ENGINE_API MeshSimpleComponent : public Component
{
    DECLARE_COMPONENT(MeshSimpleComponent);
    // Features  - Position
};

} // namespace lf
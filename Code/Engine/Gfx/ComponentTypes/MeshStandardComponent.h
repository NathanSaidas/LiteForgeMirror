#pragma once
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "Engine/Gfx/ModelRenderers/MeshModelRenderer.h"

namespace lf {

    struct MeshStandardComponentData : public ComponentData
    {
        using VertexType = MeshModelRenderer::VertexStandard;

        TVector<VertexType> mVertices;
        TVector<UInt16>     mIndices;
    };

    // Exclusive Mesh Component:
    class LF_ENGINE_API MeshStandardComponent : public Component
    {
        DECLARE_COMPONENT(MeshStandardComponent);
        // Features  - Position
    };

} // namespace lf
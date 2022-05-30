#pragma once
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"
#include "Engine/Gfx/ModelRenderers/MeshModelRenderer.h"

namespace lf {
DECLARE_ATOMIC_PTR(MeshModelRenderer);

struct MeshRendererComponentData : public ComponentData
{
    MeshModelRendererAtomicPtr mRenderer;
};

class LF_ENGINE_API MeshRendererComponent : public Component
{
    DECLARE_COMPONENT(MeshRendererComponent);

};

} // namespace lf
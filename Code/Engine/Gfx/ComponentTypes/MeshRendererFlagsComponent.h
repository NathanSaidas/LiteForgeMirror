#pragma once
#include "AbstractEngine/World/Component.h"
#include "AbstractEngine/World/ComponentFactory.h"

namespace lf {

enum class MeshRendererFlags : Int32
{
    None,
    DirtyBuffers,
    DirtyTexture
};
ENUM_BITWISE_OPERATORS(MeshRendererFlags);

struct MeshRendererFlagsComponentData : public ComponentData
{
    MeshRendererFlags mFlags;

    LF_INLINE void Set(MeshRendererFlags flags) { mFlags |= flags; }
    LF_INLINE void Clear() { mFlags = MeshRendererFlags::None; }
    LF_INLINE bool Test(MeshRendererFlags flags) { return (mFlags & flags) != MeshRendererFlags::None; }
};

class LF_ENGINE_API MeshRendererFlagsComponent : public Component
{
    DECLARE_COMPONENT(MeshRendererFlagsComponent);
};

}
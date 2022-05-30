#include "AbstractEngine/PCH.h"
#include "GfxRenderTexture.h"

namespace lf {

DEFINE_ABSTRACT_CLASS(lf::GfxRenderTexture) { NO_REFLECTION; }

GfxRenderTexture::GfxRenderTexture()
: Super()
, mWidth(INVALID)
, mHeight(INVALID)
, mFormat(Gfx::ResourceFormat::R8G8B8A8_UNORM)
{}

} // namespace lf
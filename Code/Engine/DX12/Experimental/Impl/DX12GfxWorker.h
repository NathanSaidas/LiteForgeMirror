#pragma once
#include "AbstractEngine/Gfx/GfxBase.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {
namespace experimental {

class DX12GfxWorker
{
public:
    DX12GfxWorker();
    ~DX12GfxWorker();

    bool Initialize(GfxDependencyContext& context);
    void Release();

private:

};

} // namespace experimental
} // namespace lf
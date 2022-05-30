#pragma once

#include "DX12Common.h"

namespace lf {

class GfxDevice;
class GfxCommandContext;

// internal API
ComPtr<ID3D12Device> GetDX12Device(GfxDevice& device);
ComPtr<ID3D12GraphicsCommandList> GetDX12GraphicsCommandList(GfxCommandContext& context);

} // namespace lf
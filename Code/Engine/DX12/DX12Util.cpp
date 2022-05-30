#include "Engine/PCH.h"
#include "DX12Util.h"

#include "Engine/DX12/DX12GfxDevice.h"
#include "Engine/DX12/DX12GfxCommandContext.h"

namespace lf {

ComPtr<ID3D12Device> GetDX12Device(GfxDevice& device)
{
    CriticalAssert(device.IsA(typeof(DX12GfxDevice)));
    return static_cast<DX12GfxDevice&>(device).Device();
}
ComPtr<ID3D12GraphicsCommandList> GetDX12GraphicsCommandList(GfxCommandContext& context)
{
    CriticalAssert(context.IsA(typeof(DX12GfxCommandContext)));
    return static_cast<DX12GfxCommandContext&>(context).CommandList();
}

} // namespace lf 
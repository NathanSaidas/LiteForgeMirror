// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
// 
// Permission is hereby granted, free of charge, to any person obtaining a 
// copy of this software and associated documentation files(the "Software"), 
// to deal in the Software without restriction, including without limitation 
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and / or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions :
// 
// The above copyright notice and this permission notice shall be included in 
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE 
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER 
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// ********************************************************************
#pragma once
#include "AbstractEngine/Gfx/GfxPipelineState.h"
#include "Engine/DX12/DX12Common.h"

namespace lf {

class LF_ENGINE_API DX12GfxPipelineState : public GfxPipelineState
{
    DECLARE_CLASS(DX12GfxPipelineState, GfxPipelineState);
public:
    DX12GfxPipelineState();

    void Release() override;
    void Commit(GfxDevice& device, GfxCommandContext& context) override;


    // https://docs.microsoft.com/en-us/windows/win32/api/d3d12/ns-d3d12-d3d12_rasterizer_desc
    // Suggested Line Render Algorithms for DirectX 12
    // void SetLineRenderAliased() { SetRasterMSAA(false); SetRasterLineAA(false); }
    // void SetLineRenderAlphaAntialiased() { SetRasterMSAA(false); SetRasterLineAA(true); }
    // void SetLineRenderQuadrilateral() { SetRasterMSAA(true); SetRasterLineAA(false); }
    // void SetLineRenderQuadrilateralAliased() { SetRasterMSAA(true); SetRasterLineAA(true); }



    ComPtr<ID3D12PipelineState> GetPSO() const { return mPSO; }
    ComPtr<ID3D12RootSignature> GetRootSignature() const { return mRootSignature; }
protected:
    bool AllowChanges() const override { return mPSO == nullptr; }
private:
    // Internal:
    ComPtr<ID3D12PipelineState> mPSO;
    ComPtr<ID3D12RootSignature> mRootSignature;


};

} // namespace lf
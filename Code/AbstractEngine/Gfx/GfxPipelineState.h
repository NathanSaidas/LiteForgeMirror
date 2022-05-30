// ********************************************************************
// Copyright (c) 2021 Nathan Hanlan
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
#include "Core/Memory/MemoryBuffer.h"
#include "AbstractEngine/Gfx/GfxResourceObject.h"
#include "AbstractEngine/Gfx/GfxTypes.h"

namespace lf {

// Calling SetXXX function requires changes to be allowed to be made.
// Typically after you call Commit the resource cannot be changed.
class LF_ABSTRACT_ENGINE_API GfxPipelineState : public GfxResourceObject
{
    DECLARE_CLASS(GfxPipelineState, GfxResourceObject);
public:
    using InputLayoutVector = TStackVector<Gfx::VertexInputElement, 8>;
    using ShaderParamVector = TStackVector<Gfx::ShaderParam, 8>;
    GfxPipelineState();

    void SetBlendState(const Gfx::BlendStateDesc& value) { ReportBug(AllowChanges()); mBlendStateDesc = value; }
    const Gfx::BlendStateDesc& GetBlendState() const { return mBlendStateDesc; }
    void SetRasterState(const Gfx::RasterStateDesc& value) { ReportBug(AllowChanges()); mRasterStateDesc = value; }
    const Gfx::RasterStateDesc& GetRasterState() const { return mRasterStateDesc; }
    void SetDepthStencilState(const Gfx::DepthStencilStateDesc& value) { ReportBug(AllowChanges()); mDepthStencilDesc = value; }
    const Gfx::DepthStencilStateDesc& GetDepthStencilState() const { return mDepthStencilDesc; }
    void SetRenderMode(Gfx::RenderMode value) { ReportBug(AllowChanges()); mRenderMode = value; }
    Gfx::RenderMode GetRenderMode() const { return mRenderMode; }
    void SetRenderTargetFormat(Gfx::ResourceFormat value) { ReportBug(AllowChanges()); mRenderTargetFormat = value; }
    Gfx::ResourceFormat GetRenderTargetFormat() const { return mRenderTargetFormat; }
    void SetInputLayout(const InputLayoutVector& value) { ReportBug(AllowChanges()); mInputLayout = value; }
    const InputLayoutVector& GetInputLayout() const { return mInputLayout; }
    void SetShaderParams(const ShaderParamVector& value) { ReportBug(AllowChanges()); mShaderParams = value; }
    const ShaderParamVector& GetShaderParams() const { return mShaderParams; }

    Gfx::ShaderParamID FindParam(const Token& name);

    void SetShaderByteCode(Gfx::ShaderType shader, MemoryBuffer&& buffer);
    void SetShaderByteCode(Gfx::ShaderType shader, const MemoryBuffer& buffer);
    const MemoryBuffer& GetShaderByteCode(Gfx::ShaderType shader);

    bool IsGPUReady() const { return AtomicLoad(&mGPUReady); }
protected:
    void SetGPUReady(bool value) { AtomicStore(&mGPUReady, value ? 1 : 0); }
    virtual bool AllowChanges() const = 0;
private:

    // Public:
    volatile Atomic32 mGPUReady;
    MemoryBuffer mShaderByteCode[ENUM_SIZE(Gfx::ShaderType)];
    Gfx::BlendStateDesc mBlendStateDesc;
    Gfx::RasterStateDesc mRasterStateDesc;
    Gfx::DepthStencilStateDesc mDepthStencilDesc;
    Gfx::RenderMode mRenderMode;
    Gfx::ResourceFormat mRenderTargetFormat;
    InputLayoutVector mInputLayout;
    ShaderParamVector mShaderParams;
};

} // namespace lf
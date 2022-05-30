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
#include "AbstractEngine/PCH.h"
#include "GfxPipelineState.h"

namespace lf {

DEFINE_ABSTRACT_CLASS(lf::GfxPipelineState) { NO_REFLECTION; }

GfxPipelineState::GfxPipelineState()
: Super()
, mGPUReady(0)
, mShaderByteCode()
, mBlendStateDesc()
, mRasterStateDesc()
, mDepthStencilDesc()
, mRenderMode(Gfx::RenderMode::TRIANGLES)
, mRenderTargetFormat(Gfx::ResourceFormat::R8G8B8A8_UNORM)
, mInputLayout()
, mShaderParams()
{}

Gfx::ShaderParamID GfxPipelineState::FindParam(const Token& name)
{
    for (SizeT i = 0; i < mShaderParams.size(); ++i)
    {
        if (mShaderParams[i].GetName() == name)
        {
            return Gfx::ShaderParamID(static_cast<Gfx::ShaderParamID::Value>(i), mShaderParams[i].GetType());
        }
    }
    return Gfx::ShaderParamID();
}

void GfxPipelineState::SetShaderByteCode(Gfx::ShaderType shader, MemoryBuffer&& buffer)
{
    ReportBug(AllowChanges());
    CriticalAssert(ValidEnum(shader));
    mShaderByteCode[EnumValue(shader)] = std::move(buffer);
}
void GfxPipelineState::SetShaderByteCode(Gfx::ShaderType shader, const MemoryBuffer& buffer)
{
    ReportBug(AllowChanges());
    CriticalAssert(ValidEnum(shader));
    mShaderByteCode[EnumValue(shader)].Copy(buffer);
}

const MemoryBuffer& GfxPipelineState::GetShaderByteCode(Gfx::ShaderType shader)
{
    ReportBug(AllowChanges());
    CriticalAssert(ValidEnum(shader));
    return mShaderByteCode[EnumValue(shader)];
}

} // namespace lf
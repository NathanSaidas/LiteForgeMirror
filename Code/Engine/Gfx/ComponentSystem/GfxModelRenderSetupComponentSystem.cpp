// ********************************************************************
// Copyright (c) 2022 Nathan Hanlan
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
#include "Engine/PCH.h"
#include "GfxModelRenderSetupComponentSystem.h"
#include "AbstractEngine/World/WorldScene.h"
#include "Engine/Gfx/GameRenderer.h"

namespace lf {

DEFINE_CLASS(lf::GfxModelRenderSetupComponentSystem) { NO_REFLECTION; }

/// Fence we use to register all our gfx models
class GfxModelRendererSetupFence : public ComponentSystemFence
{
    DECLARE_CLASS(GfxModelRendererSetupFence, ComponentSystemFence);
};
DEFINE_ABSTRACT_CLASS(lf::GfxModelRendererSetupFence) { NO_REFLECTION; }

GfxModelRenderSetupComponentSystem::GfxModelRenderSetupComponentSystem()
: Super()
, mProceduralTuple()
, mRegistered(false)
, mDebugPixelByteCode()
, mDebugVertexByteCode()
, mGameRenderer()
{

}

GfxModelRenderSetupComponentSystem::~GfxModelRenderSetupComponentSystem()
{

}

void GfxModelRenderSetupComponentSystem::SetDebugByteCode(const MemoryBuffer& vertexByteCode, const MemoryBuffer& pixelByteCode)
{
    mDebugPixelByteCode.Copy(pixelByteCode);
    mDebugVertexByteCode.Copy(vertexByteCode);
}

void GfxModelRenderSetupComponentSystem::SetGameRenderer(const GameRendererAtomicPtr& gameRenderer)
{
    mGameRenderer = gameRenderer;
}

bool GfxModelRenderSetupComponentSystem::OnInitialize()
{
    Assert(GetWorld()->CreateFenceBefore(typeof(GfxModelRendererSetupFence), typeof(ComponentSystemUpdateFence)));
    return true;
}

void GfxModelRenderSetupComponentSystem::OnBindTuples()
{
    BindTuple(mProceduralTuple);
}

void GfxModelRenderSetupComponentSystem::OnScheduleUpdates()
{
    if (!mRegistered)
    {
        Assert(StartConstantUpdate(
            ECSUtil::UpdateCallback::Make(this, &GfxModelRenderSetupComponentSystem::Update),
            typeof(GfxModelRendererSetupFence),
            ECSUtil::UpdateType::SERIAL,
            { },
            { typeof(ProceduralMeshComponent), typeof(WorldDataComponent) }));


        mRegistered = true;
    }
}

void GfxModelRenderSetupComponentSystem::Update()
{
    if (mGameRenderer)
    {
        ForEach(mProceduralTuple, ProceduralUpdate::Make(this, &GfxModelRenderSetupComponentSystem::UpdateProcedural));
    }
}

void GfxModelRenderSetupComponentSystem::UpdateProcedural(WorldDataComponentData* worldData, ProceduralMeshComponentData* procedural)
{
    (worldData); 
    (procedural);
    if (procedural->mDirty || procedural->mDirtyTexture)
    {
        if (!procedural->mModelRenderer)
        {
            procedural->mModelRenderer = mGameRenderer->CreateModelRenderer<ProceduralModelRenderer>();
        }
    
        if (procedural->mModelRenderer)
        {
            if (procedural->mDirty)
            {
                procedural->mModelRenderer->SetData(procedural->mVertices, procedural->mIndices, mDebugVertexByteCode, mDebugPixelByteCode);
                procedural->mDirty = false;
            }

            if (procedural->mDirtyTexture)
            {
                procedural->mModelRenderer->SetTexture(procedural->mTexture);
                procedural->mDirtyTexture = false;
            }
        }
    }
}

} // namespace lf
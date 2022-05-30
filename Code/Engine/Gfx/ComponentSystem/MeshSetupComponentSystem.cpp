#include "Engine/PCH.h"
#include "MeshSetupComponentSystem.h"
#include "Engine/Gfx/GameRenderer.h"
#include "Engine/Gfx/ModelRenderers/MeshModelRenderer.h"

namespace lf {
DEFINE_CLASS(lf::MeshSetupComponentSystem) { NO_REFLECTION; }
class MeshSetupFence : public ComponentSystemFence
{
    DECLARE_CLASS(MeshSetupFence, ComponentSystemFence);
};
DEFINE_ABSTRACT_CLASS(lf::MeshSetupFence) { NO_REFLECTION; }

MeshSetupComponentSystem::MeshSetupComponentSystem()
: Super()
, mSimpleMeshTuple()
, mTextureMeshTuple()
, mStandardMeshTuple()
, mGameRenderer()
, mRegistered(false)
{}
MeshSetupComponentSystem::~MeshSetupComponentSystem()
{}

void MeshSetupComponentSystem::SetGameRenderer(const GameRendererAtomicPtr& gameRenderer)
{
    mGameRenderer = gameRenderer;
}
bool MeshSetupComponentSystem::OnInitialize()
{
    Assert(GetWorld()->CreateFenceBefore(typeof(MeshSetupFence), typeof(ComponentSystemUpdateFence)));
    return true;
}
void MeshSetupComponentSystem::OnBindTuples()
{
    // auto meshSimple = typeof(MeshSimpleComponent);
    // auto meshSimple = typeof(MeshSimpleComponent);
    // auto meshSimple = typeof(MeshSimpleComponent);

    BindTuple(mSimpleMeshTuple);
    BindTuple(mTextureMeshTuple);
    BindTuple(mStandardMeshTuple);
}
void MeshSetupComponentSystem::OnScheduleUpdates()
{
    if (!mRegistered)
    {
        Assert(StartConstantUpdate(
            ECSUtil::UpdateCallback::Make(this, &MeshSetupComponentSystem::UpdateSimpleMesh),
            typeof(MeshSetupFence),
            ECSUtil::UpdateType::SERIAL,
            { },
            { typeof(MeshRendererFlagsComponent), typeof(MeshRendererComponent), typeof(MeshSimpleComponent) }));

        Assert(StartConstantUpdate(
            ECSUtil::UpdateCallback::Make(this, &MeshSetupComponentSystem::UpdateTextureMesh),
            typeof(MeshSetupFence),
            ECSUtil::UpdateType::SERIAL,
            { },
            { typeof(MeshRendererFlagsComponent), typeof(MeshRendererComponent), typeof(MeshTextureComponent) }));

        Assert(StartConstantUpdate(
            ECSUtil::UpdateCallback::Make(this, &MeshSetupComponentSystem::UpdateStandardMesh),
            typeof(MeshSetupFence),
            ECSUtil::UpdateType::SERIAL,
            { },
            { typeof(MeshRendererFlagsComponent), typeof(MeshRendererComponent), typeof(MeshStandardComponent) }));


        mRegistered = true;
    }
}
void MeshSetupComponentSystem::UpdateSimpleMesh()
{
    if (mGameRenderer)
    {
        ForEach(mSimpleMeshTuple, SimpleMeshSetupCallback::Make(this, &MeshSetupComponentSystem::UpdateSimpleMeshEntity));
    }
}
void MeshSetupComponentSystem::UpdateSimpleMeshEntity(MeshRendererFlagsComponentData* flags, MeshRendererComponentData* renderer, MeshSimpleComponentData* mesh)
{
    // no work
    if (!flags->Test(MeshRendererFlags::DirtyBuffers | MeshRendererFlags::DirtyTexture))
    {
        return;
    }

    // Create renderer
    if (!renderer->mRenderer)
    {
        renderer->mRenderer = mGameRenderer->CreateModelRenderer<MeshModelRenderer>();
        if (renderer->mRenderer)
        {
            renderer->mRenderer->SetPipeline(MeshModelRenderer::VT_SIMPLE, mGameRenderer->GetDebugShader(GfxRenderer::DebugShaderType::SIMPLE_MESH));
        }
    }

    // clear dirty bits
    if (renderer->mRenderer)
    {
        if (flags->Test(MeshRendererFlags::DirtyBuffers))
        {
            renderer->mRenderer->SetIndices(mesh->mIndices);
            renderer->mRenderer->SetVertices(mesh->mVertices);
        }
        flags->Clear();
    }
}

void MeshSetupComponentSystem::UpdateTextureMesh()
{
    if (mGameRenderer)
    {
        ForEach(mTextureMeshTuple, TextureMeshSetupCallback::Make(this, &MeshSetupComponentSystem::UpdateTextureMeshEntity));
    }
}

void MeshSetupComponentSystem::UpdateTextureMeshEntity(MeshRendererFlagsComponentData* flags, MeshRendererComponentData* renderer, MeshTextureComponentData* mesh)
{
    // no work
    if (!flags->Test(MeshRendererFlags::DirtyBuffers | MeshRendererFlags::DirtyTexture))
    {
        return;
    }

    // Create renderer
    if (!renderer->mRenderer)
    {
        renderer->mRenderer = mGameRenderer->CreateModelRenderer<MeshModelRenderer>();
        if (renderer->mRenderer)
        {
            renderer->mRenderer->SetPipeline(MeshModelRenderer::VT_SIMPLE, mGameRenderer->GetDebugShader(GfxRenderer::DebugShaderType::TEXTURE_MESH));
        }
    }

    // clear dirty bits
    if (renderer->mRenderer)
    {
        if (flags->Test(MeshRendererFlags::DirtyBuffers))
        {
            renderer->mRenderer->SetIndices(mesh->mIndices);
            renderer->mRenderer->SetVertices(mesh->mVertices);
        }
        if (flags->Test(MeshRendererFlags::DirtyTexture))
        {
            renderer->mRenderer->SetTexture(0, mGameRenderer->GetDebugTexture(GfxRenderer::GREEN));
            renderer->mRenderer->SetTexture(1, mGameRenderer->GetDebugTexture(GfxRenderer::RED));
            renderer->mRenderer->SetTexture(2, mGameRenderer->GetDebugTexture(GfxRenderer::PURPLE));
        }
        flags->Clear();
    }
}

void MeshSetupComponentSystem::UpdateStandardMesh()
{
    if (mGameRenderer)
    {
        ForEach(mStandardMeshTuple, StandardMeshSetupCallback::Make(this, &MeshSetupComponentSystem::UpdateStandardMeshEntity));
    }
}
void MeshSetupComponentSystem::UpdateStandardMeshEntity(MeshRendererFlagsComponentData* flags, MeshRendererComponentData* renderer, MeshStandardComponentData* mesh)
{
    // no work
    if (!flags->Test(MeshRendererFlags::DirtyBuffers | MeshRendererFlags::DirtyTexture))
    {
        return;
    }

    // Create renderer
    if (!renderer->mRenderer)
    {
        renderer->mRenderer = mGameRenderer->CreateModelRenderer<MeshModelRenderer>();
        if (renderer->mRenderer)
        {
            renderer->mRenderer->SetPipeline(MeshModelRenderer::VT_SIMPLE, mGameRenderer->GetDebugShader(GfxRenderer::DebugShaderType::STANDARD_MESH));
        }
    }

    // clear dirty bits
    if (renderer->mRenderer)
    {
        if (flags->Test(MeshRendererFlags::DirtyBuffers))
        {
            renderer->mRenderer->SetIndices(mesh->mIndices);
            renderer->mRenderer->SetVertices(mesh->mVertices);
        }
        if (flags->Test(MeshRendererFlags::DirtyTexture))
        {
            renderer->mRenderer->SetTexture(mGameRenderer->GetDebugTexture(GfxRenderer::RED));
        }
        flags->Clear();
    }
}


} // namespace lf
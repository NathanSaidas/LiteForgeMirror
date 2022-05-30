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
#include "Engine/PCH.h"
#include "GameRenderer.h"
#include "Core/Common/Enum.h"
#include "Core/Reflection/DynamicCast.h"
#include "Core/Math/Color.h"
#include "Core/Math/Vector2.h"
#include "Core/Math/Vector3.h"
#include "Core/Math/Vector4.h"
#include "Core/Utility/Log.h"
#include "Runtime/Reflection/ReflectionMgr.h"
#include "AbstractEngine/Gfx/GfxCommandContext.h"
#include "AbstractEngine/Gfx/GfxCommandQueue.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxModelRenderer.h"
#include "AbstractEngine/Gfx/GfxTypes.h"
#include "AbstractEngine/Gfx/GfxSwapChain.h"
#include "AbstractEngine/Gfx/GfxRendererDependencyContext.h"
#include "AbstractEngine/Gfx/GfxFence.h"
#include "AbstractEngine/Gfx/GfxRenderTexture.h"
#include "AbstractEngine/Gfx/GfxInputLayout.h"

namespace lf {
struct TestRenderTextureVertex
{
    Vector4 mPosition;
    Vector2 mTexCoord;
};

DEFINE_CLASS(lf::GameRenderer) { NO_REFLECTION; }
GameRenderer::GameRenderer()
: Super()
, mDevice(nullptr)
, mOutputTarget()
, mCommandQueue()
, mDebugResourceState(DebugResourceState::None)
{}
GameRenderer::~GameRenderer()
{}
bool GameRenderer::Initialize(GfxDependencyContext& context)
{
    GfxRendererDependencyContext* rendererDeps = DynamicCast<GfxRendererDependencyContext>(&context);
    if (!rendererDeps)
    {
        return false;
    }
    mDevice = context.GetGfxDevice();
    mCommandQueue = rendererDeps->GetCommandQueue();

    if(mAssets)
    {
        mTestInputLayout = GfxInputLayoutAsset(AssetPath("Engine//BuiltIn/InputLayouts/TextureMesh.json"),  AssetLoadFlags::LF_RECURSIVE_PROPERTIES);

        GfxPipelineState::ShaderParamVector simpleParams;

        GfxPipelineState::ShaderParamVector textureParams;
        textureParams.push_back(Gfx::ShaderParam().InitTexture2D(Token("gTextures0"), 0));
        textureParams.push_back(Gfx::ShaderParam().InitTexture2D(Token("gTextures1"), 1));
        textureParams.push_back(Gfx::ShaderParam().InitTexture2D(Token("gTextures2"), 2));
        textureParams.push_back(Gfx::ShaderParam().InitConstantBuffer(Token("gPerObject"), 0, sizeof(Vector3), 1));
        textureParams.push_back(Gfx::ShaderParam().InitStructuredBuffer(Token("gPerObjectStruct"), 3, sizeof(Vector3), 1));

        GfxPipelineState::ShaderParamVector standardParams;

        const String simpleMeshText = mAssets->GetShaderText("Engine//BuiltIn/Shaders/SimpleMesh.shader");
        const String textureMeshText = mAssets->GetShaderText("Engine//BuiltIn/Shaders/TextureMesh.shader");
        const String standardMeshText = mAssets->GetShaderText("Engine//BuiltIn/Shaders/StandardMesh.shader");
        mTestShaderText = mAssets->GetShaderText("Engine//BuiltIn/Shaders/TestRenderTexture.shader");


        const String* texts[] = { &simpleMeshText, &textureMeshText, &standardMeshText };
        const GfxPipelineState::ShaderParamVector* params[] = { &simpleParams, &textureParams, &standardParams };
        LF_STATIC_ASSERT(LF_ARRAY_SIZE(texts) <= ENUM_SIZE_EX(DebugShaderType));
        LF_STATIC_ASSERT(LF_ARRAY_SIZE(params) == LF_ARRAY_SIZE(texts));


        for (SizeT i = 0; i < LF_ARRAY_SIZE(texts); ++i)
        {
            auto type = ToEnum<DebugShaderType>(static_cast<Int32>(i));

            MemoryBuffer vertex;
            MemoryBuffer pixel;

            mAssets->GetShaderBinary(Gfx::ShaderType::VERTEX, *texts[i], { Token("LF_VERTEX") }, vertex);
            mAssets->GetShaderBinary(Gfx::ShaderType::PIXEL, *texts[i], { Token("LF_PIXEL") }, pixel);

            mDebugShaders[type] = mDevice->CreateResource<GfxPipelineState>();
            mDebugShaders[type]->SetShaderByteCode(Gfx::ShaderType::VERTEX, vertex);
            mDebugShaders[type]->SetShaderByteCode(Gfx::ShaderType::PIXEL, pixel);
            TStackVector<Gfx::VertexInputElement, 8> inputLayout;
            CreateInputLayout(type, inputLayout);
            mDebugShaders[type]->SetInputLayout(inputLayout);
            mDebugShaders[type]->SetShaderParams(*params[i]);
        }

        const String textures[] = {
            "Engine//BuiltIn/Textures/Red.png",
            "Engine//BuiltIn/Textures/Green.png",
            "Engine//BuiltIn/Textures/Purple.png",
        };
        LF_STATIC_ASSERT(LF_ARRAY_SIZE(textures) <= ENUM_SIZE_EX(DebugTextureType));

        for (SizeT i = 0; i < LF_ARRAY_SIZE(textures); ++i)
        {
            mDebugTextures[i] = mDevice->CreateResource<GfxTexture>();
            mDebugTextures[i]->SetBinary(mAssets->GetTexture(textures[i]));
        }

        mDebugResourceState = DebugResourceState::Created;
    }
    else
    {
        LF_LOG_WARN(gGfxLog, "Using GameRenderer without asset provider, unable to generate debug resources.");
    }

    return mCommandQueue;
}

void GameRenderer::CreateInputLayout(DebugShaderType type, TStackVector<Gfx::VertexInputElement, 8>& inputLayout)
{
    UInt32 byteOffset = 0;

    inputLayout.push_back(Gfx::VertexInputElement());
    inputLayout.back().mSemanticIndex = 0;
    inputLayout.back().mSemanticName = Token("SV_POSITION");
    inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
    inputLayout.back().mInputSlot = 0;
    inputLayout.back().mPerVertexData = true;
    inputLayout.back().mInstanceDataStepRate = 0;
    inputLayout.back().mAlignedByteOffset = byteOffset;
    byteOffset += sizeof(Vector4);


    if (type == TEXTURE_MESH || type == STANDARD_MESH)
    {
        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("COLOR");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = byteOffset;
        byteOffset += sizeof(Vector4);

        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("TEXCOORD");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = byteOffset;
        byteOffset += sizeof(Vector2);
    }

    if (type == STANDARD_MESH)
    {
        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("NORMAL");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = byteOffset;
        byteOffset += sizeof(Vector3);
    }
}

void GameRenderer::Shutdown()
{
    {
        ScopeLock lock(mObjectsLock);
        mObjects.clear();
    }

    {
        ScopeLock lock(mNewObjectsLock);
        mNewObjects.clear();
    }

    for (auto& shader : mDebugShaders)
    {
        shader.Release();
    }
    for (auto& texture : mDebugTextures)
    {
        texture.Release();
    }
    for (auto& renderTexture : mTestRenderTexture)
    {
        renderTexture.Release();
    }

    mOutputTarget.Release();
    mCommandQueue.Release();
}

GfxModelRendererAtomicPtr GameRenderer::CreateModelRendererOfType(const Type* type)
{
    GfxModelRendererAtomicPtr modelRenderer = GetReflectionMgr().CreateAtomic<GfxModelRenderer>(type);
    if (!modelRenderer)
    {
        return NULL_PTR;
    }

    {
        ScopeLock lock(mNewObjectsLock);
        mNewObjects.insert(modelRenderer);
    }

    modelRenderer->SetRenderer(this);
    return modelRenderer;
}

GfxDevice& GameRenderer::Device()
{
    CriticalAssert(mDevice != nullptr);
    return *mDevice;
}

void GameRenderer::SetWindow(GfxSwapChain* outputTarget)
{
    mOutputTarget = GetAtomicPointer(outputTarget);
}

void GameRenderer::SetupResource(GfxDevice& device, GfxCommandContext& context)
{
    ScopeLock lock(mObjectsLock);
    if (mDebugResourceState == DebugResourceState::Created)
    {
        for (auto& shader : mDebugShaders)
        {
            if (shader)
            {
                shader->Commit(device, context);
            }
        }

        for (auto& texture : mDebugTextures)
        {
            if (texture)
            {
                texture->Commit(device, context);
            }
        }
        mDebugResourceState = DebugResourceState::Committed;
    }

    for (GfxModelRenderer* renderer : mObjects)
    {
        renderer->SetupResource(device, context);
    }

    if (!mTestRenderTexture[0]) {
        for (SizeT i = 0; i < Gfx::FrameCount::Value; ++i)
        {
            mTestRenderTexture[i] = device.CreateResource<GfxRenderTexture>();
            mTestRenderTexture[i]->SetWidth(640);
            mTestRenderTexture[i]->SetHeight(640);
            mTestRenderTexture[i]->SetResourceFormat(Gfx::ResourceFormat::R8G8B8A8_UNORM);
            mTestRenderTexture[i]->Commit(device, context);
        }
        

        const String text = mTestShaderText; 

        MemoryBuffer vertex;
        MemoryBuffer pixel;
        Assert(mAssets->GetShaderBinary(Gfx::ShaderType::VERTEX, text, { Token("LF_VERTEX") }, vertex));
        Assert(mAssets->GetShaderBinary(Gfx::ShaderType::PIXEL, text, { Token("LF_PIXEL") }, pixel));

        GfxPipelineState::ShaderParamVector shaderParams;
        shaderParams.push_back(Gfx::ShaderParam().InitTexture2D(Token("gTextures0"), 0));

        GfxPipelineState::InputLayoutVector inputLayout;
        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("SV_POSITION");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32B32A32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = 0;

        inputLayout.push_back(Gfx::VertexInputElement());
        inputLayout.back().mSemanticIndex = 0;
        inputLayout.back().mSemanticName = Token("TEXCOORD");
        inputLayout.back().mFormat = Gfx::ResourceFormat::R32G32_FLOAT;
        inputLayout.back().mInputSlot = 0;
        inputLayout.back().mPerVertexData = true;
        inputLayout.back().mInstanceDataStepRate = 0;
        inputLayout.back().mAlignedByteOffset = sizeof(Vector4);


        mTestPSO = device.CreateResource<GfxPipelineState>();
        mTestPSO->SetShaderByteCode(Gfx::ShaderType::VERTEX, vertex);
        mTestPSO->SetShaderByteCode(Gfx::ShaderType::PIXEL, pixel);
        mTestPSO->SetShaderParams(shaderParams);
        mTestPSO->SetInputLayout(inputLayout);
        mTestPSO->Commit(device, context);

        TStackVector<TestRenderTextureVertex, 4> vertices;
        vertices.push_back({ {-0.9f, 0.9f, 0.0f, 1.0f}, { 0.0f, 0.0f } }); // top left
        vertices.push_back({ { 0.9f, 0.9f, 0.0f, 1.0f}, { 1.0f, 0.0f } }); // top right
        vertices.push_back({ { 0.9f, -0.9f, 0.0f, 1.0f}, { 1.0f, 1.0f } }); // bottom left
        vertices.push_back({ {-0.9f, -0.9f, 0.0f, 1.0f}, { 0.0f, 1.0f } }); // bottom right

        mTestVBO = device.CreateResource<GfxVertexBuffer>();
        mTestVBO->SetUsage(Gfx::BufferUsage::STATIC);
        mTestVBO->SetVertices(vertices);
        mTestVBO->Commit(device, context);

        TVector<UInt16> indices = { 0, 1, 2, 0, 2, 3 };

        mTestIBO = device.CreateResource<GfxIndexBuffer>();
        mTestIBO->SetUsage(Gfx::BufferUsage::STATIC);
        mTestIBO->SetIndices(indices);
        mTestIBO->Commit(device, context);

        mTestShaderText.Clear();
    }


}

void GameRenderer::OnRender(GfxDevice& device, GfxCommandContext& context)
{
    Gfx::FrameCountType currentFrame = device.GetCurrentFrame() % Gfx::FrameCount::Value;

    // Before we can start rendering the next frame, we need to wait for the last one to be done.. until we start buffering our command contexts!

    // context.ResourceBarrier(mOutputTarget, mCurrentFrame, Present, RenderTarget);
    // context.SetRenderTarget(mOutputTarget, currentFrame );

    context.BindRenderTarget(mTestRenderTexture[currentFrame]);
    
    ViewportF viewport(640, 640);
    context.SetViewport(viewport);
    
    RectI scissor(640, 640);
    context.SetScissorRect(scissor);
    
    context.ClearColor(mTestRenderTexture[currentFrame], Color::Azure);
    
    // TODO: Tutorial_2_BasicTriangle cannot render a triangle because it's not using ECS and GfxModelRenderers'
    // if (mDebugEntity.mGPUReady)
    // {
    //     context.SetIndexBuffer(mDebugEntity.mIndexBuffer);
    //     context.SetVertexBuffer(mDebugEntity.mVertexBuffer);
    //     context.SetTopology(Gfx::RenderMode::TRIANGLES);
    //     context.SetPipelineState(mDebugEntity.mPSO);
    //     context.DrawIndexed(mDebugEntity.mIndexBuffer->GetNumElements());
    // }
    
    if (mDevice)
    {
        ScopeLock lock(mObjectsLock);
        for (GfxModelRenderer* renderer : mObjects)
        {
            renderer->OnRender(device, context);
        }
    }
    context.UnbindRenderTarget(mTestRenderTexture[currentFrame]);

    context.SetRenderTarget(mOutputTarget, currentFrame);
    context.ClearColor(mOutputTarget, currentFrame, Color::AcidGreen);

    context.SetPipelineState(mTestPSO);
    context.SetTexture(Gfx::ShaderParamID(0, Gfx::ShaderParamType::SPT_TEXTURE_2D), mTestRenderTexture->AsPtr());
    context.SetVertexBuffer(mTestVBO);
    context.SetIndexBuffer(mTestIBO);
    context.SetTopology(Gfx::RenderMode::TRIANGLES);
    context.DrawIndexed(mTestIBO->GetNumElements());

    context.SetPresentSwapChainState(mOutputTarget, currentFrame);


    // 
    // mCommandContext->SetTopology(Gfx::RenderMode::TRIANGLES);
    // 
    // for (auto& obj : mRenderSet.mOpaque)
    // {
    //     auto& gd = obj->GetGraphicsData();
    // 
    //     mCommandContext->SetIndexBuffer(gd.mIndexBuffer);
    //     mCommandContext->SetVertexBuffer(gd.mVertexBuffer);
    // 
    // 
    //     
    // }

    // auto objects = mScene->GetObjects();

    // Set the vertex buffer
    // cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
    
    // Set the index buffer
    // cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
    
    // Set the topology
    // cmdList->IASetPrimitiveTopology(ri->PrimitiveType);
    
    // D3D12_GPU_VIRTUAL_ADDRESS objCBAddress = objectCB->GetGPUVirtualAddress() + ri->ObjCBIndex * objCBByteSize;
    
    // cmdList->SetGraphicsRootConstantBufferView(0, objCBAddress);
    // cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);

    // OMSetBlendFactor
    // OMSetRenderTargets
    // OMSetStencilRef
    // 
    // RSSetScissorRects
    // RSSetViewports
    // 
    // SetGraphicsRootDescriptorTable -- Setting a texture?
    // SetGraphicsRootShaderResourceView -- Setting a StructuredBuffer
    // SetGraphicsRootConstantBufferView -- Setting a cbuffer


    // ClearDepthStencilView
    // ClearRenderTargetView

    // SetGraphicsRootConstantBufferView


    // auto renderSet = GetRenderSet();
    // shadowMapFence = WriteCommands(ShadowMap);
    // SubmitCommands(ShadowMap);
    // gbufferFence = WriteCommands(GBuffer);
    // Wait(fence);

}

void GameRenderer::OnBeginFrame()
{
    CommitAndRelease();

    if (mDevice)
    {
        ScopeLock lock(mObjectsLock);
        for (GfxModelRenderer* object : mObjects)
        {
            object->OnUpdate(*mDevice);
        }
    }

    // WaitForCommitBuffers();
}

void GameRenderer::OnEndFrame()
{
    LF_DEBUG_BREAK;
    // mRenderSet.Clear();
    // auto& objects = mScene->GetObjects();
    // 
    // const Float32 renderDistanceSqr = Sqr(mCullingCamera.mRenderDistance);
    // for (const GfxSceneObjectAtomicPtr& sceneObject : objects)
    // {
    //     if (!sceneObject->IsA(typeof(GfxScene3DObject)))
    //     {
    //         continue;
    //     }
    // 
    //     GfxScene3DObjectAtomicPtr object3D = StaticCast<GfxScene3DObjectAtomicPtr>(sceneObject);
    // 
    //     object3D->CopyGameToGraphics();
    //     const Vector& objectPosition = object3D->GetGraphicsData().mPosition;
    //     const Vector& cameraPosition = mCullingCamera.mPosition;
    //     const Float32 distSqr = Vector::SqrDistance(objectPosition, cameraPosition);
    //     if (distSqr > renderDistanceSqr)
    //     {
    //         continue;
    //     }
    // 
    //     if (object3D->IsTransparent())
    //     {
    //         mRenderSet.mTransparent.push_back(object3D);
    //     }
    //     else
    //     {
    //         mRenderSet.mOpaque.push_back(object3D);
    //     }
    // }

    // auto objects = mScene->GetObjects();
    // auto renderedObjects = CullDistanceAndFrustum(objects);
    // CommitBuffers(renderedObjects);
    // SetRenderSet(renderedObjects);
}

void GameRenderer::OnUpdate()
{
}

GfxPipelineStateAtomicPtr GameRenderer::GetDebugShader(DebugShaderType type)
{
    return mDebugShaders[EnumValue(type)];
}
GfxTextureAtomicPtr GameRenderer::GetDebugTexture(DebugTextureType type)
{
    return mDebugTextures[type];
}

void GameRenderer::CommitAndRelease()
{
    CollectGarbage();
    CommitNewObjects();
}

void GameRenderer::CollectGarbage()
{
    ScopeLock lock(mObjectsLock);
    for (auto it = mObjects.begin(); it != mObjects.end();)
    {
        if (it->GetStrongRefs() == 1)
        {
            it = mObjects.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

void GameRenderer::CommitNewObjects()
{
    ScopeLock objectsLock(mObjectsLock);
    ScopeLock newObjectsLock(mNewObjectsLock);

    for (auto it = mNewObjects.begin(); it != mNewObjects.end(); ++it)
    {
        if (it->GetStrongRefs() > 1)
        {
            mObjects.insert(*it);
        }
    }

    mNewObjects.clear();
}

} // namespace lf
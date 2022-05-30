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
#include "AbstractEngine/Gfx/GfxRenderer.h"
#include "Core/Math/Vector.h"
#include "Core/Math/Quaternion.h"
#include "Core/Concurrent/Task.h"
#include "Core/Utility/StdUnorderedSet.h"
#include "AbstractEngine/Gfx/GfxTypes.h"

// temp
#include "Core/Math/Vector4.h"
#include "AbstractEngine/Gfx/GfxVertexBuffer.h"
#include "AbstractEngine/Gfx/GfxIndexBuffer.h"
#include "AbstractEngine/Gfx/GfxPipelineState.h"
#include "Runtime/Asset/AssetReferenceTypes.h"
#include "AbstractEngine/Gfx/GfxTexture.h"

namespace lf {
DECLARE_ATOMIC_PTR(GfxCommandContext);
DECLARE_ATOMIC_PTR(GfxCommandQueue);
DECLARE_ATOMIC_PTR(GfxSwapChain);
DECLARE_ATOMIC_PTR(GfxFence);
DECLARE_ATOMIC_PTR(GfxModelRenderer);

// temp
DECLARE_ATOMIC_PTR(GfxVertexBuffer);
DECLARE_ATOMIC_PTR(GfxIndexBuffer);
DECLARE_ATOMIC_PTR(GfxPipelineState);
DECLARE_ATOMIC_PTR(GfxRenderTexture);
DECLARE_ASSET(GfxTextureBinary);
DECLARE_ATOMIC_PTR(GfxTexture);
DECLARE_ASSET(GfxInputLayout);



class LF_ENGINE_API GameRenderer : public GfxRenderer
{
    DECLARE_CLASS(GameRenderer, GfxRenderer);
    using ModelRendererSet = TUnorderedSet<GfxModelRendererAtomicPtr, std::hash<const GfxModelRenderer*>>;
public:
    GameRenderer();
    ~GameRenderer() override;

    bool Initialize(GfxDependencyContext& context) override;
    void Shutdown() override;
    GfxModelRendererAtomicPtr CreateModelRendererOfType(const Type* type) override;
    GfxDevice& Device() override;

    void SetWindow(GfxSwapChain* outputTarget);

    void SetAssetProvider(const DebugAssetProviderPtr& provider) { mAssets = provider; }
    void SetupResource(GfxDevice& device, GfxCommandContext& context) override;
    void OnRender(GfxDevice& device, GfxCommandContext& context) override;
    void OnBeginFrame() override;
    void OnEndFrame() override;
    void OnUpdate() override;

    GfxPipelineStateAtomicPtr GetDebugShader(DebugShaderType type) override;
    GfxTextureAtomicPtr GetDebugTexture(DebugTextureType type) override;

private:
    GfxDevice* mDevice;
    GfxSwapChainAtomicPtr mOutputTarget;
    GfxCommandQueueAtomicPtr mCommandQueue;

    // Renderer Objects
    // **********************************
    // This method should be called once per frame to collect the garbage entities
    // and accept the new entities.
    // 
    // note: If this method is not called CreateModelRenderer methods will not work.
    // **********************************
    void CommitAndRelease();
    void CollectGarbage();
    void CommitNewObjects();
    ModelRendererSet mObjects;
    ModelRendererSet mNewObjects;
    SpinLock mNewObjectsLock;
    SpinLock mObjectsLock;
    // !Renderer Objects

    struct CameraData
    {
        Vector mPosition;
        Quaternion mRotation;

        Float32 mRenderDistance;
    };

    CameraData mRenderCamera;
    CameraData mCullingCamera;

    DebugAssetProviderPtr mAssets;

    static void CreateInputLayout(DebugShaderType type, TStackVector<Gfx::VertexInputElement, 8>& inputLayout);
    enum class DebugResourceState : Int32 
    {
        None,
        Created,
        Committed
    };
    DebugResourceState        mDebugResourceState;
    GfxPipelineStateAtomicPtr mDebugShaders[ENUM_SIZE_EX(DebugShaderType)];
    GfxTextureAtomicPtr       mDebugTextures[ENUM_SIZE_EX(DebugTextureType)];

    GfxRenderTextureAtomicPtr mTestRenderTexture[Gfx::FrameCount::Value];
    GfxPipelineStateAtomicPtr mTestPSO;
    GfxVertexBufferAtomicPtr  mTestVBO;
    GfxIndexBufferAtomicPtr   mTestIBO;
    String                    mTestShaderText;
    GfxInputLayoutAsset       mTestInputLayout;
};

}
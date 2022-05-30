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
#include "Core/Concurrent/Task.h"
#include "Core/Utility/StdMap.h"
#include "Game/GraphicsApp/GraphicsServiceBase.h"

#include "AbstractEngine/World/World.h"
#include "AbstractEngine/World/WorldScene.h"
#include "AbstractEngine/World/Entity.h"

namespace lf {
    DECLARE_ATOMIC_PTR(AppWindow);
    DECLARE_ATOMIC_PTR(InputBinding);
    DECLARE_ATOMIC_PTR(GfxSwapChain);
    DECLARE_ATOMIC_PTR(GfxTexture);
    DECLARE_ATOMIC_PTR(GameRenderer);
    DECLARE_ATOMIC_PTR(GfxPipelineState);
    DECLARE_ATOMIC_PTR(GfxStaticModelRenderer);
    DECLARE_ATOMIC_WPTR(Entity);
    DECLARE_ATOMIC_WPTR(WorldScene);
    DECLARE_PTR(DX12GfxDevice);
    DECLARE_ASSET(GfxTextureBinary);
    class AppService;
    class InputMgr;
    class World;

    // Run with -app /type=GraphicsAppBase /tutorial=Texturing
    class Graphics_Texturing : public GraphicsServiceBase
    {
        DECLARE_CLASS(Graphics_Texturing, GraphicsServiceBase);
    public:
        Graphics_Texturing();
        ~Graphics_Texturing();

        APIResult<ServiceResult::Value> OnStart() override;
        // APIResult<ServiceResult::Value> OnTryInitialize() override;
        APIResult<ServiceResult::Value> OnPostInitialize() override;
        // APIResult<ServiceResult::Value> OnBeginFrame() override;
        APIResult<ServiceResult::Value> OnEndFrame() override;
        APIResult<ServiceResult::Value> OnFrameUpdate() override;
        APIResult<ServiceResult::Value> OnShutdown(ServiceShutdownMode mode) override;

    private:
        void RegisterEntities();
        void CreateRenderer();
        void CreateEntities();
        void TestButton();

        AppService* mAppService; // Application to create window with/quit on requested.
        DX12GfxDevice* mGfxDevice;  // Gfx Device to create gfx resource with
        InputMgr* mInputMgr;   // InputMgr to listen for input events
        World*    mWorld;
        InputBindingAtomicPtr   mQuitBinding;// Input binding to quit the app
        AppWindowAtomicPtr      mWindow;     // Apps window
        GfxSwapChainAtomicPtr   mSwapChain;  // 
        GameRendererAtomicPtr   mRenderer;
        GfxTextureBinaryAsset   mTextureBinary;
        Task<GfxTextureAtomicPtr> mTextureTask;

        TStackVector<GfxTextureAtomicPtr, 100> mTextures;

        WorldSceneAtomicWPtr mTargetScene;

        TMap<Token, EntityDefinition> mEntityTypes;

        EntityAtomicWPtr mTestEntity;
    };

} // namespace lf
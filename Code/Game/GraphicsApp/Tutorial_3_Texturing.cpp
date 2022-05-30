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
#include "Tutorial_3_Texturing.h"
#include "Core/Input/InputBinding.h"
#include "Core/Input/InputMapping.h"
#include "AbstractEngine/App/AppWindow.h"
#include "AbstractEngine/App/AppService.h"
#include "AbstractEngine/Input/InputMgr.h"
#include "AbstractEngine/Gfx/GfxSwapChain.h"
#include "AbstractEngine/Gfx/GfxTexture.h"
#include "AbstractEngine/Gfx/GfxTextureBinary.h"

#include "Engine/DX12/DX12GfxDevice.h"
#include "Engine/Gfx/GameRenderer.h"

// Component Systems...
#include "Engine/Gfx/ComponentSystem/GfxModelRenderSetupComponentSystem.h"
#include "Engine/Gfx/ComponentSystem/MeshSetupComponentSystem.h"
// Component Types...
#include "Engine/World/ComponentTypes/WorldDataComponent.h"
#include "Engine/Gfx/ComponentTypes/ProceduralMeshComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshRendererComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshRendererFlagsComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshSimpleComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshTextureComponent.h"
#include "Engine/Gfx/ComponentTypes/MeshStandardComponent.h"


namespace lf {

    DEFINE_CLASS(lf::Graphics_Texturing) { NO_REFLECTION; }

    

    Graphics_Texturing::Graphics_Texturing()
        : Super()
        , mAppService(nullptr)
        , mGfxDevice(nullptr)
        , mInputMgr(nullptr)
        , mQuitBinding()
        , mWindow()
        , mSwapChain()
        , mRenderer()
        , mTextureBinary()
        , mTargetScene()
    {

    }
    Graphics_Texturing::~Graphics_Texturing()
    {

    }

    APIResult<ServiceResult::Value> Graphics_Texturing::OnStart()
    {
        auto result = Super::OnStart();
        if (result == ServiceResult::SERVICE_RESULT_FAILED)
        {
            return result;
        }

        mAppService = GetServices()->GetService<AppService>();
        mGfxDevice = GetServices()->GetService<DX12GfxDevice>();
        mInputMgr = GetServices()->GetService<InputMgr>();
        mWorld = GetServices()->GetService<World>();

        if (!mAppService || !mGfxDevice || !mInputMgr || !mWorld)
        {
            return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_FAILED);
        }

        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }

    APIResult<ServiceResult::Value> Graphics_Texturing::OnPostInitialize()
    {
        auto result = Super::OnPostInitialize();
        if (result == ServiceResult::SERVICE_RESULT_FAILED)
        {
            return result;
        }

        mWindow = mAppService->MakeWindow("MainWindow", "Tutorial 2 Texturing", 640, 640);
        mSwapChain = mGfxDevice->CreateSwapChain(mWindow);
        mWindow->Show();

        // mTextureTask = mGfxDevice->CreateResourceAsync<GfxTexture>();

        RegisterEntities();
        CreateRenderer();
        CreateEntities();

        String text = GetShaderText("Engine//Test/Shaders/BasicShader.shader");

        MemoryBuffer vertexByteCode;
        MemoryBuffer pixelByteCode;

        bool success = GetShaderBinary(Gfx::ShaderType::VERTEX, text, { Token("LF_VERTEX") }, vertexByteCode);
        Assert(success);
        success = GetShaderBinary(Gfx::ShaderType::PIXEL, text, { Token("LF_PIXEL") }, pixelByteCode);
        Assert(success);

        GfxModelRenderSetupComponentSystem* renderSetupSystem = mWorld->GetSystem<GfxModelRenderSetupComponentSystem>();
        // renderSetupSystem->SetDebugByteCode(vertexByteCode, pixelByteCode); // TODO: The system can use GameRenderer debug resources
        renderSetupSystem->SetGameRenderer(mRenderer);

        MeshSetupComponentSystem* meshSetupSystem = mWorld->GetSystem<MeshSetupComponentSystem>();
        meshSetupSystem->SetGameRenderer(mRenderer);

        mTextureBinary = GetTexture("Engine//Test/Textures/sand.png");
        Assert(mTextureBinary.IsLoaded());


        const Token GAME_FILTER("Game");
        InputMapping testKey(Token("Test"), GAME_FILTER);
        testKey.Register(InputBindingData(InputEventType::BUTTON_PRESSED, InputType::BINARY, InputDeviceType::KEYBOARD, InputCode::T));
        auto testKeyBinding = MakeConvertibleAtomicPtr<InputBinding>();
        testKeyBinding->InitializeAction(GAME_FILTER);
        testKeyBinding->CreateAction(testKey.GetPrimary(InputDeviceType::KEYBOARD));
        testKeyBinding->OnEvent([this](const InputEvent&) { TestButton(); });
        mInputMgr->RegisterBinding(testKey.GetName(), testKey.GetScope(), testKeyBinding);
        mInputMgr->PushInputFilter(GAME_FILTER);
        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }

    APIResult<ServiceResult::Value> Graphics_Texturing::OnEndFrame()
    {
        auto result = Super::OnEndFrame();
        if (result == ServiceResult::SERVICE_RESULT_FAILED)
        {
            return result;
        }

        if (!mWindow || !mWindow->IsOpen())
        {
            mGfxDevice->Unregister(mRenderer);
            mRenderer = NULL_PTR;

            mTargetScene = NULL_PTR;
            
            mAppService->Stop();
            mSwapChain = NULL_PTR;
        }

        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }

    APIResult<ServiceResult::Value> Graphics_Texturing::OnFrameUpdate()
    {
        auto result = Super::OnFrameUpdate();
        if (result == ServiceResult::SERVICE_RESULT_FAILED)
        {
            return result;
        }

        /*
        if (!mTextureTask.IsRunning() && !mTextureTask.IsComplete())
        {
            if (mTextures.size() < 100)
            {
                mTextureTask = mGfxDevice->CreateResource<GfxTexture>();
            }
            else if(mTextures.size() == 100)
            {
                for (SizeT i = 0; i < mTextures.size(); ++i)
                {
                    mTextures[i] = NULL_PTR;
                }
            }
        }
        else if(mTextureTask.IsComplete())
        {
            auto texture = mTextureTask.ResultValue();
            mTextureTask = Task<GfxTextureAtomicPtr>();

            if (!mTextureBinary)
            {
                Timer loadTimer;
                loadTimer.Start();
                mTextureBinary = GetTexture("Engine//Test/Textures/sand.png");
                loadTimer.Stop();

                gSysLog.Info(LogMessage("Loaded ") << mTextureBinary.GetPath().CStr() << " in " << ToMicroseconds(TimeTypes::Seconds(loadTimer.GetDelta())).mValue << " us");
            }

            texture->SetBinary(mTextureBinary);
            texture->SetResourceFormat(Gfx::ResourceFormat::R32G32B32A32_FLOAT);
            texture->Commit();

            const SizeT size = mTextures.size();
            gSysLog.Info(LogMessage("Create texture ") << size << " @ " << LogPtr(texture.AsPtr()));
            mTextures.push_back(texture);
        }
        */
        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
    }

    APIResult<ServiceResult::Value> Graphics_Texturing::OnShutdown(ServiceShutdownMode mode)
    {
        auto result = Super::OnShutdown(mode);

        return result;
    }

    void Graphics_Texturing::RegisterEntities()
    {
        mEntityTypes[Token("TestEntity")].SetComponentTypes({
                typeof(MeshRendererFlagsComponent),
                typeof(MeshRendererComponent),
                typeof(MeshSimpleComponent),
                typeof(WorldDataComponent)
            });

        mEntityTypes[Token("SimpleEntity")].SetComponentTypes({
                typeof(MeshRendererFlagsComponent),
                typeof(MeshRendererComponent),
                typeof(MeshSimpleComponent),
                typeof(WorldDataComponent)
            });

        mEntityTypes[Token("TextureEntity")].SetComponentTypes({
                typeof(MeshRendererFlagsComponent),
                typeof(MeshRendererComponent),
                typeof(MeshTextureComponent),
                typeof(WorldDataComponent)
            });

        mEntityTypes[Token("StandardEntity")].SetComponentTypes({
                typeof(MeshRendererFlagsComponent),
                typeof(MeshRendererComponent),
                typeof(MeshStandardComponent),
                typeof(WorldDataComponent)
            });

        for (auto& pair : mEntityTypes)
        {
            mWorld->RegisterStaticEntityDefinition(&pair.second);
        }
    }


    void Graphics_Texturing::CreateRenderer()
    {
        mRenderer = MakeConvertibleAtomicPtr<GameRenderer>();
        mRenderer->SetAssetProvider(CreateDebugAssetProvider());
        // As soon as the renderer is hooked it up, calls are made to it, to render.
        mGfxDevice->Register(mRenderer);
        mRenderer->SetWindow(mSwapChain);
    }

    void Graphics_Texturing::CreateEntities()
    {
        /*
        mTestEntity = mWorld->CreateEntity(&mEntityTypes[Token("TestEntity")]);

        if (TAtomicStrongPointer<Entity> entity = mTestEntity)
        {
            MeshSimpleComponentData* mesh = entity->GetComponent<MeshSimpleComponent>();

            auto& vertices = mesh->mVertices;
            vertices.push_back({ {0.0f, 0.5f, 0.0f, 1.0f}}); // top
            vertices.push_back({ {0.5f, -0.5f, 0.0f, 1.0f}}); // bottom right
            vertices.push_back({ {-0.5f, -0.5f, 0.0f, 1.0f}}); // bottom left

            auto& indices = mesh->mIndices;
            indices.push_back(0);
            indices.push_back(1);
            indices.push_back(2);

            MeshRendererFlagsComponentData* flags = entity->GetComponent<MeshRendererFlagsComponent>();
            flags->Set(MeshRendererFlags::DirtyBuffers);

            WorldDataComponentData* worldScene = entity->GetComponent<WorldDataComponent>();
            worldScene->mScene = mTargetScene;
        }
        */

        mTestEntity = mWorld->CreateEntity(&mEntityTypes[Token("TextureEntity")]);
        if (TAtomicStrongPointer<Entity> entity = mTestEntity)
        {
            MeshTextureComponentData* mesh = entity->GetComponent<MeshTextureComponent>();

            auto& vertices = mesh->mVertices;
            vertices.push_back({ {0.0f, 0.5f, 0.0f, 1.0f}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.5f, 0.0f} }); // top
            vertices.push_back({ {0.5f, -0.5f, 0.0f, 1.0f}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 1.0f, 1.0f} }); // bottom right
            vertices.push_back({ {-0.5f, -0.5f, 0.0f, 1.0f}, { 1.0f, 1.0f, 1.0f, 1.0f }, { 0.0f, 1.0f} }); // bottom left

            auto& indices = mesh->mIndices;
            indices.push_back(0);
            indices.push_back(1);
            indices.push_back(2);

            MeshRendererFlagsComponentData* flags = entity->GetComponent<MeshRendererFlagsComponent>();
            flags->Set(MeshRendererFlags::DirtyBuffers | MeshRendererFlags::DirtyTexture);
        }

    }

    void Graphics_Texturing::TestButton()
    {
        /*
        if (TAtomicStrongPointer<Entity> entity = mTestEntity)
        {
            ProceduralMeshComponentData* procedural = entity->GetComponent<ProceduralMeshComponent>();
            
            if (procedural->mTexture)
            {
                procedural->mTexture = NULL_PTR;
            }
            else
            {
                procedural->mTexture = GetTexture("Engine//Test/Textures/sand.png");
            }
            procedural->mDirtyTexture = true;
        }
        */
    }


} // namespace lf
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
#include "Engine/PCH.h"
#include "GameApp.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Utility/Log.h"
#include "Runtime/Async/Async.h"
#include "Runtime/Asset/DefaultAssetProcessor.h"
#include "AbstractEngine/App/AppService.h"
#include "AbstractEngine/Gfx/GfxDevice.h"
#include "AbstractEngine/Gfx/GfxMaterialProcessor.h"
#include "AbstractEngine/Gfx/GfxShaderTextProcessor.h"
#include "AbstractEngine/Gfx/GfxShaderBinaryProcessor.h"
#include "AbstractEngine/Gfx/GfxTextureBinaryProcessor.h"
#include "AbstractEngine/World/World.h"
#include "AbstractEngine/Input/InputMgr.h"


namespace lf
{

#if defined(LF_ERROR_EXCEPTIONS)
#define LF_PROTECTED_CALL(Code_, Catch_) try { Code_ } catch(::lf::Exception&) { Catch_ }
#else
#define LF_PROTECTED_CALL(Code_, Catch_) Code_
#endif

static TVector<const Type*> GetScopedServiceTypes()
{
    TVector<const Type*> types;
    types.push_back(typeof(AppService));
    types.push_back(typeof(GfxDevice));
    types.push_back(typeof(World));
    types.push_back(typeof(InputMgr));
    types.push_back(typeof(GfxDevice));
    return types;
}

DEFINE_ABSTRACT_CLASS(lf::GameApp) { NO_REFLECTION; }

GameApp::GameApp()
: mServices(GetScopedServiceTypes())
, mState(APP_START)
, mAppService(nullptr)
, mGfxService(nullptr)
, mAssetMgr()
, mAssetMgrInitialized(false)
{}

GameApp::~GameApp()
{
}
void GameApp::OnStart()
{
    mServices.SetConfig(GetConfig());
    mAssetMgr.SetGlobal();
    bool registered = RegisterServices() == ServiceResult::SERVICE_RESULT_SUCCESS;
    if (!registered)
    {
        return;
    }
    mAppService = static_cast<AppService*>(GetServices().GetService(typeof(AppService)).GetItem());
    if (!mAppService)
    {
        gSysLog.Error(LogMessage("Cannot start GameApp because there is no AppService. Make sure to register an AppService!"));
        return;
    }
    mGfxService = static_cast<GfxDevice*>(GetServices().GetService(typeof(GfxDevice)).GetItem());

    mState = APP_START;

    while (mState != APP_COMPLETE)
    {
        switch (mState)
        {
        case APP_START:
            LF_PROTECTED_CALL(Start();, HandleError(););
            break;
        case APP_INIT_RUNTIME_DEPS:
            LF_PROTECTED_CALL(InitializeRuntimeDependencies(); , HandleError(););
            break;
        case APP_INIT_LOOP:
            LF_PROTECTED_CALL(InitializeLoop(); , HandleError(););
            break;
        case APP_POST_INIT:
            LF_PROTECTED_CALL(PostInitialize(); , HandleError(););
            break;
        case APP_RUN:
            LF_PROTECTED_CALL(Run(); , HandleError(););
            break;
        case APP_SHUTDOWN_RUNTIME_DEPS:
        case APP_SHUTDOWN_RUNTIME_DEPS_ERROR:
        case APP_SHUTDOWN_RUNTIME_DEPS_CRITICAL_ERROR:
            LF_PROTECTED_CALL(ShutdownRuntimeDependencies(); , HandleError(););
            break;
        case APP_SHUTDOWN:
        case APP_SHUTDOWN_ERROR:
        case APP_SHUTDOWN_CRITICAL_ERROR:
            LF_PROTECTED_CALL(Shutdown(); , HandleError(););
            break;
        }
    }

    mServices.Clear();
}
void GameApp::OnExit()
{
    if (mAssetMgrInitialized)
    {
        mAssetMgr.Shutdown();
        mAssetMgrInitialized = false;
    }
}

void GameApp::Start()
{
    mState = APP_INIT_RUNTIME_DEPS;
    if (mServices.Start() != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        mState = APP_SHUTDOWN_ERROR;
    }
}
void GameApp::InitializeRuntimeDependencies()
{
    mState = APP_INIT_LOOP;

    // Initialize the asset mgr
    Assert(GetConfig() != nullptr);
    String projectDir = GetConfig()->GetProjectDirectory();
    String cacheDir = GetConfig()->GetCacheDirectory();
    
    AssetMgrInitializeData initData;
    initData.mProcessors.push_back(AssetProcessorPtr(LFNew<DefaultAssetProcessor>()));
    initData.mProcessors.push_back(AssetProcessorPtr(LFNew<GfxShaderTextProcessor>()));
    initData.mProcessors.push_back(AssetProcessorPtr(LFNew<GfxTextureBinaryProcessor>(Gfx::TextureFileFormat::PNG)));
    if (mGfxService)
    {
        initData.mProcessors.push_back(AssetProcessorPtr(LFNew<GfxShaderBinaryProcessor>()));
    }
    initData.mIsGlobal = true;
    Assert(mAssetMgr.Initialize(projectDir, cacheDir, true, &initData));
    gSysLog.Info(LogMessage("Initialized AssetMgr..."));
    mAssetMgrInitialized = true;

    GetAsync().EnableAppThread();
}
void GameApp::InitializeLoop()
{
    mState = APP_POST_INIT;
    SizeT loopCount = 20;
    for (SizeT i = 0; i < loopCount; ++i)
    {
        switch (mServices.TryInitialize())
        {
        case ServiceResult::SERVICE_RESULT_PENDING:
            break; // Keep trying.
        case ServiceResult::SERVICE_RESULT_SUCCESS:
            return; // All done!
        case ServiceResult::SERVICE_RESULT_FAILED:
        default:
            mState = APP_SHUTDOWN_RUNTIME_DEPS;
            return;
        }
    }
}
void GameApp::PostInitialize()
{
    mState = APP_RUN;
    if (mServices.PostInitialize() != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        mState = APP_SHUTDOWN_RUNTIME_DEPS;
    }
}
void GameApp::Run()
{
    while (mAppService->IsRunning())
    {
        if (mServices.BeginFrame() != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = APP_SHUTDOWN_RUNTIME_DEPS_ERROR;
            return;
        }

        if (mServices.FrameUpdate() != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = APP_SHUTDOWN_RUNTIME_DEPS_ERROR;
            return;
        }
        
        if (mServices.EndFrame() != ServiceResult::SERVICE_RESULT_SUCCESS) 
        {
            mState = APP_SHUTDOWN_RUNTIME_DEPS_ERROR;
            return;
        }
        mAssetMgr.Update();
    }
    mState = APP_SHUTDOWN_RUNTIME_DEPS;
}

void GameApp::ShutdownRuntimeDependencies()
{
    if (GetAsync().AppThreadRunning())
    {
        GetAsync().DisableAppThread();
    }

    switch (mState)
    {
        case APP_SHUTDOWN_RUNTIME_DEPS: mState = APP_SHUTDOWN; return;
        case APP_SHUTDOWN_RUNTIME_DEPS_ERROR: mState = APP_SHUTDOWN_ERROR; return;
        case APP_SHUTDOWN_RUNTIME_DEPS_CRITICAL_ERROR: mState = APP_SHUTDOWN_CRITICAL_ERROR; return;
        default:
            CriticalAssertMsg("Invalid app state!");
            break;
    }
}

void GameApp::Shutdown()
{
    mAppService = nullptr;
    switch (mState)
    {
        case APP_SHUTDOWN: mServices.Shutdown(ServiceShutdownMode::SHUTDOWN_NORMAL); break;
        case APP_SHUTDOWN_ERROR: mServices.Shutdown(ServiceShutdownMode::SHUTDOWN_GRACEFUL); break;
        case APP_SHUTDOWN_CRITICAL_ERROR:
        default:
            mServices.Shutdown(ServiceShutdownMode::SHUTDOWN_FAST);
            break;
    }
    mState = APP_COMPLETE;
}
void GameApp::HandleError()
{
    // Throwing an exception while shutting down should just complete the application.
    switch (mState)
    {
    case APP_SHUTDOWN_RUNTIME_DEPS:
    case APP_SHUTDOWN_RUNTIME_DEPS_ERROR:
    case APP_SHUTDOWN_RUNTIME_DEPS_CRITICAL_ERROR:
        mState = APP_SHUTDOWN_CRITICAL_ERROR;
        break;
    case APP_SHUTDOWN:
    case APP_SHUTDOWN_ERROR:
    case APP_SHUTDOWN_CRITICAL_ERROR:
        mState = APP_COMPLETE;
        break;
    default:
        mState = APP_SHUTDOWN_RUNTIME_DEPS_CRITICAL_ERROR;
        break;
    }
}

} // namespace lf
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
#include "Engine/App/Application.h"
#include "Runtime/Asset/AssetMgr.h"
#include "Runtime/Service/Service.h"

namespace lf
{

class AppService;
class GfxDevice;

//
// Override the 'RegisterServices' function to register the services the app will use.
//
// World (Service)
//   WorldScene[]
//   ComponentSystem[]
//   
// Gfx
//   GfxScene[]
// 
// Physics
//   PhysicsScene[] 
// 
// Audio
//   AudioScene[]
//
// 
class LF_ENGINE_API GameApp : public Application
{
    DECLARE_CLASS(GameApp, Application);
public:
    GameApp();
    ~GameApp();

    void OnStart() final;
    void OnExit() final;

    const ServiceContainer& GetServices() const { return mServices; }
protected:
    ServiceContainer& GetServices() { return mServices; }
    template<typename T>
    static TStrongPointer<T> MakeService()
    {
        TStrongPointer<T> service(LFNew<T>());
        service->SetType(typeof(T));
        return service;
    }

    virtual ServiceResult::Value RegisterServices() = 0;
    void Start();
    void InitializeRuntimeDependencies();
    void InitializeLoop();
    void PostInitialize();
    void Run();
    void ShutdownRuntimeDependencies();
    void Shutdown();

private:
    enum AppState
    {
        APP_START,
        APP_INIT_RUNTIME_DEPS,
        APP_INIT_LOOP,
        APP_POST_INIT,
        APP_RUN,
        APP_SHUTDOWN_RUNTIME_DEPS,
        APP_SHUTDOWN_RUNTIME_DEPS_ERROR,
        APP_SHUTDOWN_RUNTIME_DEPS_CRITICAL_ERROR,
        APP_SHUTDOWN,
        APP_SHUTDOWN_ERROR,
        APP_SHUTDOWN_CRITICAL_ERROR,
        APP_COMPLETE
    };

    void HandleError();

    ServiceContainer mServices;
    AppState         mState;

    AppService*      mAppService;
    GfxDevice*       mGfxService;

    AssetMgr         mAssetMgr;
    bool             mAssetMgrInitialized;

};

} // namespace lf

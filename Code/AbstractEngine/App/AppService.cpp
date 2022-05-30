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
#include "AbstractEngine/PCH.h"
#include "AppService.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/Thread.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Utility.h"
#include "Runtime/Async/Async.h"

#if defined(LF_OS_WINDOWS)
#include "AbstractEngine/App/Win32Window.h"
#endif

namespace lf
{
class InputMgr;
DEFINE_ABSTRACT_CLASS(lf::AppService) { NO_REFLECTION; }

static AppWindowAtomicPtr CreatePlatformWindow(const AppWindowDesc& desc, const ServiceContainer& services)
{
#if defined(LF_OS_WINDOWS)
    auto window = MakeConvertibleAtomicPtr<Win32Window>();
    window->SetType(typeof(Win32Window));
    window->InitDependencies(services);
    if (!window->Create(desc))
    {
        return  AppWindowAtomicPtr();
    }
    return window;
#else
    #error "Missing platform implementation..."
#endif

}

AppService::AppService()
: mRunning(0)
, mAppTimer()
, mFrameTimer()
, mLastFrameDelta(0.016f)
, mActualLastFrameDelta(0.016f)
{
    mAppTimer.Start();
}
AppService::~AppService()
{}

void AppService::SetRunning()
{
    if (AtomicLoad(&mRunning) < 0)
    {
        AssertMsg("AppService cannot be set to run if it has been explicitly stopped.");
    }
    else
    {
        AtomicStore(&mRunning, 1);
    }
}

void AppService::Stop()
{
    AtomicStore(&mRunning, -1);
}
bool AppService::IsRunning() const
{
    return AtomicLoad(&mRunning) > 0;
}

Float32 AppService::GetFrameDelta() const
{
    return static_cast<Float32>(mFrameTimer.PeekDelta());
}

Float32 AppService::GetAppTime() const
{
    return static_cast<Float32>(mAppTimer.PeekDelta());
}

void AppService::SaveConfig()
{
    gSysLog.Info(LogMessage("Saving app config ") << mAppConfigPath);
    mAppConfig.Write(mAppConfigPath);
}

AppWindowAtomicPtr AppService::MakeWindow(const String& id, const String& title, SizeT width, SizeT height)
{
    Assert(GetAsync().GetAppThreadId() == APP_THREAD_ID_MAIN);
    
    auto it = std::find_if(mWindows.begin(), mWindows.end(), [id](const AppWindowAtomicPtr& window)
        {
            return window->GetID() == id;
        });
    if (it != mWindows.end())
    {
        return AppWindowAtomicPtr();
    }

    AppWindowDesc desc;
    desc.mID = id;
    desc.mTitle = title;
    desc.mWidth = width;
    desc.mHeight = height;
    desc.mDefaultHidden = false;

    AppWindowAtomicPtr window = CreatePlatformWindow(desc, *GetServices());
    if (!window)
    {
        return AppWindowAtomicPtr();
    }
    mWindows.push_back(window);
    return window;
}

APIResult<ServiceResult::Value> AppService::OnStart()
{
    auto superResult = Super::OnStart();
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }

    mDispatcher = ThreadDispatcherPtr(LFNew<ThreadDispatcher>());
    AppThreadAttributes mainThreadAttribs;
    mainThreadAttribs.mDispatcher = mDispatcher;
    Assert(GetAsync().StartThread(APP_THREAD_ID_MAIN, AppThreadCallback(), mainThreadAttribs));

    auto engineConfig = GetServices()->GetConfig();
    if (engineConfig && !engineConfig->GetAppConfig().Empty())
    {
        mAppConfigPath = FileSystem::PathResolve(engineConfig->GetAppConfig());
        gSysLog.Info(LogMessage("Loading app config ") << mAppConfigPath);
        if (!mAppConfig.Read(mAppConfigPath))
        {
            mAppConfig.Write(mAppConfigPath);
        }
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> AppService::OnBeginFrame()
{
    auto superResult = Super::OnBeginFrame();
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }
    mFrameTimer.Start();

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}
APIResult<ServiceResult::Value> AppService::OnEndFrame()
{
    auto superResult = Super::OnBeginFrame();
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }

    mDispatcher->Dispatch();

    Float32 targetFrameTime = (1.0f / 60.0f);
    mActualLastFrameDelta = static_cast<Float32>(mFrameTimer.PeekDelta());
    Float32 remaining = targetFrameTime - mActualLastFrameDelta;
    if (remaining > 0.0f)
    {
        SizeT sleepTime = static_cast<SizeT>(ToMicroseconds(TimeTypes::Seconds(remaining)).mValue);
        Thread::SleepPrecise(sleepTime);
    }

    mFrameTimer.Stop();
    mLastFrameDelta = static_cast<Float32>(mFrameTimer.GetDelta());

    Float32 warnFrameRate = (1.0f / 30.0f);
    Float32 errorFrameRate = (1.0f / 20.0f);
    if (mLastFrameDelta > errorFrameRate)
    {
        gSysLog.Error(LogMessage("Long Frame Delta=") << ToMilliseconds(TimeTypes::Seconds(mLastFrameDelta)).mValue << " (ms)");
    }
    else if (mLastFrameDelta > warnFrameRate)
    {
        gSysLog.Warning(LogMessage("Long Frame Delta=") << ToMilliseconds(TimeTypes::Seconds(mLastFrameDelta)).mValue << " (ms)");
    }
    // else
    // {
    //     gSysLog.Debug(LogMessage("Sleep Delta=") << ToMilliseconds(TimeTypes::Seconds(remaining)).mValue << " (ms)");
    //     gSysLog.Debug(LogMessage("Actual Delta=") << ToMilliseconds(TimeTypes::Seconds(mActualLastFrameDelta)).mValue << "(ms)");
    //     gSysLog.Debug(LogMessage("Frame Delta=") << ToMilliseconds(TimeTypes::Seconds(mLastFrameDelta)).mValue << " (ms)");
    // }


    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> AppService::OnFrameUpdate()
{
#if defined(LF_OS_WINDOWS)
    MSG msg;

    while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
#endif

    for (auto it = mWindows.begin(); it != mWindows.end();)
    {
        if (it->GetStrongRefs() == 1)
        {
            (*it)->Destroy();
            it = mWindows.swap_erase(it);
        }
        else
        {
            ++it;
        }
    }

    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

APIResult<ServiceResult::Value> AppService::OnShutdown(ServiceShutdownMode mode)
{
    if (mode == ServiceShutdownMode::SHUTDOWN_NORMAL)
    {
        SaveConfig();
    }

    GetAsync().StopThread(APP_THREAD_ID_MAIN);
    mDispatcher = NULL_PTR;

    auto superResult = Super::OnShutdown(mode);
    if (superResult != ServiceResult::SERVICE_RESULT_SUCCESS)
    {
        return superResult;
    }
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}

} // namespace lf
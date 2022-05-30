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
#include "Core/Utility/Time.h"
#include "Core/Reflection/DynamicCast.h"
#include "Runtime/Async/ThreadDispatcher.h"
#include "Runtime/Service/Service.h"
#include "AbstractEngine/App/AppConfig.h"
#include "AbstractEngine/App/AppWindow.h"

namespace lf
{

DECLARE_PTR(ThreadDispatcher);
DECLARE_ATOMIC_PTR(AppWindow);

class LF_ABSTRACT_ENGINE_API AppService : public Service
{
    DECLARE_CLASS(AppService, Service);
public:
    AppService();
    virtual ~AppService();


    void SetRunning();
    void Stop();
    bool IsRunning() const;

    Float32 GetLastFrameDelta() const { return mLastFrameDelta; }
    Float32 GetFrameDelta() const;
    Float32 GetAppTime() const;

    void SaveConfig();

    template<typename T>
    const T* GetConfigObject() const
    {
        return static_cast<const T*>(mAppConfig.GetConfig(typeof(T)));
    }

    AppWindowAtomicPtr MakeWindow(const String& id, const String& title, SizeT width, SizeT height);
    
protected:
    APIResult<ServiceResult::Value> OnStart() override;
    APIResult<ServiceResult::Value> OnBeginFrame() override;
    APIResult<ServiceResult::Value> OnEndFrame() override;
    APIResult<ServiceResult::Value> OnFrameUpdate() override;
    APIResult<ServiceResult::Value> OnShutdown(ServiceShutdownMode mode) override;
private:
    Atomic32 mRunning;

    Timer    mAppTimer;
    Timer    mFrameTimer;
    Float32  mLastFrameDelta;
    Float32  mActualLastFrameDelta;

    ThreadDispatcherPtr mDispatcher;

    String    mAppConfigPath;
    AppConfig mAppConfig;

    TVector<AppWindowAtomicPtr> mWindows;
};
}

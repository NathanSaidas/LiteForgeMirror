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
#include "Core/Common/Enum.h"
#include "Core/Reflection/Object.h"
#include "Runtime/Reflection/ReflectionTypes.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/APIResult.h"

namespace lf {

DECLARE_STRICT_ENUM(ServiceState,
UNINITIALIZED,
STARTED,
INITIALIZED,
POST_INITIALIZED,
RUNNING,
SHUTDOWN,
FAILED);

DECLARE_ENUM(ServiceResult,
SERVICE_RESULT_SUCCESS,
SERVICE_RESULT_FAILED,
SERVICE_RESULT_PENDING);

namespace ServiceResult
{
LF_INLINE ServiceResult::Value Combine(ServiceResult::Value left, ServiceResult::Value right)
{
    if (left == right)
    {
        return left;
    }

    if (left == SERVICE_RESULT_FAILED || right == SERVICE_RESULT_FAILED)
    {
        return SERVICE_RESULT_FAILED;
    }
    return SERVICE_RESULT_PENDING;
}
} // namespace ServiceResult

// Normal - Shutdown how you would normally, doing whatever you need to
// Graceful - A error occured, so cleanup resources quickly and shutdown
// Fast - A critical error occured, we might not have much time/memory just release absolute critical items.
DECLARE_STRICT_ENUM(ServiceShutdownMode,
SHUTDOWN_NORMAL,
SHUTDOWN_GRACEFUL,
SHUTDOWN_FAST);

class ServiceContainer;
class EngineConfig;

// See 'Overview -> Program Execution' to get a detailed overview of how program execution
// and lifetime is supposed to work.
//
// The service class aims to provide a way for manager level classes to interact with each
// other in a 'container' without using singletons. (Testable manager classes)
//
// The Application Start/Initialize/Run/Shutdown code will utilize services to provide
// a better way for these manager classes to interact with each other.
//
// A service has various methods to handle the different stages of a program.
// A service has a lifetime-state { Uninitialized, Started, PostInitialized, Running, 
// PreShutdown, Shutdown, Failed_xxx } 
// where Failed_xxx is the the state at which it failed at.
// A service has a 
//
// A service returns a result { Success, Failed, Pending } where pending means theres more 
// work to be done.
// 
// A pointer to a service remains valid for the life-time of the application (eg until after shutdown)
// 
// A service instance is limited to scope (eg, there can only be one input manager)
// 
// A service can interface with other services
// 
class LF_RUNTIME_API Service : public Object
{
    // We intentionally want to hide various functions, and ensure proper state control in the 'container'
    friend ServiceContainer;
    DECLARE_CLASS(Service, Object);
public:
    Service();
    virtual ~Service();

    LF_INLINE ServiceState GetServiceState() const { return mServiceState; }
protected:
    LF_INLINE const ServiceContainer* GetServices() const { return mServiceContainer; }
    // Service Callbacks:
    virtual APIResult<ServiceResult::Value> OnStart();
    virtual APIResult<ServiceResult::Value> OnTryInitialize();
    virtual APIResult<ServiceResult::Value> OnPostInitialize();
    virtual APIResult<ServiceResult::Value> OnBeginFrame();
    virtual APIResult<ServiceResult::Value> OnEndFrame();
    virtual APIResult<ServiceResult::Value> OnFrameUpdate();
    virtual APIResult<ServiceResult::Value> OnBlockingUpdate(Service* service);
    virtual APIResult<ServiceResult::Value> OnShutdown(ServiceShutdownMode mode);
private:
    ServiceState mServiceState;
    const ServiceContainer* mServiceContainer;
};


DECLARE_PTR(Service);
// The container can be passed around any object
// only provide query options
class LF_RUNTIME_API ServiceContainer
{
public:
    ServiceContainer(const TVector<const Type*>& scopedTypes, const EngineConfig* engineConfig = nullptr);
    ~ServiceContainer();

    APIResult<Service*> GetService(const Type* type) const;
    void SetConfig(const EngineConfig* value) { mEngineConfig = value; }
    const EngineConfig* GetConfig() const { return mEngineConfig; }

    template<typename T>
    T* GetService() const
    {
        LF_STATIC_IS_A(T, Service);
        return static_cast<T*>(GetService(typeof(T)).GetItem());
    }

    // TODO: Static Services
    // InputService* GetInputService() const { return mInput; }
    // InputService* mInputService;


    // ** Attempt to register a service (Must be called before 'Start')
    APIResult<bool> Register(const ServicePtr& service);
    // ** Call at end of frame to remove all services.
    void Clear();

    // ** Attempt to start all the services (set ServiceContainer )
    APIResult<ServiceResult::Value> Start();
    // ** Attempt to initialize services (gather dependencies, load stuff)
    APIResult<ServiceResult::Value> TryInitialize();
    // ** Finalize any initialization process
    APIResult<ServiceResult::Value> PostInitialize();
    // ** Updates, Signals the beginning of a frame (must call end frame after)
    APIResult<ServiceResult::Value> BeginFrame();
    // ** Updates, Signals the end of a frame (must call begin frame before)
    APIResult<ServiceResult::Value> EndFrame();
    // ** Updates, Updates all the services
    APIResult<ServiceResult::Value> FrameUpdate();
    // ** Allows one service to block a thread on a asynchronous task, updating other services.
    // while( Task->Running ) { GetServices()->BlockingUpdate(this); }
    APIResult<ServiceResult::Value> BlockingUpdate(Service* service) const;
    // ** Shutdown the service, releasing any resources. (Respect the shutdown mode)
    APIResult<ServiceResult::Value> Shutdown(ServiceShutdownMode mode);

    
protected:
    TVector<const Type*>         mScopedTypes;
    mutable TVector<ServicePtr>  mServices;
    mutable ServiceState        mState;
    bool                        mInFrame;
    mutable TVector<Service*>    mBlockingServiceStack;
    const EngineConfig*         mEngineConfig;
};

} // namespace lf
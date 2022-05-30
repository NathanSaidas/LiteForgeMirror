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
#include "Runtime/PCH.h"
#include "Service.h"
#include "Core/Reflection/Type.h"
#include "Core/Utility/Error.h"
#include "Core/Utility/Log.h"

namespace lf {

DEFINE_ABSTRACT_CLASS(lf::Service) { NO_REFLECTION; }

Service::Service()
: Super()
, mServiceState(ServiceState::UNINITIALIZED)
, mServiceContainer(nullptr)
{}
Service::~Service()
{
    mServiceContainer = nullptr;
}
APIResult<ServiceResult::Value> Service::OnStart() { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }
APIResult<ServiceResult::Value> Service::OnTryInitialize() { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }
APIResult<ServiceResult::Value> Service::OnPostInitialize() { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }
APIResult<ServiceResult::Value> Service::OnBeginFrame() { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }
APIResult<ServiceResult::Value> Service::OnEndFrame() { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }
APIResult<ServiceResult::Value> Service::OnFrameUpdate() { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }
APIResult<ServiceResult::Value> Service::OnBlockingUpdate(Service* ) { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }
APIResult<ServiceResult::Value> Service::OnShutdown(ServiceShutdownMode ) { return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS); }


ServiceContainer::ServiceContainer(const TVector<const Type*>& scopedTypes, const EngineConfig* engineConfig)
: mScopedTypes(scopedTypes)
, mServices()
, mState(ServiceState::UNINITIALIZED)
, mInFrame(false)
, mEngineConfig(engineConfig)
{}
ServiceContainer::~ServiceContainer()
{
    
}

APIResult<Service*> ServiceContainer::GetService(const Type* type) const
{
    if (type == nullptr)
    {
        return ReportError((Service*)nullptr, ArgumentNullError, "type");
    }

    if (!type->IsA(typeof(Service)))
    {
        return ReportError((Service*)nullptr, InvalidArgumentError, "type", "Type is not a service!");
    }

    for (Service* service : mServices)
    {
        if (service->GetType()->IsA(type))
        {
            return APIResult<Service*>(service);
        }
    }
    return APIResult<Service*>(nullptr);
}

APIResult<bool> ServiceContainer::Register(const ServicePtr& service)
{
    if (!service)
    {
        return ReportError(false, ArgumentNullError, "service");
    }

    const Type* serviceType = service->GetType();
    if (!serviceType)
    {
        return ReportError(false, InvalidArgumentError, "service", "Service is not initialized with reflection.");
    }

    if (mState != ServiceState::UNINITIALIZED)
    {
        return ReportError(false, OperationFailureError, "Cannot register a service while the container is initialized.", TServiceState::GetString(mState));
    }

    const Type* scopedType = nullptr;
    for (const Type* type : mScopedTypes)
    {
        if(serviceType->IsA(type))
        {
            scopedType = type;
            break;
        }
    }

    bool addScopedType = false;
    if (scopedType == nullptr)
    {
        LF_LOG_WARN(gSysLog, "Service type is not scoped. It will be scoped to its own type. Service=" << serviceType->GetFullName());
        scopedType = serviceType;
        addScopedType = true;
    }

    for (Service* current : mServices)
    {
        if (current->IsA(scopedType))
        {
            return ReportError(false, OperationFailureError, "Number of service instances are limited to scope.", serviceType->GetFullName().CStr());
        }
    }

    gSysLog.Info(LogMessage("Registering service ") << service->GetType()->GetFullName());
    mServices.push_back(service);
    if (addScopedType)
    {
        mScopedTypes.push_back(scopedType);
    }
    service->mServiceContainer = this;
    return APIResult<bool>(true);
}

void ServiceContainer::Clear()
{
    Assert(mState == ServiceState::SHUTDOWN);
    mServices.clear();
    mScopedTypes.clear();
    mBlockingServiceStack.clear();
    mInFrame = false;
    mState = ServiceState::UNINITIALIZED;
}


static APIResult<ServiceResult::Value> CatchPendingFailure( APIResult<ServiceResult::Value>&& result, const char* operation)
{
    return result == ServiceResult::SERVICE_RESULT_FAILED ? 
        std::move(result) 
        : ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Operation cannot return Pending", operation);
}

// ** Attempt to start all the services (set ServiceContainer )
APIResult<ServiceResult::Value> ServiceContainer::Start()
{
    if (mState != ServiceState::UNINITIALIZED)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot start service container, its already initialized.", TServiceState::GetString(mState));
    }

    mState = ServiceState::STARTED;
    // Try to start all the services... 
    for (Service* service : mServices)
    {
        gSysLog.Info(LogMessage("Starting service ") << service->GetType()->GetFullName());
        auto result = service->OnStart();
        // If any are failed or pending its an error!
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = ServiceState::FAILED;
            service->mServiceState = ServiceState::FAILED;
            return CatchPendingFailure(std::move(result), "Start");
        }
        service->mServiceState = ServiceState::STARTED;
    }
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}
// ** Attempt to initialize services (gather dependencies, load stuff)
APIResult<ServiceResult::Value> ServiceContainer::TryInitialize()
{
    if (mState != ServiceState::STARTED)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot try to initialize services, container is not started!", TServiceState::GetString(mState));
    }

    bool allSuccess = true;
    for (Service* service : mServices)
    {
        auto result = service->OnTryInitialize();
        if (result == ServiceResult::SERVICE_RESULT_FAILED)
        {
            mState = ServiceState::FAILED;
            service->mServiceState = ServiceState::FAILED;
            return result;
        }
        else if (result == ServiceResult::SERVICE_RESULT_PENDING)
        {
            allSuccess = false;
        }
        else
        {
            gSysLog.Info(LogMessage("Service ") << service->GetType()->GetFullName() << " initialized.");
            service->mServiceState = ServiceState::INITIALIZED;
        }
    }

    if (allSuccess)
    {
        mState = ServiceState::INITIALIZED;
    }

    return APIResult<ServiceResult::Value>(allSuccess ? ServiceResult::SERVICE_RESULT_SUCCESS : ServiceResult::SERVICE_RESULT_PENDING);
}
// ** Finalize any initialization process
APIResult<ServiceResult::Value> ServiceContainer::PostInitialize()
{
    if (mState != ServiceState::INITIALIZED)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot try to post-initialize services, container is not started!", TServiceState::GetString(mState));
    }

    for (Service* service : mServices)
    {
        gSysLog.Info(LogMessage("Post Initializing service ") << service->GetType()->GetFullName());
        auto result = service->OnPostInitialize();
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = ServiceState::FAILED;
            service->mServiceState = ServiceState::FAILED;
            return CatchPendingFailure(std::move(result), "PostInitialize");;
        }
        service->mServiceState = ServiceState::POST_INITIALIZED;
    }
    mState = ServiceState::POST_INITIALIZED;
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);

}
// ** Updates, Signals the beginning of a frame (must call end frame after)
APIResult<ServiceResult::Value> ServiceContainer::BeginFrame()
{
    if (mState != ServiceState::RUNNING && mState != ServiceState::POST_INITIALIZED)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot BeginFrame, container is not started!", TServiceState::GetString(mState));
    }
    mState = ServiceState::RUNNING;
    if (mInFrame)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot begin frame while in the middle of a frame. (Missing EndFrame call?)", "<NONE>");
    }
    mInFrame = true;

    APIResult<ServiceResult::Value> failure(ServiceResult::SERVICE_RESULT_SUCCESS);
    for (Service* service : mServices)
    {
        service->mServiceState = ServiceState::RUNNING;
        auto result = service->OnBeginFrame();
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = ServiceState::FAILED;
            service->mServiceState = ServiceState::FAILED;
            failure.Report();
            failure = CatchPendingFailure(std::move(result), "BeginFrame");;
        }
        
    }
    return mState == ServiceState::FAILED ? std::move(failure) : APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}
// ** Updates, Signals the end of a frame (must call begin frame before)
APIResult<ServiceResult::Value> ServiceContainer::EndFrame()
{
    if (mState != ServiceState::RUNNING)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot EndFrame, container is not started!", TServiceState::GetString(mState));
    }
    if (!mInFrame)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot end frame, container never begun one. (Missing BeginFrame call?)", "<NONE>");
    }

    APIResult<ServiceResult::Value> failure(ServiceResult::SERVICE_RESULT_SUCCESS);
    for (Service* service : mServices)
    {
        auto result = service->OnEndFrame();
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = ServiceState::FAILED;
            service->mServiceState = ServiceState::FAILED;
            failure.Report();
            failure = CatchPendingFailure(std::move(result), "EndFrame");;
        }
    }
    mInFrame = false;
    return mState == ServiceState::FAILED ? std::move(failure) : APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}
// ** Updates, Updates all the services
APIResult<ServiceResult::Value> ServiceContainer::FrameUpdate()
{
    if (mState != ServiceState::RUNNING)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot FrameUpdate, container is not started!", TServiceState::GetString(mState));
    }
    if (!mInFrame)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot update outside of frame. (Missing BeginFrame call?)", "<NONE>");
    }

    APIResult<ServiceResult::Value> failure(ServiceResult::SERVICE_RESULT_SUCCESS);
    for (Service* service : mServices)
    {
        auto result = service->OnFrameUpdate();
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = ServiceState::FAILED;
            service->mServiceState = ServiceState::FAILED;
            failure.Report();
            failure = CatchPendingFailure(std::move(result), "FrameUpdate");;
        }
    }
    return mState == ServiceState::FAILED ? std::move(failure) : APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}
// ** Allows one service to block a thread on a asynchronous task, updating other services.
// while( Task->Running ) { GetServices()->BlockingUpdate(this); }
APIResult<ServiceResult::Value> ServiceContainer::BlockingUpdate(Service* service) const
{
    // TODO: Thread Check: We should only call this on service thread.
    
    if (mState != ServiceState::RUNNING)
    {
        return ReportError(ServiceResult::SERVICE_RESULT_FAILED, OperationFailureError, "Cannot call blocking update, container is not started!", TServiceState::GetString(mState));
    }

    if (std::find(mBlockingServiceStack.begin(), mBlockingServiceStack.end(), service) != mBlockingServiceStack.end())
    {
        ReportBugMsg("Found recursive BlockingUpdate call");
        return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_FAILED);
    }

    mBlockingServiceStack.push_back(service);
    for (Service* current : mServices)
    {
        auto result = current->OnBlockingUpdate(service);
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = ServiceState::FAILED;
            current->mServiceState = ServiceState::FAILED;
            return CatchPendingFailure(std::move(result), "BlockingUpdate");;
        }
    }
    mBlockingServiceStack.erase(std::find(mBlockingServiceStack.begin(), mBlockingServiceStack.end(), service));
    return APIResult<ServiceResult::Value>(ServiceResult::SERVICE_RESULT_SUCCESS);
}
// ** Shutdown the service, releasing any resources. (Respect the shutdown mode)
APIResult<ServiceResult::Value> ServiceContainer::Shutdown(ServiceShutdownMode mode)
{
    APIResult<ServiceResult::Value> failure(ServiceResult::SERVICE_RESULT_SUCCESS);
    for (Service* service : mServices)
    {
        gSysLog.Info(LogMessage("Shutting down service ") << service->GetType()->GetFullName());
        auto result = service->OnShutdown(mode);
        if (result != ServiceResult::SERVICE_RESULT_SUCCESS)
        {
            mState = ServiceState::FAILED;
            service->mServiceState = ServiceState::FAILED;
            failure.Report();
            failure = CatchPendingFailure(std::move(result), "Shutdown");;
        }
    }
    mState = ServiceState::SHUTDOWN;
    return failure;
}

} // namespace
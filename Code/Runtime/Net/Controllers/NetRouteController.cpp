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
#if 0
#include "NetRouteController.h"
#include "Runtime/Net/NetRequest.h"
#include "Runtime/Net/NetConnection.h"
#include "Runtime/Net/NetRequestHandler.h"
#include "Runtime/Net/Packets/RequestPacket.h"
#include "Runtime/Reflection/ReflectionMgr.h"

namespace lf {

bool NetRouteController::Initialize()
{
    InitHandlers();
    if (!InitTaskScheduler())
    {
        return false;
    }
    if (!InitAllocator())
    {
        mTaskScheduler.Shutdown();
        return false;
    }
    return true;
}
void NetRouteController::Reset()
{
    mTaskScheduler.Shutdown();
    mAllocator.Release();
}

bool NetRouteController::ProcessRequest(const RequestHeader& header, const ByteT* bytes, NetConnection* connection)
{
    const NetRequestHandler* handler = GetRequestHandler(header.mRoute);
    if (!handler)
    {
        return false;
    }

    NetRequestAtomicPtr request = handler->CreateRequest(bytes, header.mDataSize);
    if (!request)
    {
        return false;
    }

    Request req;
    req.mRequest = request;
    req.mPacketSecurity = header.mSecurity;
    req.mPacketUID = header.mPacketUID;
    req.mRequestHandler = handler;
    req.mConnection = GetAtomicPointer(connection);

    mTaskScheduler.RunTask([](void*) {

    });
    
    mActiveRequests.Add(req);
    return true;
}

const NetRequestHandler* NetRouteController::GetRequestHandler(const String& routeName) const
{
    auto iter = mRequestHandlers.find(routeName);
    if (iter == mRequestHandlers.end())
    {
        return nullptr;
    }
    return iter->second;
}

void NetRouteController::InitHandlers()
{
    TVector<const Type*> types = GetReflectionMgr().FindAll(typeof(NetRequestHandler));
    for (const Type* type : types)
    {
        if (type == typeof(NetRequestHandler) || type->IsAbstract())
        {
            continue;
        }

        NetRequestHandlerPtr requestHandler = GetReflectionMgr().Create<NetRequestHandler>(type);
        mRequestHandlers[requestHandler->GetRouteName()] = requestHandler;
    }
}

bool NetRouteController::InitTaskScheduler()
{
    TaskScheduler::OptionsType options;
    options.mNumWorkerThreads = 2;
    options.mDispatcherSize = 20;
#if defined(LF_DEBUG) || defined(LF_TEST)
    options.mWorkerName = "Route Worker Thread";
#endif
    mTaskScheduler.Initialize(options, true);
    return mTaskScheduler.IsRunning();
}

bool NetRouteController::InitAllocator()
{
    const SizeT MAX_OBJECTS = 100;
    const SizeT MAX_HEAPS = 3;
#if defined(LF_TEST)
    const UInt32 FLAGS = PoolHeap::PHF_DOUBLE_FREE | PoolHeap::PHF_DETECT_LOCAL_HEAP_CORRUPTION;
#else
    const UInt32 FLAGS = PoolHeap::PHF_DOUBLE_FREE;
#endif
    return mAllocator.Initialize(MAX_OBJECTS, MAX_HEAPS, FLAGS);
}

} // namespace lf
#endif 
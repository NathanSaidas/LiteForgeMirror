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
#if 0#include "Core/Net/NetTypes.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/DynamicPoolHeap.h"
#include "Core/Utility/StdMap.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Runtime/Net/PacketSecurity.h"

namespace lf {

class String;
struct RequestHeader;

DECLARE_PTR(NetRequestHandler);
DECLARE_ATOMIC_WPTR(NetConnection);
DECLARE_ATOMIC_PTR(NetRequest);

class LF_RUNTIME_API NetRouteController
{
public:
    bool Initialize();
    void Reset();

    bool ProcessRequest(const RequestHeader& header, const ByteT* bytes, NetConnection* connection);

    const NetRequestHandler* GetRequestHandler(const String& routeName) const;
private:
    struct Request
    {
        NetRequestAtomicPtr      mRequest;
        const NetRequestHandler* mRequestHandler;
        PacketUID                mPacketUID;
        PacketSecurity::Value    mPacketSecurity;
        NetConnectionAtomicWPtr  mConnection;
    };

    void InitHandlers();
    bool InitTaskScheduler();
    bool InitAllocator();

    using RequestHandlerMap = TMap<String, NetRequestHandlerPtr>;
    // ** A mapping between route names and their actual handler objects. Built at init time and the raw pointers are safe to hold onto until shutdown.
    RequestHandlerMap mRequestHandlers;

    using ResponseMap = TMap<PacketUID, Request>;
    using ResponseConnectionMap = TMap<ConnectionID, ResponseMap>;
    // ** A mapping from a connection ID to responses that are awaiting an ack.
    ResponseConnectionMap mPendingResponses;
    // ** A list of active requests being processed.
    TVector<Request> mActiveRequests;

    TaskScheduler mTaskScheduler;
    
    TDynamicPoolHeap<Request> mAllocator;
};

} // namespace lf
#endif 
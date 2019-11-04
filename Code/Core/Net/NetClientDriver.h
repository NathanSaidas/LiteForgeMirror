#pragma once
// ********************************************************************
// Copyright (c) 2019 Nathan Hanlan
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
#include "Core/Net/NetDriver.h"

#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Net/Controllers/NetClientController.h"
#include "Core/Net/Controllers/NetEventController.h"
#include "Core/Net/NetTransport.h"

namespace lf {

class LF_CORE_API NetClientDriver : public NetDriver
{
public:
    using Super = NetDriver;

    NetClientDriver(const NetClientDriver&) = delete;
    NetClientDriver& operator=(const NetClientDriver&) = delete;
    NetClientDriver(NetClientDriver&& other) = delete;
    NetClientDriver& operator=(NetClientDriver&& other) = delete;

    NetClientDriver();
    ~NetClientDriver();

    void SendEvent(NetEventType::Value eventType, NetEvent* event) final;

    bool Initialize(Crypto::RSAKey serverKey, const IPEndPointAny& endPoint, UInt16 appID, UInt16 appVersion);
    void Shutdown();

    void Update();

    bool IsConnected() const;
    bool EmitHeartbeat(bool force = false);
private:
    TaskScheduler       mTaskScheduler;
    NetTransport        mTransport;
    NetClientController mClientController;
    NetEventController  mEventController;
    bool                mHeartbeatWaiting;
    UInt32              mHeartbeatID;
    Int64               mHeartbeatTick;

};

} // namespace lf

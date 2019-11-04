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
#include "Core/Net/Controllers/NetConnectionController.h"
#include "Core/Net/Controllers/NetEventController.h"
#include "Core/Net/Controllers/NetServerController.h"
#include "Core/Net/NetTransport.h"

namespace lf {

class LF_CORE_API NetServerDriver : public NetDriver
{
public:
    using Super = NetDriver;

    NetServerDriver(const NetServerDriver&) = delete;
    NetServerDriver& operator=(const NetServerDriver&) = delete;
    NetServerDriver(NetServerDriver&& other) = delete;
    NetServerDriver& operator=(NetServerDriver&& other) = delete;

    NetServerDriver();
    ~NetServerDriver();

    void SendEvent(NetEventType::Value eventType, NetEvent* event) final;

    bool Initialize(Crypto::RSAKey serverKey, UInt16 port, UInt16 appID, UInt16 appVersion);
    void Shutdown();

    void Update();
    void DropConnection(ConnectionID connection);

private:
    TaskScheduler           mTaskScheduler;
    NetTransport            mTransport;
    NetConnectionController mConnectionController;
    NetServerController     mServerController;
    NetEventController      mEventController;

};

} // namespace lf
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
#include "NetServerDriver.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/TransportHandlers/ServerConnectionHandler.h"
#include "Core/Net/TransportHandlers/ServerHeartbeatHandler.h"
#include "Core/Net/NetConnection.h"
#include "Core/Utility/Log.h"

namespace lf {

NetServerDriver::NetServerDriver()
{

}
NetServerDriver::~NetServerDriver()
{

}

void NetServerDriver::SendEvent(NetEventType::Value eventType, NetEvent* event)
{
    switch (eventType)
    {
    case NetEventType::NET_EVENT_CONNECT_SUCCESS:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_CONNECT_SUCCESS"));

    } break;
    case NetEventType::NET_EVENT_CONNECT_FAILED:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_CONNECT_FAILED"));

    } break;
    case NetEventType::NET_EVENT_CONNECTION_CREATED:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_CONNECTION_CREATED"));

    } break;
    case NetEventType::NET_EVENT_CONNECTION_TERMINATED:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_CONNECTION_TERMINATED"));

    } break;
    case NetEventType::NET_EVENT_HEARTBEAT_RECEIVED:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_HEARTBEAT_RECEIVED"));

    } break;
    case NetEventType::NET_EVENT_DATA_RECEIVED_REQUEST:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_DATA_RECEIVED_REQUEST"));

    } break;
    case NetEventType::NET_EVENT_DATA_RECEIVED_RESPONSE:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_DATA_RECEIVED_RESPONSE"));

    } break;
    case NetEventType::NET_EVENT_DATA_RECEIVED_ACTION:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_CONNECT_SUCCESS"));

    } break;
    case NetEventType::NET_EVENT_DATA_RECEIVED_REPLICATION:
    {
        gSysLog.Info(LogMessage("Server Event received: NET_EVENT_CONNECT_SUCCESS"));

    } break;
    default:
        CriticalAssertMsg("Unknown event!");
        break;
    }
    mEventController.Free(event);
}

bool NetServerDriver::Initialize(Crypto::RSAKey serverKey, UInt16 port, UInt16 appID, UInt16 appVersion)
{
    // Initialize ServerController
    if (!mServerController.Initialize(std::move(serverKey)))
    {
        return false;
    }

    // todo: Initialize ConnectionController
    
    // Initialize EventController
    if (!mEventController.Initialize())
    {
        mServerController.Reset();
        return false;
    }

    // Initialize TaskScheduler
    TaskTypes::TaskSchedulerOptions options;
    options.mDispatcherSize = 20;
    options.mNumWorkerThreads = 2;
#if defined(LF_DEBUG) || defined(LF_TEST)
    options.mWorkerName = "NetServerWorker";
#endif

    mTaskScheduler.Initialize(options, true);
    if (!mTaskScheduler.IsRunning())
    {
        mEventController.Reset();
        mServerController.Reset();
        return false;
    }

    // Initialize Transport w/ handlers
    NetTransportConfig config;
    config.SetAppId(appID);
    config.SetAppVersion(appVersion);
    config.SetPort(port);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT,
        LFNew<ServerConnectionHandler>(
            MMT_GENERAL,
            &mTaskScheduler,
            &mConnectionController,
            &mServerController,
            &mEventController,
            this)
    );
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_HEARTBEAT,
        LFNew<ServerHeartbeatHandler>(
            MMT_GENERAL,
            &mTaskScheduler,
            &mConnectionController,
            &mEventController,
            this)
    );

    mTransport.Start(std::move(config));
    if (!mTransport.IsRunning())
    {
        mEventController.Reset();
        mServerController.Reset();
        mTaskScheduler.Shutdown();
        return false;
    }
    return true;
}
void NetServerDriver::Shutdown()
{
    mTaskScheduler.Shutdown();
    mTransport.Stop();
    mEventController.Reset();
    mServerController.Reset();
    mConnectionController.Reset();
}

void NetServerDriver::Update()
{
    mEventController.GCCollect();
    TArray<NetConnectionAtomicPtr> disconnected;
    mConnectionController.Update(disconnected);

    for (NetConnection* connection : disconnected)
    {
        if (connection)
        {
            gSysLog.Info(LogMessage("User ") << connection->mID << " connection timed out.");
            connection->mSocket.Close();
            connection->mClientKey.Clear();
            connection->mUniqueServerKey.Clear();
            connection->mSharedKey.Clear();
        }
    }


}

void NetServerDriver::DropConnection(ConnectionID connection)
{
    if (mConnectionController.DeleteConnection(connection))
    {
        gSysLog.Info(LogMessage("User ") << connection << " was removed.");
    }
}

} // namespace lf
 

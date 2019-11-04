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
#include "NetClientDriver.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/TransportHandlers/ClientConnectionHandler.h"
#include "Core/Net/TransportHandlers/ClientHeartbeatHandler.h"
#include "Core/Utility/ByteOrder.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"

namespace lf {

NetClientDriver::NetClientDriver()
: Super()
, mTaskScheduler()
, mTransport()
, mClientController()
, mEventController()
, mHeartbeatWaiting(false)
, mHeartbeatID(0)
, mHeartbeatTick(0)
{
}
NetClientDriver::~NetClientDriver()
{
    CriticalAssert(!mTaskScheduler.IsRunning());
    CriticalAssert(!mTransport.IsRunning());
}

void NetClientDriver::SendEvent(NetEventType::Value eventType, NetEvent* event)
{
    switch (eventType)
    {
        case NetEventType::NET_EVENT_CONNECT_SUCCESS:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_CONNECT_SUCCESS"));
            
        } break;
        case NetEventType::NET_EVENT_CONNECT_FAILED:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_CONNECT_FAILED"));

        } break;
        case NetEventType::NET_EVENT_CONNECTION_CREATED:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_CONNECTION_CREATED"));

        } break;
        case NetEventType::NET_EVENT_CONNECTION_TERMINATED:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_CONNECTION_TERMINATED"));

        } break;
        case NetEventType::NET_EVENT_HEARTBEAT_RECEIVED:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_HEARTBEAT_RECEIVED"));
            mHeartbeatWaiting = false;
            mHeartbeatTick = GetClockTime();
        } break;
        case NetEventType::NET_EVENT_DATA_RECEIVED_REQUEST:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_DATA_RECEIVED_REQUEST"));

        } break;
        case NetEventType::NET_EVENT_DATA_RECEIVED_RESPONSE:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_DATA_RECEIVED_RESPONSE"));

        } break;
        case NetEventType::NET_EVENT_DATA_RECEIVED_ACTION:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_CONNECT_SUCCESS"));

        } break;
        case NetEventType::NET_EVENT_DATA_RECEIVED_REPLICATION:
        {
            gSysLog.Info(LogMessage("Client Event received: NET_EVENT_CONNECT_SUCCESS"));

        } break;
        default:
            CriticalAssertMsg("Unknown event!");
            break;
    }
    mEventController.Free(event);
}

bool NetClientDriver::Initialize(Crypto::RSAKey serverKey, const IPEndPointAny& endPoint, UInt16 appID, UInt16 appVersion)
{
    TaskTypes::TaskSchedulerOptions options;
    options.mDispatcherSize = 20;
    options.mNumWorkerThreads = 2;
#if defined(LF_DEBUG) || defined(LF_TEST)
    options.mWorkerName = "NetClientWorker";
#endif


    // Initialize ClientController
    if (!mClientController.Initialize(std::move(serverKey)))
    {
        return false;
    }

    // Create a connection packet message.
    PacketDataType::ConnectPacketData packet;
    SizeT size = sizeof(packet.mBytes);
    const bool encoded = ConnectPacket::EncodePacket
    (
        packet.mBytes,
        size,
        mClientController.GetKey(),
        mClientController.GetServerKey(),
        mClientController.GetSharedKey(),
        mClientController.GetHMACKey(),
        mClientController.GetChallenge()
    );

    if (!encoded)
    {
        return false;
    }

    // Initialize EventController
    if (!mEventController.Initialize())
    {
        mClientController.Reset();
        return false;
    }
    // Initialize TaskScheduler
    mTaskScheduler.Initialize(options, true);
    if(!mTaskScheduler.IsRunning())
    {
        mClientController.Reset();
        mEventController.Reset();
        return false;
    }
    // Initialize Transport w/ handlers
    NetTransportConfig config;
    config.SetAppId(appID);
    config.SetAppVersion(appVersion);
    config.SetPort(SwapBytes(endPoint.mPort));
    config.SetEndPoint(endPoint);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT,
        LFNew<ClientConnectionHandler>(
            MMT_GENERAL,
            &mTaskScheduler,
            &mClientController,
            &mEventController,
            this)
    );
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_HEARTBEAT,
        LFNew<ClientHeartbeatHandler>(
            MMT_GENERAL,
            &mTaskScheduler,
            &mClientController,
            &mEventController,
            this)
    );

    // Attempt to startup transport
    mTransport.Start(std::move(config), packet.mBytes, size);
    if (!mTransport.IsRunning())
    {
        mTaskScheduler.Shutdown();
        mEventController.Reset();
        mClientController.Reset();
        return false;
    }

    mHeartbeatTick = GetClockTime();

    return true;
}
void NetClientDriver::Shutdown()
{
    if (mTaskScheduler.IsRunning())
    {
        mTaskScheduler.Shutdown();
    }
    if (mTransport.IsRunning())
    {
        mTransport.Stop();
    }
    mEventController.Reset();
    mClientController.Reset();
}

void NetClientDriver::Update()
{
    mEventController.GCCollect();
    
    if (IsConnected())
    {
        Float64 frequency = static_cast<Float64>(GetClockFrequency());
        Float64 elapsed = (GetClockTime() - mHeartbeatTick) / frequency;
        if (elapsed > 0.500)
        {
            Shutdown();
            gSysLog.Info(LogMessage("Server connection timed out."));
        }
    }
}

bool NetClientDriver::IsConnected() const
{
    return mClientController.IsConnected();
}

bool NetClientDriver::EmitHeartbeat(bool force)
{
    if (!IsConnected() || (mHeartbeatWaiting && force == false))
    {
        return false;
    }

    // Encode heartbeat
    ByteT bytes[1024];
    SizeT size = sizeof(bytes);
    if
    (
        !HeartbeatPacket::EncodePacket
        (
            bytes,
            size,
            mClientController.GetUniqueKey(),
            mClientController.GetClientNonce(),
            mClientController.GetServerNonce(),
            mClientController.GetConnectionID(),
            mHeartbeatID
        )
    )
    {
        return false;
    }

    // Try and send it
    mHeartbeatWaiting = true;
    if (!mTransport.Send(bytes, size, mTransport.GetBoundEndPoint()))
    {
        mHeartbeatWaiting = false;
        return false;
    }
    ++mHeartbeatID;
    return true;
}

} // namespace lf
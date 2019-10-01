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
#include "NetTransport.h"
#if !defined(LF_IMPL_OPAQUE_OPTIMIZE)
#include "NetTransportImpl.h"
#endif
#include "Core/Common/Assert.h"
#include "Core/Crypto/RSA.h"
#include "Core/Net/NetFramework.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/NetTransportHandler.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Utility/ByteOrder.h"
#include "Core/Utility/Crc32.h"
#include "Core/Utility/ErrorCore.h"

#include "Core/Utility/Log.h" // temp

#include <utility>

namespace lf {

LF_IMPL_OPAQUE(NetTransport)::LF_IMPL_OPAQUE(NetTransport)()
: mInboundThread()
, mInbound()
, mBoundEndPoint()
, mAppId(0)
, mAppVersion(0)
, mHandlers()
, mRunning(0)
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        mHandlers[i] = nullptr;
    }
}
LF_IMPL_OPAQUE(NetTransport)::LF_IMPL_OPAQUE(NetTransport)(LF_IMPL_OPAQUE(NetTransport) && other)
: mInboundThread(std::move(other.mInboundThread))
, mInbound(std::move(other.mInbound))
, mBoundEndPoint(std::move(other.mBoundEndPoint))
, mAppId(other.mAppId)
, mAppVersion(other.mAppVersion)
, mHandlers()
, mRunning(other.mRunning)
{
    other.mAppId = 0;
    other.mAppVersion = 0;

    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        mHandlers[i] = other.mHandlers[i];
        other.mHandlers[i] = nullptr;
    }
    AtomicStore(&other.mRunning, 0);
}
LF_IMPL_OPAQUE(NetTransport)::~LF_IMPL_OPAQUE(NetTransport)()
{
    CriticalAssertEx(!IsRunning(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
    CriticalAssertEx(!mInboundThread.IsRunning(), LF_ERROR_RESOURCE_LEAK, ERROR_API_CORE);
}

LF_IMPL_OPAQUE(NetTransport)& LF_IMPL_OPAQUE(NetTransport)::operator=(LF_IMPL_OPAQUE(NetTransport)&& other)
{
    if (this == &other)
    {
        return *this;
    }

    mInboundThread = std::move(other.mInboundThread);
    mInbound = std::move(other.mInbound);
    mBoundEndPoint = std::move(other.mBoundEndPoint);
    mAppId = other.mAppId;
    mAppVersion = other.mAppVersion;
    other.mAppId = 0;
    other.mAppVersion = 0;
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        mHandlers[i] = other.mHandlers[i];
        other.mHandlers[i] = nullptr;
    }
    AtomicStore(&mRunning, AtomicLoad(&other.mRunning));
    AtomicStore(&other.mRunning, 0);
    return *this;
}

const char* PACKET_TYPE_NAMES[]=
{
    "CONNECT",
    "DISCONNECT",
    "HEARTBEAT",
    "MESSAGE"
};

void LF_IMPL_OPAQUE(NetTransport)::Start(NetTransportConfig&& config)
{
    if (IsRunning())
    {
        return;
    }

    // Verify Config:
    if (config.GetPort() == 0) // todo: This is actually valid, only IF the config is for 'client'
    {
        gSysLog.Error(LogMessage("NetTransport failed to start. Invalid configuration. 'Port' != 0 "));
        return;
    }

    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        mHandlers[i] = config.GetTransportHandler(static_cast<NetPacketType::Value>(i));
        if (!mHandlers[i])
        {
            gSysLog.Warning(LogMessage("NetTransport does not have a handler for ") << PACKET_TYPE_NAMES[i] << " packets.");
        }
    }
    config.CloseHandlers();

    mAppId = config.GetAppId();
    mAppVersion = config.GetAppVersion();
    
    
    // Servers require a 'inbound' socket.
    // Clients can use one socket to send and receive?
    //

    gSysLog.Info(LogMessage("Creating socket..."));
    if (!mInbound.Create(NetProtocol::NET_PROTOCOL_UDP))
    {
        return;
    }

    gSysLog.Info(LogMessage("Binding socket..."));
    if (!mInbound.Bind(config.GetPort()))
    {
        return;
    }
    CriticalAssertEx(IPV4(mBoundEndPoint, "127.0.0.1", config.GetPort()), LF_ERROR_INTERNAL, ERROR_API_CORE);
    gSysLog.Info(LogMessage("Initializing NetTransportHandlers..."));
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        if (mHandlers[i])
        {
            mHandlers[i]->Initialize();
        }
    }

    AtomicStore(&mRunning, 1);
    gSysLog.Info(LogMessage("Forking receiver..."));
    mInboundThread.Fork(ProcessReceiveThread, this);
    mInboundThread.SetDebugName("NetTransportReceive");

    config = NetTransportConfig();
}

void LF_IMPL_OPAQUE(NetTransport)::Stop()
{
    if (!IsRunning())
    {
        return;
    }

    AtomicStore(&mRunning, 0);
    if (mInbound.IsAwaitingReceive())
    {
        gSysLog.Info(LogMessage("Flushing receiver..."));
        
        UDPSocket flusher;
        flusher.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP);
        

        ByteT flushMsg[2] = { 0 };
        SizeT inOutBytes = sizeof(flushMsg);
        while (mInbound.IsAwaitingReceive())
        {
            gSysLog.Info(LogMessage("Sending flush bytes..."));
            AssertEx(flusher.SendTo(flushMsg, inOutBytes, mBoundEndPoint), LF_ERROR_INTERNAL, ERROR_API_CORE);
            SleepCallingThread(1);
        }
    }

    gSysLog.Info(LogMessage("Joining receiver..."));
    mInboundThread.Join();

    gSysLog.Info(LogMessage("Shutting down NetTransportHandlers..."));
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mHandlers); ++i)
    {
        if (mHandlers[i])
        {
            mHandlers[i]->Shutdown();
            LFDelete(mHandlers[i]);
            mHandlers[i] = nullptr;
        }
    }

    mInbound.Close();
    
}

bool LF_IMPL_OPAQUE(NetTransport)::IsRunning() const
{
    return AtomicLoad(&mRunning) != 0;
}

void LF_IMPL_OPAQUE(NetTransport)::ProcessReceiveThread(void* self)
{
    reinterpret_cast<LF_IMPL_OPAQUE(NetTransport)*>(self)->ProcessReceive();
}

void LF_IMPL_OPAQUE(NetTransport)::ProcessReceive()
{
    gSysLog.Info(LogMessage("Executing transport receiver..."));

    ByteT bytes[2048];
    while (IsRunning())
    {
        SizeT receivedBytes = sizeof(bytes);
        IPEndPointAny sender;
        if (!mInbound.ReceiveFrom(bytes, receivedBytes, sender) || !IsRunning())
        {
            continue;
        }

        if (receivedBytes < sizeof(PacketHeader))
        {
            continue;
        }

        const PacketHeader* header = reinterpret_cast<PacketHeader*>(bytes);
        if (header->mAppID != mAppId)
        {
            continue;
        }

        // todo: In the future we could possibly handle 'older' versions of packets but definately not newer.
        if (header->mAppVersion != mAppVersion)
        {
            continue;
        }

        UInt32 crc32 = PacketUtility::CalcCrc32(bytes, receivedBytes);
        if (header->mCrc32 != crc32)
        {
            // Connectionless so we need to ACK with the InBound socket.. but we can also kick it off to the 'acker'
            if (PacketUtility::GetHeaderType(bytes, receivedBytes) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE)
            {
                Crypto::RSAKey dummy;
                ByteT ack[PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE];
                SizeT ackSize = sizeof(ack);
                if (PacketUtility::PrepareAckCorruptHeader(bytes, receivedBytes, ack, ackSize, dummy))
                {
                    mInbound.SendTo(ack, ackSize, sender);
                }
            }
            // todo: Ack(Corrupt)
            else
            {
                // figure out the connection etc...
            }
            continue;
        }

        // todo: Verify LogicalPacketData (ie, do the flags/type make sense)
        if (header->mType >= NetPacketType::MAX_VALUE)
        {
            continue;
        }

        if (!mHandlers[header->mType])
        {
            continue;
        }

#if defined(LF_USE_EXCEPTIONS)
        try {
#endif
            mHandlers[header->mType]->ReceivePacket(bytes, receivedBytes, sender);
#if defined(LF_USE_EXCEPTIONS)
        }
        catch (Exception& exception)
        {
            (exception);
        }
#endif
    }

    gSysLog.Info(LogMessage("Terminating transport receiver..."));
}

} // namespace lf
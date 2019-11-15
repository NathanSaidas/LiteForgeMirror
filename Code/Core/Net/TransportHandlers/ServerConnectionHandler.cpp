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
#include "ServerConnectionHandler.h"
#include "Core/Common/Assert.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/RSA.h"
#include "Core/Net/Controllers/NetConnectionController.h"
#include "Core/Net/Controllers/NetEventController.h"
#include "Core/Net/Controllers/NetServerController.h"
#include "Core/Net/ConnectPacket.h"
#include "Core/Net/NetConnection.h"
#include "Core/Net/NetDriver.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Utility/Time.h"
#include "Core/Utility/Log.h"

#include "Core/Net/NetFramework.h"

#include <utility>

namespace lf {

// 256 == The assumed RSA key size in bytes.
static const Crypto::RSAKeySize REQUIRED_KEY_SIZE = Crypto::RSA_KEY_2048;
static const SizeT REQUIRED_PACKET_SIZE = ConnectPacket::HeaderType::ACTUAL_SIZE + 256;


ServerConnectionHandler::ServerConnectionHandler
(
    TaskScheduler* taskScheduler, 
    NetConnectionController* connectionController, 
    NetServerController* serverController,
    NetEventController* eventController,
    NetDriver* driver
)
: mTaskScheduler(taskScheduler)
, mConnectionController(connectionController)
, mEventController(eventController)
, mServerController(serverController)
, mDriver(driver)
, mAllocator()
{

}

ServerConnectionHandler::ServerConnectionHandler(ServerConnectionHandler&& other)
: mTaskScheduler(other.mTaskScheduler)
, mConnectionController(other.mConnectionController)
, mEventController(other.mEventController)
, mServerController(other.mServerController)
, mDriver(other.mDriver)
, mAllocator(std::move(other.mAllocator))
{
    other.mTaskScheduler = nullptr;
    other.mConnectionController = nullptr;
    other.mEventController = nullptr;
    other.mServerController = nullptr;
    other.mDriver = nullptr;
}

ServerConnectionHandler::~ServerConnectionHandler()
{
    CriticalAssert(mAllocator.GetHeap().GetHeapCount() == 0);
}
ServerConnectionHandler& ServerConnectionHandler::operator=(ServerConnectionHandler&& other)
{
    if (this != &other)
    {
        mTaskScheduler = other.mTaskScheduler;
        mConnectionController = other.mConnectionController;
        mEventController = other.mEventController;
        mServerController = other.mServerController;
        mDriver = other.mDriver;
        mAllocator = std::move(other.mAllocator);

        other.mTaskScheduler = nullptr;
        other.mConnectionController = nullptr;
        other.mEventController = nullptr;
        other.mServerController = nullptr;
        other.mDriver = nullptr;
    }
    return *this;
}

void ServerConnectionHandler::DecodePacket(PacketType* packetData)
{
    // Under the assumption we're calling it from our internal function.

    Crypto::RSAKey clientKey;
    Crypto::AESKey sharedKey;
    ByteT hmacKey[Crypto::HMAC_KEY_SIZE];
    ByteT challenge[ConnectPacket::CHALLENGE_SIZE];
    ConnectPacket::HeaderType header;

    if
    (
        !ConnectPacket::DecodePacket
        (
            packetData->mBytes,
            packetData->mSize,
            mServerController->GetServerKey(),
            clientKey,
            sharedKey,
            hmacKey,
            challenge,
            header
        )
    )
    {
        gSysLog.Debug(LogMessage("Dropping packet, failed to decode packet."));
        // mTelemetryController->DecodePacketFailure(CONNECT)
        return;
    }

    // Allocate a connection
    NetConnection* connection = mConnectionController->InsertConnection();
    if (!connection)
    {
        // mTelemetryController->ConnectionAllocationFailure()
        return;
    }

    // Finalize Connection:
    connection->mEndPoint = packetData->mSender;
    connection->mClientKey = std::move(clientKey);
    connection->mSharedKey = std::move(sharedKey);
    memcpy(connection->mHMACKey, hmacKey, Crypto::HMAC_KEY_SIZE);
    connection->mUniqueServerKey.GeneratePair(REQUIRED_KEY_SIZE);
    
    PacketData1024 ackPacket;
    SizeT ackPacketBytes = sizeof(ackPacket.mBytes);
    if 
    (
        !ConnectPacket::EncodeAckPacket
        (
            ackPacket.mBytes,
            ackPacketBytes,
            connection->mClientKey,
            connection->mUniqueServerKey,
            connection->mSharedKey,
            connection->mHMACKey,
            challenge,
            connection->mServerNonce,
            connection->mID
        )
    )
    {
        // mTelemetryController->EncodeAckPacketFailure(CONNECT)
        return;
    }

    // todo: Support both IPV4/IPV6 instead of one or the other.
    // connection->mSocket.Create(connection->mEndPoint.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4 ? NetProtocol::NET_PROTOCOL_IPV4_UDP : NetProtocol::NET_PROTOCOL_IPV6_UDP);
    // connection->mSocket.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP);
    connection->mSocket.Create(NetProtocol::NET_PROTOCOL_IPV6_UDP);
    connection->mLastTick = GetClockTime(); // todo: May be worth to tick on the date time?
    
    SizeT sentBytes = ackPacketBytes;
    if (!connection->mSocket.SendTo(ackPacket.mBytes, sentBytes, connection->mEndPoint) || sentBytes != ackPacketBytes)
    {
        // mTelemetryController->SocketSendFailure()
    }
    PacketData::SetZero(ackPacket);

    // todo: This event MUST be processed before we can 'deallocate the connection ID.
    // a) Put some sort of 'lock' on the connection until event is processed.
    // b) Pass atomic wptr of the connection
    NetConnectionCreatedEvent* event = mEventController->Allocate<NetConnectionCreatedEvent>();
    NET_EVENT_DEBUG_INFO(event);
    event->mConnectionID = connection->mID;
    mDriver->SendEvent(event->GetType(), event);
}

void ServerConnectionHandler::OnInitialize()
{
    const SizeT POOL_OBJECT_COUNT = 256;
    const SizeT POOL_MAX_HEAPS = 3;
    const UInt32 POOL_FLAGS = PoolHeap::PHF_DOUBLE_FREE;

    CriticalAssert(mAllocator.Initialize(POOL_OBJECT_COUNT, POOL_MAX_HEAPS, POOL_FLAGS));
    CriticalAssert(mServerController->GetServerKey().GetKeySize() == REQUIRED_KEY_SIZE);
}
void ServerConnectionHandler::OnShutdown()
{
    mAllocator.Release();
}
void ServerConnectionHandler::OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender)
{
    // Server does not process Acks
    if (PacketUtility::IsAck(bytes, byteLength))
    {
        return;
    }

    // Invalid Packet Format:
    if (byteLength < REQUIRED_PACKET_SIZE)
    {
        return;
    }

    PacketType* connectPacket = mAllocator.Allocate();
    if (!connectPacket)
    {
        return;
    }

    connectPacket->mSize = static_cast<UInt16>(byteLength);
    connectPacket->mType = NetPacketType::NET_PACKET_TYPE_CONNECT;
    connectPacket->mRetransmits = 0;
    connectPacket->mSender = sender;
    memcpy(connectPacket->mBytes, bytes, byteLength);

    // Begin the net task:
    mTaskScheduler->RunTask([connectPacket, this](void*)
    {
        DecodePacket(connectPacket);
        mAllocator.Free(connectPacket);
    });
    
}
void ServerConnectionHandler::OnUpdateFrame()
{
    mAllocator.GCCollect(); // todo: Timed GC perhaps?
    
}

} // namespace lf 
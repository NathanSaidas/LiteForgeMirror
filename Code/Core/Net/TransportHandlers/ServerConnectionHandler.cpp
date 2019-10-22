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
#include "Core/Net/ConnectPacket.h"
#include "Core/Net/NetConnection.h"
#include "Core/Net/NetConnectionController.h"
#include "Core/Net/NetServerController.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Utility/Time.h"

#include "Core/Net/NetFramework.h"

#include <utility>

namespace lf {

// 256 == The assumed RSA key size in bytes.
static const Crypto::RSAKeySize REQUIRED_KEY_SIZE = Crypto::RSA_KEY_2048;
static const SizeT REQUIRED_PACKET_SIZE = ConnectPacket::HeaderType::ACTUAL_SIZE + 256;


ServerConnectionHandler::ServerConnectionHandler(TaskScheduler* taskScheduler, NetConnectionController* connectionController, NetServerController* serverController)
: mTaskScheduler(taskScheduler)
, mConnectionController(connectionController)
, mServerController(serverController)
, mPacketPool()
{

}

ServerConnectionHandler::ServerConnectionHandler(ServerConnectionHandler&& other)
: mTaskScheduler(other.mTaskScheduler)
, mConnectionController(other.mConnectionController)
, mServerController(other.mServerController)
, mPacketPool(std::move(other.mPacketPool))
{
    other.mTaskScheduler = nullptr;
    other.mConnectionController = nullptr;
    other.mServerController = nullptr;
}

ServerConnectionHandler::~ServerConnectionHandler()
{
    CriticalAssert(mPacketPool.GetHeapCount() == 0);
}
ServerConnectionHandler& ServerConnectionHandler::operator=(ServerConnectionHandler&& other)
{
    if (this != &other)
    {
        mTaskScheduler = other.mTaskScheduler;
        mConnectionController = other.mConnectionController;
        mServerController = other.mServerController;
        mPacketPool = std::move(other.mPacketPool);

        other.mTaskScheduler = nullptr;
        other.mConnectionController = nullptr;
        other.mServerController = nullptr;
    }
    return *this;
}

void ServerConnectionHandler::DecodePacket(ConnectPacketData* packetData)
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
        FreePacket(packetData);
        // mTelemetryController->DecodePacketFailure(CONNECT)
        return;
    }

    // Allocate a connection
    NetConnection* connection = mConnectionController->InsertConnection();
    if (!connection)
    {
        FreePacket(packetData);
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
    SizeT ackPacketBytes = sizeof(PacketData1024);
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
            connection->mID
        )
    )
    {
        FreePacket(packetData);
        // mTelemetryController->EncodeAckPacketFailure(CONNECT)
        return;
    }

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
    FreePacket(packetData);
}

void ServerConnectionHandler::OnInitialize()
{
    const SizeT POOL_OBJECT_COUNT = 256;
    const SizeT POOL_MAX_HEAPS = 3;
    const UInt32 POOL_FLAGS = PoolHeap::PHF_DOUBLE_FREE;

    CriticalAssert(mPacketPool.Initialize(sizeof(ConnectPacketData), alignof(ConnectPacketData), POOL_OBJECT_COUNT, POOL_MAX_HEAPS, POOL_FLAGS));
    CriticalAssert(mServerController->GetServerKey().GetKeySize() == REQUIRED_KEY_SIZE);
}
void ServerConnectionHandler::OnShutdown()
{
    mPacketPool.Release();
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

    ConnectPacketData* connectPacket = AllocatePacket();
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
    });
    
}
void ServerConnectionHandler::OnUpdateFrame()
{
    mPacketPool.GCCollect(); // todo: Timed GC perhaps?
    
}

ServerConnectionHandler::ConnectPacketData* ServerConnectionHandler::AllocatePacket()
{
    void* object = mPacketPool.Allocate();
    if (object)
    {
        return new(object)ConnectPacketData();
    }
    return nullptr;
}
void ServerConnectionHandler::FreePacket(ConnectPacketData* packet)
{
    if (packet)
    {
        packet->~ConnectPacketData();
        PacketData::SetZero(*packet);
        mPacketPool.Free(packet);
    }
}

} // namespace lf 
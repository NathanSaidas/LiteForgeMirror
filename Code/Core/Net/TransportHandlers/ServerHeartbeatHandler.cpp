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

#include "ServerHeartbeatHandler.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Net/Controllers/NetConnectionController.h"
#include "Core/Net/Controllers/NetEventController.h"
#include "Core/Net/NetConnection.h"
#include "Core/Net/NetDriver.h"
#include "Core/Net/HeartbeatPacket.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Utility/Time.h"

namespace lf {

ServerHeartbeatHandler::ServerHeartbeatHandler(TaskScheduler* taskScheduler, NetConnectionController* connectionController, NetEventController* eventController, NetDriver* driver)
: mTaskScheduler(taskScheduler)
, mConnectionController(connectionController)
, mEventController(eventController)
, mDriver(driver)
, mAllocator()
{}
ServerHeartbeatHandler::ServerHeartbeatHandler(ServerHeartbeatHandler&& other)
: mTaskScheduler(other.mTaskScheduler)
, mConnectionController(other.mConnectionController)
, mEventController(other.mEventController)
, mDriver(other.mDriver)
, mAllocator(std::move(other.mAllocator))
{
    other.mTaskScheduler = nullptr;
    other.mConnectionController = nullptr;
    other.mEventController = nullptr;
    other.mDriver = nullptr;
}
ServerHeartbeatHandler::~ServerHeartbeatHandler()
{
    CriticalAssert(mAllocator.GetHeap().GetHeapCount() == 0);
}
ServerHeartbeatHandler& ServerHeartbeatHandler::operator=(ServerHeartbeatHandler&& other)
{
    if (this != &other)
    {
        mTaskScheduler = other.mTaskScheduler;
        mConnectionController = other.mConnectionController;
        mEventController = other.mEventController;
        mDriver = other.mDriver;
        mAllocator = std::move(other.mAllocator);
        other.mTaskScheduler = nullptr;
        other.mConnectionController = nullptr;
        other.mEventController = nullptr;
        other.mDriver = nullptr;
    }
    return *this;
}

void ServerHeartbeatHandler::DecodePacket(PacketType* packet)
{
    // Connection ID:
    HeartbeatPacket::HeaderType* header = reinterpret_cast<HeartbeatPacket::HeaderType*>(packet->mBytes);
    ConnectionID connectionID = header->mConnectionID;
    NetConnection* connection = mConnectionController->FindConnection(connectionID);
    if (!connection)
    {
        // Bad Connection -- !!!

        return;
    }

    ByteT clientMessage[NET_HEARTBEAT_NONCE_SIZE];
    ByteT serverMessage[NET_HEARTBEAT_NONCE_SIZE];
    HeartbeatPacket::HeaderType outHeader;
    {
        ScopeRWLockRead readLock(connection->mLock);
        if (!connection->mUniqueServerKey.HasPrivateKey())
        {
            // Disconnected -- !!!
            return;
        }

        if
            (
                !HeartbeatPacket::DecodePacket
                (
                    packet->mBytes,
                    packet->mSize,
                    connection->mUniqueServerKey,
                    clientMessage,
                    serverMessage,
                    outHeader
                )
                )
        {
            // Decode Failed -- !!!
            return;
        }

        // Verify Nonce
        if (memcmp(serverMessage, connection->mServerNonce, NET_HEARTBEAT_NONCE_SIZE) != 0)
        {
            // Bad Nonce
            return;
        }
    }
    

    ByteT newServerNonce[NET_HEARTBEAT_NONCE_SIZE];
    Crypto::SecureRandomBytes(newServerNonce, NET_HEARTBEAT_NONCE_SIZE);
    // Update Client Nonce
    {
        ScopeRWLockWrite writeLock(connection->mLock);
        memcpy(connection->mClientNonce, clientMessage, NET_HEARTBEAT_NONCE_SIZE);
        memcpy(connection->mServerNonce, newServerNonce, NET_HEARTBEAT_NONCE_SIZE);;
        connection->mLastTick = GetClockTime();
    }


    ByteT ack[sizeof(PacketType::mBytes)];
    SizeT ackSize = sizeof(ack);
    if
    (
        HeartbeatPacket::EncodeAckPacket
        (
            ack,
            ackSize,
            connection->mClientKey,
            clientMessage,
            newServerNonce,
            outHeader.mPacketUID
        )
    )
    {
        connection->mSocket.SendTo(ack, ackSize, connection->mEndPoint);
    }

    // Event: todo: Sender?
    NetHeartbeatReceivedEvent* event = mEventController->Allocate<NetHeartbeatReceivedEvent>();
    NET_EVENT_DEBUG_INFO(event);
    event->mSender = connectionID;
    memcpy(event->mNonce, serverMessage, NET_HEARTBEAT_NONCE_SIZE);
    mDriver->SendEvent(event->GetType(), event);
}

void ServerHeartbeatHandler::OnInitialize()
{
    const SizeT OBJECT_COUNT = 256;
    const SizeT MAX_HEAPS = 3;
    const UInt32 FLAGS = PoolHeap::PHF_DOUBLE_FREE;
    CriticalAssert(mAllocator.Initialize(OBJECT_COUNT, MAX_HEAPS, FLAGS));
}

void ServerHeartbeatHandler::OnShutdown()
{
    mAllocator.Release();
}

void ServerHeartbeatHandler::OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender)
{
    if (PacketUtility::IsAck(bytes, byteLength))
    {
        return;
    }

    // todo: Cull by packet size?

    PacketType* packet = mAllocator.Allocate();
    if (!packet)
    {
        return;
    }

    packet->mSize = static_cast<UInt16>(byteLength);
    packet->mType = NetPacketType::NET_PACKET_TYPE_HEARTBEAT;
    packet->mRetransmits = 0;
    packet->mSender = sender;
    memcpy(packet->mBytes, bytes, byteLength);

    mTaskScheduler->RunTask([packet, this](void*)
    {
        DecodePacket(packet);
        mAllocator.Free(packet);
    });
}

void ServerHeartbeatHandler::OnUpdateFrame()
{
    mAllocator.GCCollect();
}

} // namespace lf
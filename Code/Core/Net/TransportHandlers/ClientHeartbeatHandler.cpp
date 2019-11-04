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
#include "ClientHeartbeatHandler.h"
#include "Core/Common/Assert.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Net/Controllers/NetClientController.h"
#include "Core/Net/Controllers/NetEventController.h"
#include "Core/Net/NetDriver.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Net/HeartbeatPacket.h"

namespace lf {


ClientHeartbeatHandler::ClientHeartbeatHandler(TaskScheduler* taskScheduler, NetClientController* clientController, NetEventController* eventController, NetDriver* driver)
: mTaskScheduler(taskScheduler)
, mClientController(clientController)
, mEventController(eventController)
, mDriver(driver)
, mAllocator()
{
}
ClientHeartbeatHandler::ClientHeartbeatHandler(ClientHeartbeatHandler&& other)
: mTaskScheduler(other.mTaskScheduler)
, mClientController(other.mClientController)
, mEventController(other.mEventController)
, mDriver(other.mDriver)
, mAllocator(std::move(other.mAllocator))
{
    other.mTaskScheduler = nullptr;
    other.mClientController = nullptr;
    other.mEventController = nullptr;
    other.mDriver = nullptr;
}
ClientHeartbeatHandler::~ClientHeartbeatHandler()
{
    CriticalAssert(mAllocator.GetHeap().GetHeapCount() == 0);
}
ClientHeartbeatHandler& ClientHeartbeatHandler::operator=(ClientHeartbeatHandler&& other) 
{
    if (this != &other)
    {
        mTaskScheduler = other.mTaskScheduler;
        mEventController = other.mEventController;
        mClientController = other.mClientController;
        mDriver = other.mDriver;
        mAllocator = std::move(other.mAllocator);
        other.mTaskScheduler = nullptr;
        other.mClientController = nullptr;
        other.mEventController = nullptr;
        other.mDriver = nullptr;
    }
    return *this;
}

void ClientHeartbeatHandler::DecodePacket(PacketType* packet)
{
    HeartbeatPacket::AckHeaderType* header = reinterpret_cast<HeartbeatPacket::AckHeaderType*>(packet->mBytes);
    // Verify Nonce
    // Notify Client to Update with bytes.

    // Server failed to ack
    // todo: Can handle this error a bit better.
    if (header->mStatus != NetAckStatus::NET_ACK_STATUS_OK)
    {
        return;
    }

    ByteT clientMessage[NET_HEARTBEAT_NONCE_SIZE];
    ByteT serverMessage[NET_HEARTBEAT_NONCE_SIZE];
    UInt32 packetUID;
    HeartbeatPacket::AckHeaderType outHeader;

    if 
    (   !HeartbeatPacket::DecodeAckPacket
        (
        packet->mBytes,
        packet->mSize,
        mClientController->GetKey(),
        clientMessage,
        serverMessage,
        packetUID,
        outHeader
        )
    )
    {
        return;
    }


    if (!mClientController->SetNonce(clientMessage, serverMessage))
    {
        return;
    }

    NetHeartbeatReceivedEvent* event = mEventController->Allocate<NetHeartbeatReceivedEvent>();
    NET_EVENT_DEBUG_INFO(event);
    event->mSender = INVALID_CONNECTION;
    memcpy(event->mNonce, serverMessage, NET_HEARTBEAT_NONCE_SIZE);
    mDriver->SendEvent(event->GetType(), event);
}

void ClientHeartbeatHandler::OnInitialize()
{
    const SizeT OBJECT_COUNT = 256;
    const SizeT MAX_HEAPS = 3;
    const UInt32 FLAGS = PoolHeap::PHF_DOUBLE_FREE;
    CriticalAssert(mAllocator.Initialize(OBJECT_COUNT, MAX_HEAPS, FLAGS));
    
}
void ClientHeartbeatHandler::OnShutdown()
{
    mAllocator.Release();
}
void ClientHeartbeatHandler::OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender)
{
    if (!PacketUtility::IsAck(bytes, byteLength))
    {
        return;
    }

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
void ClientHeartbeatHandler::OnUpdateFrame()
{
    mAllocator.GCCollect();
}


}
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
#include "ClientConnectionHandler.h"
#include "Core/Common/Assert.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/RSA.h"
#include "Core/Net/ConnectPacket.h"
#include "Core/Net/Controllers/NetClientController.h"
#include "Core/Net/Controllers/NetEventController.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Net/NetDriver.h"

#include <utility>

namespace lf {

// 256 == The assumed RSA key size in bytes.
static const Crypto::RSAKeySize REQUIRED_RSA_SIZE = Crypto::RSA_KEY_2048;
static const Crypto::AESKeySize REQUIRED_AES_SIZE = Crypto::AES_KEY_256;
static const SizeT REQUIRED_PACKET_SIZE = ConnectPacket::HeaderType::ACTUAL_SIZE + 256;

ClientConnectionHandler::ClientConnectionHandler
(
    TaskScheduler* taskScheduler, 
    NetClientController* clientController,
    NetEventController* eventController,
    NetDriver* driver
)
: mTaskScheduler(taskScheduler)
, mClientController(clientController)
, mEventController(eventController)
, mDriver(driver)
, mAllocator()
{

}
ClientConnectionHandler::ClientConnectionHandler(ClientConnectionHandler&& other)
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
ClientConnectionHandler::~ClientConnectionHandler()
{
    CriticalAssert(mAllocator.GetHeap().GetHeapCount() == 0);
}
ClientConnectionHandler& ClientConnectionHandler::operator=(ClientConnectionHandler&& other)
{
    if (this != &other)
    {
        mTaskScheduler = other.mTaskScheduler;
        mClientController = other.mClientController;
        mEventController = other.mEventController;
        mDriver = other.mDriver;
        mAllocator = std::move(other.mAllocator);
        other.mTaskScheduler = nullptr;
        other.mClientController = nullptr;
        other.mEventController = nullptr;
        other.mDriver = nullptr;
    }
    return *this;
}

void ClientConnectionHandler::DecodePacket(PacketType* packetData)
{
    Crypto::RSAKey uniqueKey;
    ByteT challenge[ConnectPacket::CHALLENGE_SIZE];
    ByteT serverNonce[ConnectPacket::NONCE_SIZE];
    ConnectionID connectionID;
    ConnectPacket::AckHeaderType header;

    // todo: This gets called on a worker thread.. We might want to synchronize and dispatch with the 'client network thread' before we issue calls to them...

    if
    (
        !ConnectPacket::DecodeAckPacket
        (
            packetData->mBytes,
            packetData->mSize,
            mClientController->GetKey(),
            uniqueKey,
            mClientController->GetSharedKey(),
            mClientController->GetHMACKey(),
            challenge,
            serverNonce,
            connectionID, 
            header
        )
    )
    {
        NetConnectFailedEvent* event = mEventController->Allocate<NetConnectFailedEvent>();
        NET_EVENT_DEBUG_INFO(event);
        event->mReason = ConnectionFailureMsg::CFM_UNKNOWN;
        mDriver->SendEvent(event->GetType(), event);
        return;
    }

    if (memcmp(challenge, mClientController->GetChallenge(), sizeof(challenge)) != 0)
    {
        NetConnectFailedEvent* event = mEventController->Allocate<NetConnectFailedEvent>();
        NET_EVENT_DEBUG_INFO(event);
        event->mReason = ConnectionFailureMsg::CFM_UNKNOWN;
        mDriver->SendEvent(event->GetType(), event);
        return;
    }

    if (!mClientController->SetConnectionID(connectionID, std::move(uniqueKey), serverNonce))
    {
        NetConnectFailedEvent* event = mEventController->Allocate<NetConnectFailedEvent>();
        NET_EVENT_DEBUG_INFO(event);
        event->mReason = ConnectionFailureMsg::CFM_UNKNOWN;
        mDriver->SendEvent(event->GetType(), event);
        return;
    }
    else
    {

        NetConnectSuccessEvent* event = mEventController->Allocate<NetConnectSuccessEvent>();
        NET_EVENT_DEBUG_INFO(event);
        memcpy(event->mServerNonce, serverNonce, sizeof(serverNonce));
        mDriver->SendEvent(event->GetType(), event);
    }
}

void ClientConnectionHandler::OnInitialize()
{
    const SizeT POOL_OBJECT_COUNT = 256;
    const SizeT POOL_MAX_HEAPS = 3;
    const UInt32 POOL_FLAGS = PoolHeap::PHF_DOUBLE_FREE;

    CriticalAssert(mAllocator.Initialize(POOL_OBJECT_COUNT, POOL_MAX_HEAPS, POOL_FLAGS));
    CriticalAssert(mClientController->GetKey().GetKeySize() == REQUIRED_RSA_SIZE);
    CriticalAssert(mClientController->GetSharedKey().GetKeySize() == REQUIRED_AES_SIZE);
}

void ClientConnectionHandler::OnShutdown()
{
    mAllocator.Release();
}

void ClientConnectionHandler::OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender)
{
    if (!PacketUtility::IsAck(bytes, byteLength))
    {
        return;
    }

    if (byteLength < REQUIRED_PACKET_SIZE)
    {
        return;
    }

    PacketType* connectPacket = mAllocator.Allocate();
    if(!connectPacket)
    {
        return;
    }
    connectPacket->mSize = static_cast<UInt16>(byteLength);
    connectPacket->mType = NetPacketType::NET_PACKET_TYPE_CONNECT;
    connectPacket->mRetransmits = 0;
    connectPacket->mSender = sender;
    memcpy(connectPacket->mBytes, bytes, byteLength);
    
    mTaskScheduler->RunTask([connectPacket, this](void*)
    {
        DecodePacket(connectPacket);
        mAllocator.Free(connectPacket);
    });
}

void ClientConnectionHandler::OnUpdateFrame()
{
    mAllocator.GCCollect();
}

}
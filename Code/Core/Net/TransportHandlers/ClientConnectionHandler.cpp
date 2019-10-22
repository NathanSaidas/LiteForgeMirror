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
#include "Core/Net/NetClientController.h"
#include "Core/Net/PacketUtility.h"

#include <utility>

namespace lf {

// 256 == The assumed RSA key size in bytes.
static const Crypto::RSAKeySize REQUIRED_RSA_SIZE = Crypto::RSA_KEY_2048;
static const Crypto::AESKeySize REQUIRED_AES_SIZE = Crypto::AES_KEY_256;
static const SizeT REQUIRED_PACKET_SIZE = ConnectPacket::HeaderType::ACTUAL_SIZE + 256;

ClientConnectionHandler::ClientConnectionHandler(TaskScheduler* taskScheduler, NetClientController* clientController)
: mTaskScheduler(taskScheduler)
, mClientController(clientController)
, mPacketPool()
{

}
ClientConnectionHandler::ClientConnectionHandler(ClientConnectionHandler&& other)
: mTaskScheduler(other.mTaskScheduler)
, mClientController(other.mClientController)
, mPacketPool(std::move(other.mPacketPool))
{
    other.mTaskScheduler = nullptr;
    other.mClientController = nullptr;
}
ClientConnectionHandler::~ClientConnectionHandler()
{
    CriticalAssert(mPacketPool.GetHeapCount() == 0);
}
ClientConnectionHandler& ClientConnectionHandler::operator=(ClientConnectionHandler&& other)
{
    if (this != &other)
    {
        mTaskScheduler = other.mTaskScheduler;
        mClientController = other.mClientController;
        mPacketPool = std::move(other.mPacketPool);
        other.mTaskScheduler = nullptr;
        other.mClientController = nullptr;
    }
    return *this;
}

void ClientConnectionHandler::DecodePacket(ConnectAckPacketData* packetData)
{
    Crypto::RSAKey uniqueKey;
    ByteT challenge[ConnectPacket::CHALLENGE_SIZE];
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
            connectionID, 
            header
        )
    )
    {
        mClientController->OnConnectFailed(ConnectionFailureMsg::CFM_UNKNOWN);
        FreePacket(packetData);
        return;
    }

    if (memcmp(challenge, mClientController->GetChallenge(), sizeof(challenge)) != 0)
    {
        mClientController->OnConnectFailed(ConnectionFailureMsg::CFM_UNKNOWN);
        FreePacket(packetData);
        return;
    }

    mClientController->OnConnectSuccess(connectionID, std::move(uniqueKey));
    FreePacket(packetData);
}

void ClientConnectionHandler::OnInitialize()
{
    const SizeT POOL_OBJECT_COUNT = 256;
    const SizeT POOL_MAX_HEAPS = 3;
    const UInt32 POOL_FLAGS = PoolHeap::PHF_DOUBLE_FREE;

    CriticalAssert(mPacketPool.Initialize(sizeof(ConnectAckPacketData), alignof(ConnectAckPacketData), POOL_OBJECT_COUNT, POOL_MAX_HEAPS, POOL_FLAGS));
    CriticalAssert(mClientController->GetKey().GetKeySize() == REQUIRED_RSA_SIZE);
    CriticalAssert(mClientController->GetSharedKey().GetKeySize() == REQUIRED_AES_SIZE);
}

void ClientConnectionHandler::OnShutdown()
{
    mPacketPool.Release();
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

    ConnectAckPacketData* connectPacket = AllocatePacket();
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
    });
}

void ClientConnectionHandler::OnUpdateFrame()
{
    mPacketPool.GCCollect();
}

ClientConnectionHandler::ConnectAckPacketData* ClientConnectionHandler::AllocatePacket()
{
    void* object = mPacketPool.Allocate();
    if (object)
    {
        return new(object)ConnectAckPacketData();
    }
    return nullptr;
}

void ClientConnectionHandler::FreePacket(ConnectAckPacketData* packet)
{
    if (packet)
    {
        packet->~ConnectAckPacketData();
        PacketData::SetZero(*packet);
        mPacketPool.Free(packet);
    }
}

}
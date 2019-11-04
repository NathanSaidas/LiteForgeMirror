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

#include "Core/Test/Test.h"

#include "Core/Concurrent/TaskScheduler.h"

#include "Core/Net/Controllers/NetClientController.h"
#include "Core/Net/Controllers/NetConnectionController.h"
#include "Core/Net/Controllers/NetServerController.h"
#include "Core/Net/Controllers/NetEventController.h"
#include "Core/Net/NetClientDriver.h"
#include "Core/Net/NetConnection.h"
#include "Core/Net/NetEvent.h"
#include "Core/Net/NetFramework.h"
#include "Core/Net/NetServerDriver.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/NetTransport.h"
#include "Core/Net/NetTransportHandler.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Net/TransportHandlers/ServerConnectionHandler.h"
#include "Core/Net/TransportHandlers/ClientConnectionHandler.h"
#include "Core/Net/ConnectPacket.h"
#include "Core/Net/HeartbeatPacket.h"

#include "Core/Memory/PoolHeap.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/AtomicSmartPointer.h"

#include "Core/Crypto/SecureRandom.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Crypto/HMAC.h"

#include "Core/Math/Random.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Time.h"
#include "Core/Utility/UniqueNumber.h"
#include <utility>

#include "Core/String/StringCommon.h"

namespace lf {

const SizeT TEST_PORT = 27015;
const char* TEST_IPV4_TARGET = "127.0.0.1";
const char* TEST_IPV6_TARGET = "::1";

struct NetTestInitializer
{
    NetTestInitializer() :
        mRelease(!IsNetInitialized())
    {
        if (mRelease) { TEST(NetInitialize()); }
    }
    ~NetTestInitializer()
    {
        if (mRelease) { TEST(NetShutdown()); }
    }
    bool mRelease;
};

struct TestClientState
{
    Crypto::AESKey mSharedKey;
    Crypto::RSAKey mClientKey; // Public/Private
    Crypto::RSAKey mServerKey; // Public Only
    
    ByteT mHMACKey[Crypto::HMAC_KEY_SIZE];
    Crypto::RSAKey mUniqueKey; // Public Only

    ByteT mChallenge[32];
};

struct TestServerState
{
    Crypto::AESKey mSharedKey;
    Crypto::RSAKey mClientKey; // Public Only
    Crypto::RSAKey mUniqueKey; // Public/Private

    ByteT mHMACKey[Crypto::HMAC_KEY_SIZE];
    Crypto::RSAKey mServerKey; // Public/Private

    ByteT mChallenge[NET_CLIENT_CHALLENGE_SIZE];
    ByteT mServerNonce[NET_HEARTBEAT_NONCE_SIZE];
    ByteT mClientNonce[NET_HEARTBEAT_NONCE_SIZE];
};

using TestPacketType = TPacketData<4096>;
using TestHeaderType = PacketHeader;
using TestAckHeaderType = AckPacketHeader;

void InitStates(TestClientState& client, TestServerState& server)
{
    Crypto::AESKey sharedKey;
    Crypto::RSAKey clientKey;
    Crypto::RSAKey serverKey;

    sharedKey.Generate(Crypto::AES_KEY_256);
    clientKey.GeneratePair(Crypto::RSA_KEY_2048);
    serverKey.GeneratePair(Crypto::RSA_KEY_2048);

    client.mSharedKey = sharedKey;
    client.mClientKey = clientKey;
    TEST(client.mServerKey.LoadPublicKey(serverKey.GetPublicKey()));

    server.mServerKey = serverKey;

    TEST(client.mSharedKey.GetKeySize() == Crypto::AES_KEY_256);
    TEST(client.mClientKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(client.mClientKey.HasPublicKey());
    TEST(client.mClientKey.HasPrivateKey());
    TEST(client.mServerKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(client.mServerKey.HasPublicKey());
    TEST(!client.mServerKey.HasPrivateKey());
    TEST(client.mUniqueKey.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(!client.mUniqueKey.HasPublicKey());
    TEST(!client.mUniqueKey.HasPrivateKey());

    TEST(server.mSharedKey.GetKeySize() == Crypto::AES_KEY_Unknown);
    TEST(server.mClientKey.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(!server.mClientKey.HasPrivateKey());
    TEST(!server.mClientKey.HasPublicKey());
    TEST(server.mServerKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(server.mServerKey.HasPublicKey());
    TEST(server.mServerKey.HasPrivateKey());
    TEST(server.mUniqueKey.GetKeySize() == Crypto::RSA_KEY_Unknown);
    TEST(!server.mUniqueKey.HasPublicKey());
    TEST(!server.mUniqueKey.HasPrivateKey());
}

void TestClientServerCommunication(TestClientState& client, TestServerState& server)
{
    // Quick Sanity Check:

    // Shared Key:
    TEST(client.mSharedKey.GetKeySizeBytes() == server.mSharedKey.GetKeySizeBytes());
    TEST(memcmp(client.mSharedKey.GetKey(), server.mSharedKey.GetKey(), server.mSharedKey.GetKeySizeBytes()) == 0);

    // HMAC Key:
    TEST(memcmp(client.mHMACKey, server.mHMACKey, Crypto::HMAC_KEY_SIZE) == 0);

    // Client Key:
    TEST(client.mClientKey.HasPublicKey());
    TEST(client.mClientKey.HasPrivateKey());
    TEST(server.mClientKey.HasPublicKey());
    TEST(!server.mClientKey.HasPrivateKey());
    TEST(client.mClientKey.GetPublicKey() == server.mClientKey.GetPublicKey());

    // Server Key:
    TEST(client.mServerKey.HasPublicKey());
    TEST(!client.mServerKey.HasPrivateKey());
    TEST(server.mServerKey.HasPublicKey());
    TEST(server.mServerKey.HasPrivateKey());
    TEST(client.mServerKey.GetPublicKey() == server.mServerKey.GetPublicKey());

    // Unique Key:
    TEST(client.mUniqueKey.HasPublicKey());
    TEST(!client.mUniqueKey.HasPrivateKey());
    TEST(server.mUniqueKey.HasPublicKey());
    TEST(server.mUniqueKey.HasPrivateKey());
    TEST(client.mUniqueKey.GetPublicKey() == server.mUniqueKey.GetPublicKey());


    ByteT IV[16];
    Crypto::SecureRandomBytes(IV, sizeof(IV));

    ByteT randomMessage[1500];
    Crypto::SecureRandomBytes(randomMessage, sizeof(randomMessage));

    ByteT cipherText[4096];
    SizeT cipherTextLength = sizeof(cipherText);

    ByteT plainText[4096];
    SizeT plainTextLength = sizeof(plainText);

    // Client <-> Server | SharedKey
    TEST(Crypto::AESEncrypt(&client.mSharedKey, IV, randomMessage, sizeof(randomMessage), cipherText, cipherTextLength));
    TEST(Crypto::AESDecrypt(&server.mSharedKey, IV, cipherText, cipherTextLength, plainText, plainTextLength));

    TEST(memcmp(randomMessage, cipherText, sizeof(randomMessage)) != 0);
    TEST(memcmp(randomMessage, plainText, sizeof(randomMessage)) == 0);
    TEST(plainTextLength == sizeof(randomMessage));

    // Client -> Server | ServerKey
    cipherTextLength = sizeof(cipherText);
    plainTextLength = sizeof(plainText);

    const SizeT RSA_SIZE = 96;
    TEST(Crypto::RSAEncryptPublic(&client.mServerKey, randomMessage, RSA_SIZE, cipherText, cipherTextLength));
    TEST(Crypto::RSADecryptPrivate(&server.mServerKey, cipherText, cipherTextLength, plainText, plainTextLength));

    TEST(memcmp(randomMessage, cipherText, RSA_SIZE) != 0);
    TEST(memcmp(randomMessage, plainText, RSA_SIZE) == 0);
    TEST(plainTextLength == RSA_SIZE);

    // Client <- Server | ClientKey
    cipherTextLength = sizeof(cipherText);
    plainTextLength = sizeof(plainText);

    TEST(Crypto::RSAEncryptPublic(&server.mClientKey, randomMessage, RSA_SIZE, cipherText, cipherTextLength));
    TEST(Crypto::RSADecryptPrivate(&client.mClientKey, cipherText, cipherTextLength, plainText, plainTextLength));

    TEST(memcmp(randomMessage, cipherText, RSA_SIZE) != 0);
    TEST(memcmp(randomMessage, plainText, RSA_SIZE) == 0);
    TEST(plainTextLength == RSA_SIZE);

    cipherTextLength = sizeof(cipherText);
    plainTextLength = sizeof(plainText);

    TEST(Crypto::RSAEncryptPublic(&client.mUniqueKey, randomMessage, RSA_SIZE, cipherText, cipherTextLength));
    TEST(Crypto::RSADecryptPrivate(&server.mUniqueKey, cipherText, cipherTextLength, plainText, plainTextLength));

    TEST(memcmp(randomMessage, cipherText, RSA_SIZE) != 0);
    TEST(memcmp(randomMessage, plainText, RSA_SIZE) == 0);
    TEST(plainTextLength == RSA_SIZE);
}

REGISTER_TEST(ClientServerConnectionTest)
{
    TestClientState client;
    TestServerState server;
    TestPacketType connectPacket;
    TestPacketType ackPacket;

    PacketData::SetZero(connectPacket);
    PacketData::SetZero(ackPacket);
    InitStates(client, server);
    Crypto::SecureRandomBytes(client.mChallenge, sizeof(client.mChallenge));
    Crypto::SecureRandomBytes(client.mHMACKey, sizeof(client.mHMACKey));

    connectPacket.mSize = sizeof(connectPacket.mBytes);
    SizeT size = connectPacket.mSize;
    // The client creates a connection message to send to the server
    bool r = ConnectPacket::EncodePacket
        (
            connectPacket.mBytes, 
            size,
            client.mClientKey,
            client.mServerKey,
            client.mSharedKey,
            client.mHMACKey,
            client.mChallenge
        );
    TEST(r);

    // The server can verify authenticity of protected via hmac
    ConnectPacket::HeaderType header;
    r = ConnectPacket::DecodePacket
        (
            connectPacket.mBytes,
            size,
            server.mServerKey,
            server.mClientKey,
            server.mSharedKey,
            server.mHMACKey,
            server.mChallenge,
            header
        );
    TEST(r);


    ackPacket.mSize = sizeof(ackPacket.mBytes);
    size = ackPacket.mSize;
    TEST(server.mUniqueKey.GeneratePair(Crypto::RSA_KEY_2048));
    // The server can then acknowledge the client with some sort of status
    r = ConnectPacket::EncodeAckPacket
        (
            ackPacket.mBytes,
            size,
            server.mClientKey,
            server.mUniqueKey,
            server.mSharedKey,
            server.mHMACKey,
            server.mChallenge,
            server.mServerNonce,
            47
        );
    TEST(r);

    ConnectionID connectionID = 0;
    ConnectPacket::AckHeaderType ackHeader;
    ByteT challenge[ConnectPacket::CHALLENGE_SIZE];
    ByteT serverNonce[ConnectPacket::NONCE_SIZE];
    // The client can verify authenticity of server by verifying their challenge and hmac
    r = ConnectPacket::DecodeAckPacket
        (
            ackPacket.mBytes,
            size,
            client.mClientKey,
            client.mUniqueKey,
            client.mSharedKey,
            client.mHMACKey,
            challenge,
            serverNonce,
            connectionID,
            ackHeader
        );
    TEST(r);
    TEST(connectionID == 47);
    TEST(memcmp(serverNonce, server.mServerNonce, sizeof(serverNonce)) == 0);

    // The client is now able to communicate with the server until they timeout or are evicted.
    TEST(client.mClientKey.GetPublicKey() == server.mClientKey.GetPublicKey());
    TEST(client.mServerKey.GetPublicKey() == server.mServerKey.GetPublicKey());
    TestClientServerCommunication(client, server);
}

static void ClearCache(ByteT* data, SizeT dataSize, ByteT val)
{
    for (SizeT i = 0; i < dataSize; ++i)
    {
        data[i] = val;
    }
}

REGISTER_TEST(TestLookUpTime)
{
    if (!TestFramework::GetConfig().mStress)
    {
        gTestLog.Info(LogMessage("Ignoring test, stress tests not enabled..."));
        return;
    }

    SetCTitle("My Console Title");

    const SizeT cacheSize = 16 * 1024 * 1024;
    ByteT* cache = static_cast<ByteT*>(LFAlloc(cacheSize, 4));
    UInt32 packetIds[5000];

    gTestLog.Info(LogMessage("Building Tracker..."));
    gTestLog.Sync();
    TMap<UInt32, SizeT> tracker;
    while (tracker.size() != 5000)
    {
        UInt32 id;
        Crypto::SecureRandomBytes(reinterpret_cast<ByteT*>(&id), sizeof(id));
        tracker[id] = 0;
    }

    SizeT n = 0;
    for (auto kv : tracker)
    {
        packetIds[n++] = kv.first;
    }

    Int32 seed = 0xDAF2C33D;
    const SizeT iterations = 1000; //  20 * 5 * 60 * 100;
    Float64* times = static_cast<Float64*>(LFAlloc(sizeof(Float64) * iterations, alignof(Float64)));
    Float64 tmin = FLT_MAX;
    Float64 tmax = FLT_MIN;
    Float64 total = 0.0;
    Int64 start;
    Int64 end;
    Int64 frequency = GetClockFrequency();

    gTestLog.Info(LogMessage("Running benchmark..."));
    gTestLog.Sync();
    for (SizeT i = 0; i < iterations; ++i)
    {
        UInt32 id = packetIds[Random::Range(seed, 0, LF_ARRAY_SIZE(packetIds) - 1)];
        (id);
        // ClearCache(cache, 0, 0);
        ClearCache(cache, cacheSize, static_cast<ByteT>(i & 0xFF));
        start = GetClockTime();
        tracker[id]++;
        // PacketUtility::CalcCrc32(reinterpret_cast<ByteT*>(packetIds), sizeof(packetIds));
        end = GetClockTime();
        Float64 actualTime = (end - start) / static_cast<Float64>(frequency);
        tmin = Min(tmin, actualTime);
        tmax = Max(tmax, actualTime);
        times[i] = actualTime;

        SStream ss;
        ss << "Running benchmark " << i << "/" << iterations << "...";
        SetCTitle(ss.Str().CStr());
    }

    gTestLog.Info(LogMessage("Generating Results..."));
    gTestLog.Sync();
    for (SizeT i = 0; i < iterations; ++i)
    {
        total += times[i];
    }

    Float64 average = total / iterations;

    LoggerMessage msg(__FILE__, __LINE__, "");
    msg << "\nIterations = " << iterations;
    msg << "\nTotal = " << total;
    msg << "\nMin = " << tmin * 1000.0 << " (ms)";
    msg << "\nMax = " << tmax * 1000.0 << " (ms)";
    msg << "\nAverage = " << average * 1000.0 << " (ms)";
    gTestLog.Info(msg);

    LFFree(times);
    LFFree(cache);
    // Bottleneck Test
    // CalcCrc32
}

struct HeartbeatNonce
{
    ByteT client[HeartbeatPacket::MESSAGE_SIZE];
    ByteT server[HeartbeatPacket::MESSAGE_SIZE];
};
using HeartbeatData = PacketData1024;

struct HeartbeatState
{
    void ClientToServer(HeartbeatNonce& c, HeartbeatNonce& s, HeartbeatData& packet, HeartbeatPacket::HeaderType& serverHeader);
    void ServerToClient(HeartbeatNonce& c, HeartbeatNonce& s, HeartbeatData& packet, const HeartbeatPacket::HeaderType& serverHeader);


    Crypto::RSAKey mUniqueKey;
    Crypto::RSAKey mClientKey;
};

void HeartbeatState::ClientToServer(HeartbeatNonce& c, HeartbeatNonce& s, HeartbeatData& packet, HeartbeatPacket::HeaderType& serverHeader)
{
    // As the client we should generate our own nonce
    Crypto::SecureRandomBytes(c.client, sizeof(c.client));

    // As the client we should be able to encode a packet.
    SizeT packetSize = sizeof(packet.mBytes);
    TEST_CRITICAL(HeartbeatPacket::EncodePacket
    (
        packet.mBytes,
        packetSize,
        mUniqueKey,
        c.client,
        c.server,
        0,
        0
    ));
    packet.mSize = static_cast<UInt16>(packetSize);

    // As the server we should be able to decode the data.
    TEST_CRITICAL(HeartbeatPacket::DecodePacket
    (
        packet.mBytes,
        packetSize,
        mUniqueKey,
        s.client,
        s.server,
        serverHeader
    ));

    // As the server we must confirm the snonce
    TEST(memcmp(s.server, c.server, sizeof(c.server)) == 0);

    // As the server we must generate a new snonce
    Crypto::SecureRandomBytes(s.server, sizeof(s.server));
}

void HeartbeatState::ServerToClient(HeartbeatNonce& c, HeartbeatNonce& s, HeartbeatData& packet, const HeartbeatPacket::HeaderType& serverHeader)
{

    // As a server we must acknowledge the client
    SizeT packetSize = sizeof(packet.mBytes);
    TEST_CRITICAL(HeartbeatPacket::EncodeAckPacket
    (
        packet.mBytes,
        packetSize,
        mClientKey,
        s.client,
        s.server,
        GetPacketUID(serverHeader)
    ));
    packet.mSize = static_cast<UInt16>(packetSize);

    // As the client we must decode the ack
    HeartbeatPacket::AckHeaderType clientHeader;
    UInt32 packetUID = 0;
    TEST_CRITICAL(HeartbeatPacket::DecodeAckPacket
    (
        packet.mBytes,
        packetSize,
        mClientKey,
        c.client,
        c.server,
        packetUID,
        clientHeader
    ));

    // As the client we must confirm the cnonce
    TEST(memcmp(c.client, s.client, sizeof(c.client)) == 0);
}

REGISTER_TEST(HeartbeatPacketTest)
{
    if (!TestFramework::GetConfig().mStress)
    {
        gTestLog.Info(LogMessage("Ignoring test, stress tests not enabled..."));
        return;
    }

    HeartbeatState state;
    HeartbeatNonce c, s;
    

    TEST_CRITICAL(state.mUniqueKey.GeneratePair(Crypto::RSA_KEY_2048));
    TEST_CRITICAL(state.mClientKey.GeneratePair(Crypto::RSA_KEY_2048));

    // As the client we should've received a nonce from the server while establishing an secure connection.
    Crypto::SecureRandomBytes(c.server, sizeof(c.server));
    memcpy(s.server, c.server, sizeof(c.server));

    for (SizeT i = 0; i < 100007; ++i)
    {
        HeartbeatData packet;
        HeartbeatPacket::HeaderType serverHeader;
        state.ClientToServer(c, s, packet, serverHeader);
        state.ServerToClient(c, s, packet, serverHeader);
    }


}

REGISTER_TEST(NetEventTest)
{
    NetEventController eventController;
    TEST_CRITICAL(eventController.Initialize());

    // Test all events can be allocated/written/read/freed without issue.
    {
        NetConnectSuccessEvent* event = eventController.Allocate<NetConnectSuccessEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_CONNECT_SUCCESS);
        Crypto::SecureRandomBytes(event->mServerNonce, sizeof(event->mServerNonce));
        eventController.Free(event);
    }
    
    {
        NetConnectFailedEvent* event = eventController.Allocate<NetConnectFailedEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_CONNECT_FAILED);
        event->mReason = 1;
        eventController.Free(event);
    }

    {
        NetConnectionCreatedEvent* event = eventController.Allocate<NetConnectionCreatedEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_CONNECTION_CREATED);
        event->mConnectionID = 1;
        eventController.Free(event);
    }

    {
        NetConnectionTerminatedEvent* event = eventController.Allocate<NetConnectionTerminatedEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_CONNECTION_TERMINATED);
        event->mReason = 1;
        eventController.Free(event);
    }

    {
        NetHeartbeatReceivedEvent* event = eventController.Allocate<NetHeartbeatReceivedEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_HEARTBEAT_RECEIVED);
        Crypto::SecureRandomBytes(event->mNonce, sizeof(event->mNonce));
        eventController.Free(event);
    }

    {
        NetDataReceivedRequestEvent* event = eventController.Allocate<NetDataReceivedRequestEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_DATA_RECEIVED_REQUEST);
        eventController.Free(event);
    }

    {
        NetDataReceivedResponseEvent* event = eventController.Allocate<NetDataReceivedResponseEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_DATA_RECEIVED_RESPONSE);
        eventController.Free(event);
    }

    {
        NetDataReceivedActionEvent* event = eventController.Allocate<NetDataReceivedActionEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_DATA_RECEIVED_ACTION);
        eventController.Free(event);
    }

    {
        NetDataReceivedReplicationEvent* event = eventController.Allocate<NetDataReceivedReplicationEvent>();
        TEST_CRITICAL(event != nullptr);
        TEST(event->GetType() == NetEventType::NET_EVENT_DATA_RECEIVED_REPLICATION);
        eventController.Free(event);
    }

    eventController.Reset();
}


REGISTER_TEST(NetClientDriverTest)
{
    NetTestInitializer initializer;
    NetClientDriver client;

    {
        Crypto::RSAKey key;
        TEST_CRITICAL(key.GeneratePair(Crypto::RSA_KEY_2048));

        IPEndPointAny localIP;
        TEST_CRITICAL(IPCast(IPV6(TEST_IPV6_TARGET, TEST_PORT), localIP));

        TEST_CRITICAL(client.Initialize(std::move(key), localIP, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION));
        client.Shutdown();
    }
}

REGISTER_TEST(NetServerDriverTest)
{
    NetTestInitializer initializer;
    NetServerDriver server;

    {
        Crypto::RSAKey key;
        TEST_CRITICAL(key.GeneratePair(Crypto::RSA_KEY_2048));

        TEST_CRITICAL(server.Initialize(std::move(key), TEST_PORT, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION));
        server.Shutdown();
    }
}

REGISTER_TEST(NetClientServerConnectionTest)
{
    NetTestInitializer initializer;
    NetClientDriver client;
    NetServerDriver server;
    Crypto::RSAKey key;
    TEST_CRITICAL(key.GeneratePair(Crypto::RSA_KEY_2048));

    TEST_CRITICAL(server.Initialize(key, TEST_PORT, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION));

    IPEndPointAny localIP;
    TEST_CRITICAL(IPCast(IPV6(TEST_IPV6_TARGET, TEST_PORT), localIP));
    TEST_CRITICAL(client.Initialize(key, localIP, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION));
    
    while (!client.IsConnected())
    {
        SleepCallingThread(0);
    }

    SizeT heartbeatsEmitted = 0;
    Int64 frequency = GetClockFrequency();
    Int64 time = GetClockTime();
    Float64 elapsed = 0.0;
    Int64 heartbeatTime = GetClockTime();
    Float64 heartbeatElapsed = 0.0;
    bool serverKicked = false;
    while (elapsed < 10.0)
    {
        elapsed = (GetClockTime() - time) / static_cast<Float64>(frequency);
        heartbeatElapsed = (GetClockTime() - heartbeatTime) / static_cast<Float64>(frequency);

        if (elapsed > 1.0 && !serverKicked)
        {
            server.DropConnection(0);
            serverKicked = true;
        }

        if (heartbeatElapsed > 0.1)
        {
            if (client.EmitHeartbeat(true))
            {
                heartbeatElapsed = 0.0;
                heartbeatTime = GetClockTime();
                ++heartbeatsEmitted;
                gTestLog.Info(LogMessage("Emitting heartbeat..."));
            }
        }
        server.Update();
        client.Update();
    }

    gTestLog.Info(LogMessage("Emitted ") << heartbeatsEmitted << " heartbeats.");
    client.Shutdown();
    server.Shutdown();
    LF_DEBUG_BREAK;
}

} // namespace lf
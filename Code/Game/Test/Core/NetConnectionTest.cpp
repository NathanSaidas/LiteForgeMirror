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
#include "Core/Net/NetClientController.h"
#include "Core/Net/NetConnection.h"
#include "Core/Net/NetConnectionController.h"
#include "Core/Net/NetFramework.h"
#include "Core/Net/NetServerController.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/NetTransport.h"
#include "Core/Net/NetTransportHandler.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Net/TransportHandlers/ClientConnectionHandler.h"
#include "Core/Net/TransportHandlers/ServerConnectionHandler.h"


#include "Core/Net/ConnectPacket.h"
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

    ByteT mChallenge[32];
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


void CreateConnectMessage(TestPacketType& packet, TestClientState& client)
{
    TEST(client.mSharedKey.GetKeySize() == Crypto::AES_KEY_256);
    TEST(client.mServerKey.HasPublicKey());
    TEST(client.mServerKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST(client.mClientKey.HasPublicKey());
    TEST(client.mClientKey.GetKeySize() == Crypto::RSA_KEY_2048);

    struct RSAMessage
    {
        ByteT mHMACKey[Crypto::HMAC_KEY_SIZE];
        ByteT mIV[16];
        ByteT mSharedKey[32];
        ByteT mHMAC[Crypto::HMAC_HASH_SIZE];
        ByteT mChallenge[32];
    };

    struct AESMessage
    {
        String mPublicKey;
    };

    const SizeT SERVER_KEY_SIZE_BYTES = client.mServerKey.GetKeySizeBytes();

    RSAMessage rsa;
    Crypto::SecureRandomBytes(rsa.mHMACKey, LF_ARRAY_SIZE(rsa.mHMACKey));
    Crypto::SecureRandomBytes(rsa.mIV, LF_ARRAY_SIZE(rsa.mIV));
    memcpy(rsa.mSharedKey, client.mSharedKey.GetKey(), client.mSharedKey.GetKeySizeBytes());
    Crypto::SecureRandomBytes(rsa.mChallenge, LF_ARRAY_SIZE(rsa.mChallenge));
    memcpy(client.mChallenge, rsa.mChallenge, sizeof(rsa.mChallenge));
    memcpy(client.mHMACKey, rsa.mHMACKey, sizeof(rsa.mHMACKey));

    gTestLog.Info(LogMessage("Challenge=0x") << BytesToHex(rsa.mChallenge, LF_ARRAY_SIZE(rsa.mChallenge)));
    gTestLog.Sync();

    AESMessage aes;
    aes.mPublicKey = client.mClientKey.GetPublicKey();

    // [header]
    // [rsa]
    // [aes]

    // Write Header
    TestHeaderType* header = reinterpret_cast<TestHeaderType*>(packet.mBytes);
    header->mAppID = NetConfig::NET_APP_ID;
    header->mAppVersion = NetConfig::NET_APP_VERSION;
    header->mType = NetPacketType::NET_PACKET_TYPE_CONNECT;
    header->mFlags = 0; // Connect doesn't use flags, the decoding/response is implicit.

    // Write AES Block
    const ByteT* inBytes = reinterpret_cast<const ByteT*>(aes.mPublicKey.CStr());
    ByteT* outBytes = &header->mPadding[SERVER_KEY_SIZE_BYTES];
    SizeT outBytesCapacity = LF_ARRAY_SIZE(TestPacketType::mBytes) - (TestHeaderType::ACTUAL_SIZE + SERVER_KEY_SIZE_BYTES);
    TEST(Crypto::AESEncrypt(&client.mSharedKey, rsa.mIV, inBytes, aes.mPublicKey.Size(), outBytes, outBytesCapacity));

    // Compute HMAC of AES block
    TEST(Crypto::HMACCompute(rsa.mHMACKey, outBytes, outBytesCapacity, rsa.mHMAC));

    // Write RSA Block
    SizeT outEncryptedCapacity = LF_ARRAY_SIZE(TestPacketType::mBytes) - (TestHeaderType::ACTUAL_SIZE + outBytesCapacity);
    TEST(outEncryptedCapacity >= SERVER_KEY_SIZE_BYTES);
    TEST(Crypto::RSAEncryptPublic(&client.mServerKey, reinterpret_cast<const ByteT*>(&rsa), sizeof(RSAMessage), &header->mPadding[0], outEncryptedCapacity));

    const SizeT totalSize = TestHeaderType::ACTUAL_SIZE + outEncryptedCapacity + outBytesCapacity;
    header->mCrc32 = PacketUtility::CalcCrc32(packet.mBytes, totalSize);

    packet.mType = header->mType;
    packet.mSize = static_cast<UInt16>(totalSize);
    packet.mRetransmits = 0;
}

void ParseConnectMessage(TestPacketType& packet, TestServerState& server)
{
    TEST(server.mServerKey.HasPrivateKey());
    TEST(server.mServerKey.GetKeySize() == Crypto::RSA_KEY_2048);

    struct RSAMessage
    {
        ByteT mHMACKey[Crypto::HMAC_KEY_SIZE];
        ByteT mIV[16];
        ByteT mSharedKey[32];
        ByteT mHMAC[Crypto::HMAC_HASH_SIZE];
        ByteT mChallenge[32];
    };

    struct AESMessage
    {
        String mPublicKey;
    };

    const SizeT SERVER_KEY_SIZE_BYTES = server.mServerKey.GetKeySizeBytes();

    TestHeaderType* header = reinterpret_cast<TestHeaderType*>(packet.mBytes);
    TEST(header->mAppID == NetConfig::NET_APP_ID);
    TEST(header->mAppVersion == NetConfig::NET_APP_VERSION);
    TEST(header->mType == NetPacketType::NET_PACKET_TYPE_CONNECT);

    const SizeT rsaBlockSize = SERVER_KEY_SIZE_BYTES;
    const SizeT aesBlockSize = packet.mSize - (TestHeaderType::ACTUAL_SIZE + rsaBlockSize);

    // Read RSA Block
    RSAMessage rsa;
    SizeT rsaMessageSize = SERVER_KEY_SIZE_BYTES;
    TEST(Crypto::RSADecryptPrivate(&server.mServerKey, &header->mPadding[0], rsaBlockSize, reinterpret_cast<ByteT*>(&rsa), rsaMessageSize));

    gTestLog.Info(LogMessage("Challenge=0x") << BytesToHex(rsa.mChallenge, LF_ARRAY_SIZE(rsa.mChallenge)));
    gTestLog.Sync();

    ByteT* aesBytes = &header->mPadding[SERVER_KEY_SIZE_BYTES];

    // Verify HMAC
    ByteT hmac[Crypto::HMAC_HASH_SIZE];
    TEST(Crypto::HMACCompute(rsa.mHMACKey, aesBytes, aesBlockSize, hmac));
    TEST(memcmp(rsa.mHMAC, hmac, Crypto::HMAC_HASH_SIZE) == 0);

    TEST(server.mSharedKey.Load(Crypto::AES_KEY_256, rsa.mSharedKey));

    // Read AES Block
    ByteT decryptedAESBlock[LF_ARRAY_SIZE(TestPacketType::mBytes)];
    SizeT decryptedAESBlockSize = sizeof(decryptedAESBlock);
    TEST(Crypto::AESDecrypt(&server.mSharedKey, rsa.mIV, aesBytes, aesBlockSize, decryptedAESBlock, decryptedAESBlockSize));

    String clientKey(decryptedAESBlockSize, reinterpret_cast<const char*>(decryptedAESBlock), COPY_ON_WRITE);
    TEST(server.mClientKey.LoadPublicKey(clientKey));

    memcpy(server.mChallenge, rsa.mChallenge, sizeof(rsa.mChallenge));
    memcpy(server.mHMACKey, rsa.mHMACKey, Crypto::HMAC_KEY_SIZE);
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

void CreateConnectResponseMessage(TestPacketType& packet, TestServerState& server)
{
    // todo: 
    // use NET_PACKET_FLAG_ACK
    // use AckPacketHeaderType
    // echo back their RSA Message
    // 
    // ? How do we communicate status? Global value?

    server.mUniqueKey.GeneratePair(Crypto::RSA_KEY_2048);


    struct RSAMessage
    {
        ByteT mIV[16];
        ByteT mHMAC[Crypto::HMAC_HASH_SIZE];
        ByteT mChallenge[32];
    };

    struct AESMessage
    {
        String mPublicKey;
    };

    const SizeT SERVER_KEY_SIZE_BYTES = server.mServerKey.GetKeySizeBytes();

    TestAckHeaderType* header = reinterpret_cast<TestAckHeaderType*>(packet.mBytes);
    header->mAppID = NetConfig::NET_APP_ID;
    header->mAppVersion = NetConfig::NET_APP_VERSION;
    header->mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
    header->mType = NetPacketType::NET_PACKET_TYPE_CONNECT;
    header->mStatus = NetAckStatus::NET_ACK_STATUS_OK;
    
    RSAMessage rsa;
    Crypto::SecureRandomBytes(rsa.mIV, LF_ARRAY_SIZE(rsa.mIV));
    memcpy(rsa.mChallenge, server.mChallenge, sizeof(rsa.mChallenge));

    AESMessage aes;
    aes.mPublicKey = server.mUniqueKey.GetPublicKey();

    const ByteT* inBytes = reinterpret_cast<const ByteT*>(aes.mPublicKey.CStr());
    const SizeT inBytesLength = aes.mPublicKey.Size();
    ByteT* aesOutputBytes = &(&header->mPadding)[SERVER_KEY_SIZE_BYTES];
    SizeT aesOutputBytesLength = sizeof(packet.mBytes) - (TestAckHeaderType::ACTUAL_SIZE + SERVER_KEY_SIZE_BYTES);

    // AES Block
    TEST(Crypto::AESEncrypt(&server.mSharedKey, rsa.mIV, inBytes, inBytesLength, aesOutputBytes, aesOutputBytesLength));

    // HMAC
    TEST(Crypto::HMACCompute(server.mHMACKey, aesOutputBytes, aesOutputBytesLength, rsa.mHMAC));

    // RSA Block:
    SizeT rsaOutputBytesLength = SERVER_KEY_SIZE_BYTES;
    TEST(Crypto::RSAEncryptPublic(&server.mClientKey, reinterpret_cast<const ByteT*>(&rsa), sizeof(rsa), &header->mPadding, rsaOutputBytesLength));
    TEST(rsaOutputBytesLength == SERVER_KEY_SIZE_BYTES);

    SizeT totalSize = static_cast<UInt16>(TestAckHeaderType::ACTUAL_SIZE + rsaOutputBytesLength + aesOutputBytesLength);
    header->mCrc32 = PacketUtility::CalcCrc32(packet.mBytes, totalSize);

    packet.mType = header->mType;
    packet.mSize = static_cast<UInt16>(totalSize);
    packet.mRetransmits = 0;
}

void ParseConnectResponseMessage(TestPacketType& packet, TestClientState& client)
{
    TestAckHeaderType* header = reinterpret_cast<TestAckHeaderType*>(packet.mBytes);
    TEST(NetPacketFlag::BitfieldType(header->mFlags).Has(NetPacketFlag::NET_PACKET_FLAG_ACK));
    TEST(header->mType == NetPacketType::NET_PACKET_TYPE_CONNECT);
    TEST(header->mStatus == NetAckStatus::NET_ACK_STATUS_OK);
    TEST(header->mCrc32 == PacketUtility::CalcCrc32(packet.mBytes, packet.mSize));

    const SizeT rsaSize = client.mClientKey.GetKeySizeBytes();
    const SizeT aesSize = packet.mSize - (TestAckHeaderType::ACTUAL_SIZE + rsaSize);

    const ByteT* rsaBytes = &header->mPadding;
    const ByteT* aesBytes = &rsaBytes[rsaSize];

    ByteT hmac[Crypto::HMAC_HASH_SIZE];
    TEST(Crypto::HMACCompute(client.mHMACKey, aesBytes, aesSize, hmac));

    ByteT rsaDecrypted[4096];
    SizeT rsaDecryptedSize = sizeof(rsaDecrypted);
    TEST(Crypto::RSADecryptPrivate(&client.mClientKey, rsaBytes, rsaSize, rsaDecrypted, rsaDecryptedSize));

    struct RSAMessage
    {
        ByteT mIV[16];
        ByteT mHMAC[Crypto::HMAC_HASH_SIZE];
        ByteT mChallenge[32];
    };

    RSAMessage* rsa = reinterpret_cast<RSAMessage*>(rsaDecrypted);
    TEST(memcmp(rsa->mChallenge, client.mChallenge, sizeof(rsa->mChallenge)) == 0);
    TEST(memcmp(rsa->mHMAC, hmac, sizeof(hmac)) == 0);
    
    ByteT aesDecrypted[4096];
    SizeT aesDecryptedSize = sizeof(aesDecrypted);
    TEST(Crypto::AESDecrypt(&client.mSharedKey, rsa->mIV, aesBytes, aesSize, aesDecrypted, aesDecryptedSize));

    String publickey(aesDecryptedSize, reinterpret_cast<const char*>(aesDecrypted), COPY_ON_WRITE);
    TEST(client.mUniqueKey.LoadPublicKey(publickey));

}

REGISTER_TEST(ClientServerConnectionTest)
{
    TestClientState client;
    TestServerState server;
    TestPacketType connectPacket;
    TestPacketType ackPacket;

    // The server has their own public/private RSA key for initial communications
    // The client must generate their own key pair, the client is assumed to know the server public key
    InitStates(client, server);

    // The client creates a connection message to send to the server
    // [ Client Public Key ]
    // [ Shared Key ]
    // *[ Challenge ]
    CreateConnectMessage(connectPacket, client);
    // The server can verify authenticity of protected via hmac
    // - Load Client Public Key
    // - Load Shared Key
    ParseConnectMessage(connectPacket, server);
    // The server can then acknowledge the client with some sort of status
    // [ Unique Server Public Key ]
    // *[ Challenge ]
    CreateConnectResponseMessage(ackPacket, server);
    // The client can verify authenticity of server by verifying their challenge and hmac
    ParseConnectResponseMessage(ackPacket, client);

    // The client is now able to communicate with the server until they timeout or are evicted.
    TEST(client.mClientKey.GetPublicKey() == server.mClientKey.GetPublicKey());
    TEST(client.mServerKey.GetPublicKey() == server.mServerKey.GetPublicKey());
    TestClientServerCommunication(client, server);

    PacketData::SetZero(connectPacket);
    PacketData::SetZero(ackPacket);

    client = TestClientState();
    server = TestServerState();
    InitStates(client, server);
    Crypto::SecureRandomBytes(client.mChallenge, sizeof(client.mChallenge));
    Crypto::SecureRandomBytes(client.mHMACKey, sizeof(client.mHMACKey));

    connectPacket.mSize = sizeof(connectPacket.mBytes);
    SizeT size = connectPacket.mSize;
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
    r = ConnectPacket::EncodeAckPacket
        (
            ackPacket.mBytes,
            size,
            server.mClientKey,
            server.mUniqueKey,
            server.mSharedKey,
            server.mHMACKey,
            server.mChallenge,
            47
        );
    TEST(r);

    ConnectionID connectionID = 0;
    ConnectPacket::AckHeaderType ackHeader;
    ByteT challenge[ConnectPacket::CHALLENGE_SIZE];
    r = ConnectPacket::DecodeAckPacket
        (
            ackPacket.mBytes,
            size,
            client.mClientKey,
            client.mUniqueKey,
            client.mSharedKey,
            client.mHMACKey,
            challenge,
            connectionID,
            ackHeader
        );
    TEST(r);
    TEST(connectionID == 47);
}

struct TestClientTransport
{
    NetTransport mTransport;
    TaskScheduler mTaskScheduler;
    NetClientController mClientController;
};

struct TestServerTransport
{
    NetTransport mTransport;
    TaskScheduler mTaskScheduler;
    NetServerController mServerController;
    NetConnectionController mConnectionController;

};

void InitClientTransport(TestClientTransport& client, Crypto::RSAKey serverKey, const IPEndPointAny& serverEndPoint)
{
    TaskTypes::TaskSchedulerOptions options;
    options.mDispatcherSize = 20;
    options.mNumDeliveryThreads = 2;
    options.mNumWorkerThreads = 2;
    client.mTaskScheduler.Initialize(options, true);
    TEST(client.mTaskScheduler.IsRunning());
    
    TEST(client.mClientController.Initialize(std::move(serverKey)));

    NetTransportConfig config;
    config.SetAppId(NetConfig::NET_APP_ID);
    config.SetAppVersion(NetConfig::NET_APP_VERSION);
    config.SetPort(TEST_PORT);
    config.SetEndPoint(serverEndPoint);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT, 
        LFNew<ClientConnectionHandler>(
            MMT_GENERAL, 
            &client.mTaskScheduler, 
            &client.mClientController)
    );

    
    ServerConnectionHandler::ConnectPacketData packet;
    SizeT size = sizeof(packet.mBytes);
    const bool encoded = ConnectPacket::EncodePacket
    (
        packet.mBytes,
        size,
        client.mClientController.GetKey(),
        client.mClientController.GetServerKey(),
        client.mClientController.GetSharedKey(),
        client.mClientController.GetHMACKey(),
        client.mClientController.GetChallenge()
    );
    TEST(encoded);
    client.mTransport.Start(std::move(config), packet.mBytes, size);
}

void InitServerTransport(TestServerTransport& server, Crypto::RSAKey serverKey)
{
    TaskTypes::TaskSchedulerOptions options;
    options.mDispatcherSize = 20;
    options.mNumDeliveryThreads = 2;
    options.mNumWorkerThreads = 2;
    server.mTaskScheduler.Initialize(options, true);
    TEST(server.mTaskScheduler.IsRunning());

    TEST(server.mServerController.Initialize(std::move(serverKey)));

    NetTransportConfig config;
    config.SetAppId(NetConfig::NET_APP_ID);
    config.SetAppVersion(NetConfig::NET_APP_VERSION);
    config.SetPort(TEST_PORT);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT, 
        LFNew<ServerConnectionHandler>(
            MMT_GENERAL, 
            &server.mTaskScheduler, 
            &server.mConnectionController, 
            &server.mServerController)
    );

    server.mTransport.Start(std::move(config));
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

REGISTER_TEST(NetConnectionTest)
{
    NetTestInitializer init;

    Crypto::RSAKey serverKey;
    serverKey.GeneratePair(Crypto::RSA_KEY_2048);

    TestServerTransport server;
    InitServerTransport(server, serverKey);
    SleepCallingThread(16);

    // todo: In the transport.. if the message comes in as 'IPV4' or 'IPV6' we can translate back to the specified type
    IPEndPointAny localIP;
    TEST(IPCast(IPV6("::1", TEST_PORT), localIP));
    TEST(!IPEmpty(localIP));
    TestClientTransport client;
    InitClientTransport(client, serverKey, localIP);

    // client.SendAnonymous(localIP, bytes, size);
    // client.IsConnected()
    // client.IsClient()
    // client.IsServer()

    const auto x = alignof(IPEndPointAny);

    SleepCallingThread(5000);

    client.mTransport.Stop();
    server.mTransport.Stop();
    client.mTaskScheduler.Shutdown();
    server.mTaskScheduler.Shutdown();

    client.mClientController.Reset();
    server.mServerController.Reset();

    
    // 
    // Crypto::RSAKey serverKey;
    // Crypto::RSAKey clientKey;
    // Crypto::AESKey dataKey;
    // NetConnectionController connectionController;
    // 
    // serverKey.GeneratePair(Crypto::RSA_KEY_2048);
    // clientKey.GeneratePair(Crypto::RSA_KEY_2048);
    // dataKey.Generate(Crypto::AES_KEY_256);
    // 
    // NetTransportConfig config;
    // config.SetAppId(NetConfig::NET_APP_ID);
    // config.SetAppVersion(NetConfig::NET_APP_VERSION);
    // config.SetPort(TEST_PORT);
    // config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT, LFNew<NetConnectionHandler>(MMT_GENERAL, &connectionController, &serverKey));
    // config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_DISCONNECT, LFNew<NetDisconnectionHandler>());
    // config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_HEARTBEAT, LFNew<NetHeartbeatHandler>());
    // 
    // NetServer server;
    // server.Start(std::move(config), serverKey);
    // 
    // NetClient client;
    // client.Start(std::move(config), serverKey, clientKey, dataKey);
    // config.SetAppId(NetConfig::NET_APP_ID);
    // config.SetAppVersion(NetConfig::NET_APP_VERSION);
    // config.SetPort(TEST_PORT);
    // config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT, LFNew<NetConnectionHandler>(MMT_GENERAL, &connectionController, &serverKey));
    // config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_DISCONNECT, LFNew<NetDisconnectionHandler>());
    // config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_HEARTBEAT, LFNew<NetHeartbeatHandler>());
    // 
    // // NetClient client;
    // client.Stop();
    // // 
    // server.Stop();

#if 0
    Crypto::AESKey key;
    key.Generate(Crypto::AES_KEY_256);

    String messageA = "Hello Message220Hello Message220";
    String messageB = "Hello Message220Hello Message220"; // "RandomSalt493900Hello Message220";
    String a;
    String b;
    String x;
    String y;

    ByteT iv[16];
    Crypto::SecureRandomBytes(iv, LF_ARRAY_SIZE(iv));

    TEST(Crypto::AESEncrypt(&key, iv, messageA, a));
    String aHex = BytesToHex(reinterpret_cast<const ByteT*>(a.CStr()), a.Size());
    TEST(Crypto::AESEncrypt(&key, iv, messageB, b));
    String bHex = BytesToHex(reinterpret_cast<const ByteT*>(b.CStr()), b.Size());
    TEST(Crypto::AESDecrypt(&key, iv, a, x));
    TEST(Crypto::AESDecrypt(&key, iv, b, y));
    TEST(x == messageA);
    TEST(y == messageB);
#endif
    

}

} // namespace lf
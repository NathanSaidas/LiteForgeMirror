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

#include "NetSocket.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/IO/EngineConfig.h"
#include "Core/IO/BinaryStream.h"
#include "Core/String/StringCommon.h"
#include "Core/String/SStream.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Platform/File.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Platform/Thread.h"
#include "Core/Test/Test.h"
#include "Core/Utility/ByteOrder.h"
#include "Core/Utility/Crc32.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Utility/StackTrace.h"

#include "Core/Net/ConnectPacket.h"
#include "Core/Net/NetFramework.h"
#include "Core/Net/NetTransport.h"
#include "Core/Net/NetTransportHandler.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/PacketUtility.h"
#include "Core/Net/UDPSocket.h"
#include "Core/Net/NetConnection.h"

#include "Core/Math/Random.h"
#include "Core/Math/SSECommon.h"

namespace lf {

// UInt16 UDPSocket::GetPort() const
// {
//     // calling this function before we bind or call sendto (clients only) will fails.
//     sockaddr_storage myAddress;
//     socklen_t myAddressLen = sizeof(myAddress);
//     if (getsockname(mSocket, reinterpret_cast<sockaddr*>(&myAddress), &myAddressLen) == -1)
//     {
//         LogWSAError(gSysLog, "getsockname");
//         return 0;
//     }
//     UInt16 port = 0;
//     if (myAddress.ss_family == AF_INET)
//     {
//         port = reinterpret_cast<sockaddr_in*>(&myAddress)->sin_port;
//     }
//     else if (myAddress.ss_family == AF_INET6)
//     {
//         port = reinterpret_cast<sockaddr_in6*>(&myAddress)->sin6_port;
//     }
//     return port;
// }

const UInt16 TEST_PORT = 27015;
const char* TEST_IPV4_TARGET = "127.0.0.1";
const char* TEST_IPV6_TARGET = "::1";
const NetProtocol::Value TEST_PROTOCOL = NetProtocol::NET_PROTOCOL_IPV6_UDP;
UInt16 gSenderPort = 0;

#define MAKE_TEST_IPV4(EndPoint_) IPV4(EndPoint_, TEST_IPV4_TARGET, TEST_PORT)
#define MAKE_TEST_IPV6(EndPoint_) IPV6(EndPoint_, TEST_IPV6_TARGET, TEST_PORT)
#define MAKE_TEST_IP(EndPoint_) MAKE_TEST_IPV6(EndPoint_)


void RecvThread(void* ptr)
{
    UDPSocket* socket = reinterpret_cast<UDPSocket*>(ptr);

    ByteT bytes[4096];
    SizeT inOutBytes = sizeof(bytes);
    IPEndPointAny sender;

    socket->Bind(TEST_PORT);
    if (!socket->ReceiveFrom(bytes, inOutBytes, sender))
    {
        gTestLog.Error(LogMessage("Server failed to receive bytes."));
    }
    else
    {
        gTestLog.Info(LogMessage("Server received ") << inOutBytes << " bytes...");
    }

    // UDPSocket connection;
    // if (sender.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4)
    // {
    //     connection.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP);
    //     connection.BindOutbound(sender);
    // }
    // else if (sender.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV6)
    // {
    //     connection.Create(NetProtocol::NET_PROTOCOL_IPV6_UDP);
    //     connection.BindOutbound(sender);
    // }
    // else
    // {
    // 
    // }
    SleepCallingThread(200);

    gSenderPort = SwapBytes(sender.mPort);

    IPEndPointAny localSender;
    IPV6(localSender, "::1", SwapBytes(sender.mPort));
    

    gTestLog.Info(LogMessage("Local Sender =") << IPToString(localSender));
    gTestLog.Info(LogMessage("Actual Sender=") << IPToString(sender));
    gTestLog.Sync();


    UDPSocket sn;
    sn.Create(TEST_PROTOCOL);
    if (!sn.SendTo(bytes, inOutBytes, sender))
    {
        gTestLog.Error(LogMessage("Server failed to respond!"));
    }

    socket->ReceiveFrom(bytes, inOutBytes, sender);
    gTestLog.Info(LogMessage("Server Done!"));
}

void SendThread(void* ptr)
{
    gTestLog.Info(LogMessage("Client awaiting 15 seconds...\n"));

    SleepCallingThread(1000);
    UDPSocket* socket = reinterpret_cast<UDPSocket*>(ptr);

    ByteT byteMessage[2000];
    Crypto::SecureRandomBytes(byteMessage, sizeof(byteMessage));


    String message; //  = "Hello Socket!";
    message.Resize(sizeof(byteMessage));
    memcpy(const_cast<char*>(message.CStr()), byteMessage, sizeof(byteMessage));

    SizeT inOutBytes = message.Size();

    IPEndPointAny endPoint;
    TEST_CRITICAL(MAKE_TEST_IP(endPoint));

    bool result = socket->SendTo(reinterpret_cast<const ByteT*>(message.CStr()), inOutBytes, endPoint);
    if (!result || inOutBytes != message.Size())
    {
        gTestLog.Error(LogMessage("Client failed to send bytes."));
    }
    else
    {
        gTestLog.Info(LogMessage("Client sent ") << inOutBytes << "/" << message.Size() << " bytes.");
    }

    socket->ReceiveFrom(byteMessage, inOutBytes, endPoint);
    gTestLog.Info(LogMessage("Client Wait!"));

    socket->ReceiveFrom(byteMessage, inOutBytes, endPoint);
    gTestLog.Info(LogMessage("Client Done!"));
}

REGISTER_TEST(BasicNetSocketTest)
{
    TEST_CRITICAL(NetInitialize());
    TEST(IsNetInitialized());

    {
        UDPSocket socket;
        socket.Create(TEST_PROTOCOL);
    }

    {
        UDPSocket receiver;
        // UDPSocket receiverB;
        UDPSocket sender;
        
        TEST(receiver.Create(NetProtocol::NET_PROTOCOL_UDP));
        // TEST(receiverB.Create(NetProtocol::NET_PROTOCOL_UDP));
        TEST(sender.Create(TEST_PROTOCOL));
        
        // Thread recvThreadB; recvThreadB.Fork(RecvThread, &receiverB);
        Thread recvThread; recvThread.Fork(RecvThread, &receiver);
        Thread sendThread; sendThread.Fork(SendThread, &sender);

        SleepCallingThread(2000);
        

        
        // recvThreadB.Join();
        SleepCallingThread(500);
        // sendThread.Join();
        SleepCallingThread(2500);

        TEST(!receiver.IsAwaitingReceive() || receiver.Shutdown());
        recvThread.Join();

        TEST(!sender.IsAwaitingReceive() || sender.Shutdown());
        sendThread.Join();

        // if (receiver.IsAwaitingReceive())
        // {
        //     String flushCmd = "flush";
        //     IPEndPointAny flushEndPoint;
        //     IPV4(flushEndPoint, "127.0.0.1", receiver.GetBoundPort());
        // 
        //     UDPSocket flush;
        //     flush.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP);
        //     while (receiver.IsAwaitingReceive())
        //     {
        //         SizeT inOutBytes = sizeof(flushCmd.Size());
        //         flush.SendTo(reinterpret_cast<const ByteT*>(flushCmd.CStr()), inOutBytes, flushEndPoint);
        //         SleepCallingThread(1);
        //     }
        //     receiver.Close();
        // }
        // recvThread.Join();
        // 
        // if (sender.IsAwaitingReceive())
        // {
        //     String flushCmd = "flush";
        //     IPEndPointAny flushEndPoint;
        //     if (sender.GetProtocol() == NetAddressFamily::NET_ADDRESS_FAMILY_IPV6)
        //     {
        //         IPV6(flushEndPoint, "::1", gSenderPort);
        //     }
        //     else
        //     {
        //         IPV4(flushEndPoint, "127.0.0.1", gSenderPort);
        //     }
        // 
        //     UDPSocket flush;
        //     flush.Create(sender.GetProtocol());
        //     while (sender.IsAwaitingReceive())
        //     {
        //         SizeT inOutBytes = sizeof(flushCmd.Size());
        //         flush.SendTo(reinterpret_cast<const ByteT*>(flushCmd.CStr()), inOutBytes, flushEndPoint);
        //         SleepCallingThread(1);
        //     }
        //     sender.Close();
        // }
        // sendThread.Join();
    }

    TEST(NetShutdown());
    TEST(IsNetInitialized() == false);
}

// Now we will begin packet descriptions:

REGISTER_TEST(ByteOrderSwapTest)
{
    TEST_CRITICAL(IsLittleEndian()); // Test assumes little endian mode.
    TEST(SwapBytes(UInt64(0xAABBCCDD11223344)) == UInt64(0x44332211DDCCBBAA));
    TEST(SwapBytes(Int64(0xAABBCCDD11223344)) == Int64(0x44332211DDCCBBAA));
    TEST(SwapBytes(UInt32(0xAABBCCDD)) == UInt32(0xDDCCBBAA));
    TEST(SwapBytes(Int32(0xAABBCCDD)) == Int32(0xDDCCBBAA));
    TEST(SwapBytes(UInt16(0xAABB)) == UInt16(0xBBAA));
    TEST(SwapBytes(Int16(0x1122)) == Int16(0x2211));
}

REGISTER_TEST(IPEndPointTest)
{
    {
        IPv4EndPoint ipv4;
        TEST(InvalidEnum(static_cast<NetAddressFamily::Value>(ipv4.mAddressFamily)));
        TEST(ipv4.mPort == 0);
        TEST(ipv4.mAddress.mWord == 0);

        TEST_CRITICAL(IPV4(reinterpret_cast<IPEndPointAny&>(ipv4), "127.0.0.1", 27015));
        TEST(ValidEnum(static_cast<NetAddressFamily::Value>(ipv4.mAddressFamily)));
        TEST(ipv4.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4);
        TEST(ipv4.mPort == SwapBytes(UInt16(27015)));
        TEST(ipv4.mAddress.mBytes[0] == 127);
        TEST(ipv4.mAddress.mBytes[1] == 0);
        TEST(ipv4.mAddress.mBytes[2] == 0);
        TEST(ipv4.mAddress.mBytes[3] == 1);

        {
            IPv4EndPoint copied(ipv4);
            TEST(copied == ipv4);
            TEST(copied != IPv4EndPoint());
        }

        {
            IPv4EndPoint copied;
            TEST(copied != ipv4);
            copied = ipv4;
            TEST(copied == ipv4);
        }

        {
            IPv4EndPoint copied(ipv4);
            IPv4EndPoint moved(std::move(copied));
            TEST(copied == IPv4EndPoint());
            TEST(moved == ipv4);
        }

        {
            IPv4EndPoint copied(ipv4);
            IPv4EndPoint moved;
            moved = std::move(copied);
            TEST(copied == IPv4EndPoint());
            TEST(moved == ipv4);
        }
    }

    {
        IPv6EndPoint ipv6;
        TEST(InvalidEnum(static_cast<NetAddressFamily::Value>(ipv6.mAddressFamily)));
        TEST(ipv6.mPort == 0);
        for (SizeT i = 0; i < LF_ARRAY_SIZE(ipv6.mAddress.mWord); ++i)
        {
            TEST(ipv6.mAddress.mWord[i] == 0);
        }

        TEST_CRITICAL(IPV6(reinterpret_cast<IPEndPointAny&>(ipv6), "::1", 27015));
        TEST(ValidEnum(static_cast<NetAddressFamily::Value>(ipv6.mAddressFamily)));
        TEST(ipv6.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV6);
        TEST(ipv6.mPort == SwapBytes(UInt16(27015)));
        TEST(ipv6.mAddress.mWord[0] == 0);
        TEST(ipv6.mAddress.mWord[1] == 0);
        TEST(ipv6.mAddress.mWord[2] == 0);
        TEST(ipv6.mAddress.mWord[3] == 0);
        TEST(ipv6.mAddress.mWord[4] == 0);
        TEST(ipv6.mAddress.mWord[5] == 0);
        TEST(ipv6.mAddress.mWord[6] == 0);
        TEST(ipv6.mAddress.mWord[7] == SwapBytes(UInt16(1)));

        {
            IPv6EndPoint copied(ipv6);
            TEST(copied == ipv6);
            TEST(copied != IPv6EndPoint());
        }

        {
            IPv6EndPoint copied;
            TEST(copied != ipv6);
            copied = ipv6;
            TEST(copied == ipv6);
        }

        {
            IPv6EndPoint copied(ipv6);
            IPv6EndPoint moved(std::move(copied));
            TEST(copied == IPv6EndPoint());
            TEST(moved == ipv6);
        }

        {
            IPv6EndPoint copied(ipv6);
            IPv6EndPoint moved;
            moved = std::move(copied);
            TEST(copied == IPv6EndPoint());
            TEST(moved == ipv6);
        }
    }

}


struct NetDriverHostArgs
{
    UInt16 mPort;
};

struct NetDriverClientArgs
{
    UInt16 mPort;
    const char* mAddress;
};

// class NetDriver
// {
// public:
//     void Start(NetDriverHostArgs args);
//     void Start(NetDriverClientArgs args);
// 
//     void Stop();
// 
//     bool IsRunning() const;
// private:
//     NetTransport mTransport;
// 
// };



static void UDPSocketSendReceiveSameSocketRecvThread(void* arg)
{
    ByteT bytes[2048];
    SizeT numBytes = sizeof(bytes);
    IPEndPointAny sender;

    UDPSocket* socket = reinterpret_cast<UDPSocket*>(arg);
    socket->ReceiveFrom(bytes, numBytes, sender);
}

static void UDPSocketSendReceiveSameSocketSendThread(void* arg)
{
    ByteT dummyData[16];
    SizeT dummySize = sizeof(dummyData);
    IPEndPointAny target;
    TEST(IPV4(target, TEST_IPV4_TARGET, TEST_PORT));

    UDPSocket* socket = reinterpret_cast<UDPSocket*>(arg);
    SleepCallingThread(1500);
    socket->SendTo(dummyData, dummySize, target);
}

REGISTER_TEST(UDPSocketSendReceiveSameSocketTest)
{
    // TEST_CRITICAL(IsNetInitialized() || NetInitialize());
    if (!IsNetInitialized())
    {
        return;
    }

    ByteT dummyData[16];
    SizeT dummySize = sizeof(dummyData);
    IPEndPointAny target;
    TEST(IPV4(target, TEST_IPV4_TARGET, TEST_PORT));

    UDPSocket client;
    TEST(client.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP));
    UDPSocket sender;
    TEST(sender.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP));
    TEST(sender.SendTo(dummyData, dummySize, target));

    // t1: Send data to server...
    TEST(client.SendTo(dummyData, dummySize, target));

    TEST(sender.GetBoundPort() > 0);
    TEST(client.GetBoundPort() > 0);

    // t1: Recv data from server...
    Thread recv;
    recv.Fork(UDPSocketSendReceiveSameSocketRecvThread, &client);
    // t2: Send data to server...
    Thread send;
    send.Fork(UDPSocketSendReceiveSameSocketSendThread, &client);

    SleepCallingThread(5000);



    // flush receiver...
    UDPSocket flusher;
    flusher.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP);
    IPV4(target, "127.0.0.1", client.GetBoundPort());
    while (client.IsAwaitingReceive())
    {
        flusher.SendTo(dummyData, dummySize, target);
        SleepCallingThread(1000);
    }
    recv.Join();
    send.Join();
    TEST_CRITICAL(NetShutdown());
}

static String GetTestPath()
{
    return FileSystem::PathJoin(FileSystem::PathJoin(TestFramework::GetConfig().mEngineConfig->GetTempDirectory(), "TestInput"), "NetTest");
}

static String ReadRSAKey(const String& filename) 
{
    String testFilePath = FileSystem::PathJoin(GetTestPath(), filename);
    String text;
    File file;
    if (file.Open(testFilePath, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        text.Resize(static_cast<SizeT>(file.GetSize()));
        TEST(file.Read(const_cast<char*>(text.CStr()), text.Size()) == text.Size());
    }
    return text;
}

static bool ReadAESKey(const String& filename, Crypto::AESKey& cryptoKey)
{
    String testFilePath = FileSystem::PathJoin(GetTestPath(), filename);
    File file;
    if (!file.Open(testFilePath, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        return false;
    }

    UInt32 keySizeBytes = 0;
    ByteT key[32];

    if (file.Read(&keySizeBytes, sizeof(UInt32)) != sizeof(UInt32))
    {
        return false;
    }
    if (file.Read(key, sizeof(key)) != sizeof(key))
    {
        return false;
    }
    Crypto::AESKeySize aesKeySize = Crypto::AES_KEY_Unknown;
    if (keySizeBytes == 16)
    {
        aesKeySize = Crypto::AES_KEY_128;
    }
    else if(keySizeBytes == 32)
    {
        aesKeySize = Crypto::AES_KEY_256;
    }
    return cryptoKey.Load(aesKeySize, key);
}

struct ConnectSignature256
{
    ByteT mIV[16];
    ByteT mKey[32];
    ByteT mSalt[32];
    ByteT mHash[32];
};

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

REGISTER_TEST(NetTransportSecureCommunicationTest)
{
    NetTestInitializer init;

    // Server:
    Crypto::RSAKey serverPrivateKey;
    Crypto::RSAKey serverConnectionKey;
    Crypto::AESKey serverConnectionMessageKey;
    ByteT serverHMACKey[Crypto::HMAC_KEY_SIZE];
    ByteT serverChallenge[ConnectPacket::CHALLENGE_SIZE];

    // Client:
    Crypto::RSAKey clientServerKey;
    Crypto::RSAKey clientKey;
    Crypto::AESKey clientMessageKey;
    ByteT clientHMACKey[Crypto::HMAC_KEY_SIZE];
    ByteT clientChallenge[ConnectPacket::CHALLENGE_SIZE];

    TEST_CRITICAL(serverPrivateKey.LoadPrivateKey(ReadRSAKey("rsa_2048_private.key")));
    TEST_CRITICAL(clientServerKey.LoadPublicKey(ReadRSAKey("rsa_2048_public.key")));
    TEST(!clientServerKey.HasPrivateKey());

    TEST_CRITICAL(clientKey.LoadPrivateKey(ReadRSAKey("rsa_2048_client_private.key")));
    TEST_CRITICAL(ReadAESKey("aes_256.key", clientMessageKey) && clientMessageKey.GetKey());

    TEST_CRITICAL(serverPrivateKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST_CRITICAL(clientServerKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST_CRITICAL(clientMessageKey.GetKeySize() == Crypto::AES_KEY_256);
    TEST(serverPrivateKey.GetPublicKey() == clientServerKey.GetPublicKey());
    TEST(clientKey.GetPublicKey() != clientServerKey.GetPublicKey());
    
    // Simulate Key Exchange:
    ByteT packetBytes[1024];
    SizeT packetBytesLength = sizeof(packetBytes);
    TEST_CRITICAL(ConnectPacket::EncodePacket(packetBytes, packetBytesLength, clientKey, clientServerKey, clientMessageKey, clientHMACKey, clientChallenge));

    // memsearch:
    {
        // todo: ConnectPacket::GetSecureData(out IV, out HMAC);

        String packetMem(packetBytesLength, reinterpret_cast<const char*>(packetBytes), COPY_ON_WRITE);
        String searchKey(clientMessageKey.GetKeySizeBytes(), reinterpret_cast<const char*>(clientMessageKey.GetKey()), COPY_ON_WRITE);
        String searchHmacKey(Crypto::HMAC_KEY_SIZE, reinterpret_cast<const char*>(clientHMACKey), COPY_ON_WRITE);
        String searchChallenge(ConnectPacket::CHALLENGE_SIZE, reinterpret_cast<const char*>(clientChallenge), COPY_ON_WRITE);
        // The message key SHOULD be encrypted and we shouldn't be able to find sensitive data very easily in the packet.
        TEST(Invalid(packetMem.Find(searchKey)));
        TEST(Invalid(packetMem.Find(clientKey.GetPublicKey())));
        TEST(Invalid(packetMem.Find(searchHmacKey)));
        TEST(Invalid(packetMem.Find(searchChallenge)));
    }

    // Verify we're sending the right header.
    const PacketHeader* header = reinterpret_cast<const PacketHeader*>(packetBytes);
    TEST(header->mAppID == NetConfig::NET_APP_ID);
    TEST(header->mAppVersion == NetConfig::NET_APP_VERSION);
    TEST(header->mCrc32 == PacketUtility::CalcCrc32(packetBytes, packetBytesLength));
    TEST(header->mType == NetPacketType::NET_PACKET_TYPE_CONNECT);
    NetPacketFlag::BitfieldType flags(header->mFlags);
    TEST(flags.Is(0));

    // Verify Server Receive

    ConnectPacket::HeaderType outHeader;
    TEST_CRITICAL(ConnectPacket::DecodePacket(packetBytes, packetBytesLength, serverPrivateKey, serverConnectionKey, serverConnectionMessageKey, serverHMACKey, serverChallenge, outHeader));
    TEST(outHeader.mAppID == header->mAppID);
    TEST(outHeader.mAppVersion == header->mAppVersion);
    TEST(outHeader.mCrc32 == header->mCrc32);
    TEST(outHeader.mFlags == header->mFlags);
    TEST(outHeader.mType == header->mType);

    // Verify Keys..
    TEST(serverConnectionKey.GetPublicKey() == clientKey.GetPublicKey());
    TEST(serverConnectionMessageKey.GetKeySize() == clientMessageKey.GetKeySize());
    TEST(memcmp(serverConnectionMessageKey.GetKey(), clientMessageKey.GetKey(), clientMessageKey.GetKeySizeBytes()) == 0);
    TEST(memcmp(serverHMACKey, clientHMACKey, sizeof(clientHMACKey)) == 0);
    TEST(memcmp(serverChallenge, clientChallenge, sizeof(clientChallenge)) == 0);

    // todo: Encode/Decode AckPacket
}

REGISTER_TEST(RSASignatureReplayAttack)
{
    // Server:
    Crypto::RSAKey serverPrivateKey;
    Crypto::RSAKey serverConnectionKey;
    Crypto::AESKey serverConnectionMessageKey;

    // Client:
    Crypto::RSAKey clientServerKey;
    Crypto::RSAKey clientKey;
    Crypto::AESKey clientMessageKey;

    TEST_CRITICAL(serverPrivateKey.LoadPrivateKey(ReadRSAKey("rsa_2048_private.key")));
    TEST_CRITICAL(clientServerKey.LoadPublicKey(ReadRSAKey("rsa_2048_public.key")));
    TEST(!clientServerKey.HasPrivateKey());

    TEST_CRITICAL(clientKey.LoadPrivateKey(ReadRSAKey("rsa_2048_client_private.key")));
    TEST_CRITICAL(ReadAESKey("aes_256.key", clientMessageKey) && clientMessageKey.GetKey());

    TEST_CRITICAL(serverPrivateKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST_CRITICAL(clientServerKey.GetKeySize() == Crypto::RSA_KEY_2048);
    TEST_CRITICAL(clientMessageKey.GetKeySize() == Crypto::AES_KEY_256);
    TEST(serverPrivateKey.GetPublicKey() == clientServerKey.GetPublicKey());
    TEST(clientKey.GetPublicKey() != clientServerKey.GetPublicKey());
    TEST_CRITICAL(serverConnectionMessageKey.Load(clientMessageKey.GetKeySize(), clientMessageKey.GetKey()));
    TEST_CRITICAL(serverConnectionKey.LoadPublicKey(clientKey.GetPublicKey()));

    struct Message { ByteT mData[1024]; };
    TStaticArray<String, 512> signatures;
    TStaticArray<Message, 512> messages;
    messages.Resize(512);
    const SizeT numMessages = 256;
    const SizeT numReplay = 2;

    // Loop once
    for (SizeT r = 0; r < numReplay; ++r)
    {
        // This isn't a perfect example of protection against replay attack as we would actually need
        // to use a nonce or some sort of expiration/checking to make sure we've received X message
        // This does however demonstrate how signing/verifying our data means an attacker cannot use
        // the same signature twice for the same data. All signatures are to be 'unique'
        SizeT msgIndex = 0;
        bool isReplay = r != 0;
        Int32 messageSeed = 0xCEECEE70;
        for (SizeT k = 0; k < numMessages; ++k)
        {
            ByteT messageBytes[1024];
            SizeT messageLength = Random::Range(messageSeed, 48, 1008);
            for (SizeT i = 0; i < messageLength; ++i)
            {
                messageBytes[i] = static_cast<ByteT>(Random::Mod(messageSeed, 0xFF));
            }

            // [ Encrypt ][ Sign ]

            // Client Send:
            ByteT iv[16];
            Crypto::SecureRandomBytes(iv, 16);
            ByteT encrypted[1024];
            SizeT encryptedLength = sizeof(encrypted);
            TEST_CRITICAL(Crypto::AESEncrypt(&clientMessageKey, iv, messageBytes, messageLength, encrypted, encryptedLength));
            String signature;
            TEST_CRITICAL(Crypto::RSASignPublic(&clientServerKey, encrypted, encryptedLength, signature));
            TEST_CRITICAL(msgIndex < messages.Size());
            if (isReplay)
            {
                // Same message different signature
                TEST(std::find(signatures.begin(), signatures.end(), signature) == signatures.end());
                TEST(memcmp(messages[msgIndex++].mData, messageBytes, messageLength) == 0);
            }
            else
            {
                memcpy(messages[msgIndex++].mData, messageBytes, messageLength);
                signatures.Add(signature);
            }

            // Server Receive:
            TEST_CRITICAL(Crypto::RSAVerifyPrivate(&serverPrivateKey, encrypted, encryptedLength, signature));

            ByteT decrypted[1024];
            SizeT decryptedLength = sizeof(decrypted);
            TEST_CRITICAL(Crypto::AESDecrypt(&serverConnectionMessageKey, iv, encrypted, encryptedLength, decrypted, decryptedLength));
            TEST(decryptedLength == messageLength);
            TEST(memcmp(decrypted, messageBytes, decryptedLength) == 0);

            // Server Ack:
            messageLength = Random::Range(messageSeed, 48, 180);
            for (SizeT i = 0; i < messageLength; ++i)
            {
                messageBytes[i] = static_cast<ByteT>(Random::Mod(messageSeed, 0xFF));
            }
            encryptedLength = sizeof(encrypted);
            TEST_CRITICAL(Crypto::AESEncrypt(&serverConnectionMessageKey, iv, messageBytes, messageLength, encrypted, encryptedLength));
            TEST_CRITICAL(Crypto::RSASignPublic(&serverConnectionKey, encrypted, encryptedLength, signature));
            TEST_CRITICAL(msgIndex < messages.Size());
            if (isReplay)
            {
                TEST(std::find(signatures.begin(), signatures.end(), signature) == signatures.end());
                TEST(memcmp(messages[msgIndex++].mData, messageBytes, messageLength) == 0);
            }
            else
            {
                memcpy(messages[msgIndex++].mData, messageBytes, messageLength);
                signatures.Add(signature);
            }

            // Client Receive:
            TEST_CRITICAL(Crypto::RSAVerifyPrivate(&clientKey, encrypted, encryptedLength, signature));

            decryptedLength = sizeof(decrypted);
            TEST_CRITICAL(Crypto::AESDecrypt(&clientMessageKey, iv, encrypted, encryptedLength, decrypted, decryptedLength));
            TEST(decryptedLength == messageLength);
            TEST(memcmp(decrypted, messageBytes, decryptedLength) == 0);
        }
    }
}

String GetShortHexString(UInt16 value)
{
    String str = ToHexString(value);
    while (str.Size() != 4)
    {
        str.Insert("0", 0);
    }
    return str;
}

String GetEndPointString(const IPEndPointAny& endPoint)
{
    if (endPoint.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4)
    {
        const IPv4EndPoint& ipv4 = reinterpret_cast<const IPv4EndPoint&>(endPoint);
        SStream ss;
        ss << ipv4.mAddress.mBytes[0] << "." << ipv4.mAddress.mBytes[1] << "." << ipv4.mAddress.mBytes[2] << "." << ipv4.mAddress.mBytes[3] << ":" << ipv4.mPort;
        return ss.Str();
    }
    else if (endPoint.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV6)
    {
        const IPv6EndPoint& ipv6 = reinterpret_cast<const IPv6EndPoint&>(endPoint);
        SStream ss;
        for (SizeT i = 0; i < LF_ARRAY_SIZE(ipv6.mAddress.mWord); ++i)
        {
            if (i != 0)
            {
                ss << "::";
            }
            ss << GetShortHexString(ipv6.mAddress.mWord[i]);
        }

        ss << ":" << ipv6.mPort;
        return ss.Str();
    }
    return String();
}

class MessageTransportHandler : public NetTransportHandler
{
protected:
    void OnInitialize() override
    {
        gTestLog.Info(LogMessage("MessageTransportHandler::OnInitialize"));
    }

    void OnShutdown() override
    {
        gTestLog.Info(LogMessage("MessageTransportHandler::OnShutdown"));
    }

    void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) override
    {
        (bytes);
        (byteLength);
        gTestLog.Info(LogMessage("MessageTransportHandler::OnReceivePacket. Sender=") << GetEndPointString(sender));
    }

    void OnUpdateFrame() override
    {
        gTestLog.Info(LogMessage("MessageTransportHandler::OnUpdateFrame"));

    }
};

REGISTER_TEST(PacketUtilityTest)
{
    {
        PacketHeader header;
        const ByteT* headerBytes = reinterpret_cast<const ByteT*>(&header);

        header.mAppID = NetConfig::NET_APP_ID;
        header.mAppVersion = NetConfig::NET_APP_VERSION;
        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_CONNECT;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_DISCONNECT;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_HEARTBEAT;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_MESSAGE;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED);

        // Ack
        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_CONNECT;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_DISCONNECT;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_HEARTBEAT;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_MESSAGE;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED);

        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE, NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        TEST(PacketUtility::IsAck(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::IsConnected(headerBytes, sizeof(PacketHeader)) == false);
        TEST(PacketUtility::IsSecureConnected(headerBytes, sizeof(PacketHeader)));
        TEST(PacketUtility::GetHeaderType(headerBytes, sizeof(PacketHeader)) == NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED);
    }

    Crypto::AESKey sharedKey;
    TEST_CRITICAL(ReadAESKey("aes_256.key", sharedKey));

    Crypto::RSAKey serverKey;
    TEST_CRITICAL(serverKey.LoadPrivateKey(ReadRSAKey("rsa_2048_private.key")));

    // Ack Base
    {
        PacketHeader header;
        const ByteT* headerBytes = reinterpret_cast<const ByteT*>(&header);

        header.mAppID = NetConfig::NET_APP_ID;
        header.mAppVersion = NetConfig::NET_APP_VERSION;
        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_CONNECT;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));

        SizeT corruptAckSize = PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE;
        SizeT okAckSize = PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE;
        ByteT corruptAck[PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE];
        ByteT okAck[PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE];

        TEST(PacketUtility::PrepareAckCorruptHeader(headerBytes, sizeof(header), corruptAck, corruptAckSize, serverKey));
        TEST(PacketUtility::IsAck(corruptAck, corruptAckSize));
        TEST(PacketUtility::GetAckSize(PacketUtility::GetHeaderType(corruptAck, corruptAckSize)) == AckPacketHeader::ACTUAL_SIZE);
        TEST(PacketUtility::CalcCrc32(corruptAck, corruptAckSize) == reinterpret_cast<PacketHeader*>(corruptAck)->mCrc32);

        TEST(PacketUtility::PrepareAckOkHeader(headerBytes, sizeof(header), okAck, okAckSize, serverKey));
        TEST(PacketUtility::IsAck(okAck, okAckSize));
        TEST(PacketUtility::GetAckSize(PacketUtility::GetHeaderType(okAck, okAckSize)) == AckPacketHeader::ACTUAL_SIZE);
        TEST(PacketUtility::CalcCrc32(okAck, okAckSize) == reinterpret_cast<PacketHeader*>(okAck)->mCrc32);

    }
    // Ack Connected
    {
        ConnectedPacketHeader header;
        const ByteT* headerBytes = reinterpret_cast<const ByteT*>(&header);

        header.mAppID = NetConfig::NET_APP_ID;
        header.mAppVersion = NetConfig::NET_APP_VERSION;
        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_MESSAGE;
        header.mConnectionID = 378;
        header.mPacketUID = 2993409;
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));

        SizeT corruptAckSize = PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE;
        SizeT okAckSize = PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE;
        ByteT corruptAck[PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE];
        ByteT okAck[PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE];

        TEST(PacketUtility::PrepareAckCorruptHeader(headerBytes, sizeof(header), corruptAck, corruptAckSize, serverKey));
        TEST(PacketUtility::IsAck(corruptAck, corruptAckSize));
        TEST(PacketUtility::GetAckSize(PacketUtility::GetHeaderType(corruptAck, corruptAckSize)) == AckConnectedPacketHeader::ACTUAL_SIZE);
        TEST(PacketUtility::CalcCrc32(corruptAck, corruptAckSize) == reinterpret_cast<PacketHeader*>(corruptAck)->mCrc32);

        TEST(PacketUtility::PrepareAckOkHeader(headerBytes, sizeof(header), okAck, okAckSize, serverKey));
        TEST(PacketUtility::IsAck(okAck, okAckSize));
        TEST(PacketUtility::GetAckSize(PacketUtility::GetHeaderType(okAck, okAckSize)) == AckConnectedPacketHeader::ACTUAL_SIZE);
        TEST(PacketUtility::CalcCrc32(okAck, okAckSize) == reinterpret_cast<PacketHeader*>(okAck)->mCrc32);

    }
    // Ack Secure Connected
    {
        SecureConnectedPacketHeader header;
        const ByteT* headerBytes = reinterpret_cast<const ByteT*>(&header);

        header.mAppID = NetConfig::NET_APP_ID;
        header.mAppVersion = NetConfig::NET_APP_VERSION;
        header.mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY, NetPacketFlag::NET_PACKET_FLAG_SECURE }).value;
        header.mType = NetPacketType::NET_PACKET_TYPE_MESSAGE;

        // make 'secure' packet
        SizeT encryptedBytes = 48;
        ByteT secureData[48];
        *reinterpret_cast<UInt16*>(&secureData[32]) = 378;
        *reinterpret_cast<UInt32*>(&secureData[34]) = 2993409;
        Crypto::SecureRandomBytes(&secureData[38], sizeof(header.mReservedPadding));
        Crypto::SHA256HashType hash = Crypto::SHA256Hash(reinterpret_cast<const ByteT*>(&header.mConnectionID), 16);
        memcpy(&secureData[0], hash.mData, sizeof(hash));
        ByteT iv[16];
        Crypto::SecureRandomBytes(iv, 16);
        TEST_CRITICAL(Crypto::AESEncrypt(&sharedKey, iv, &secureData[0], sizeof(secureData) - 1, &header.mHash[0], encryptedBytes));
        header.mCrc32 = PacketUtility::CalcCrc32(headerBytes, sizeof(header));
        // copy decrypted bytes back into header as PrepareAck assumes data is already decrypted.
        memcpy(&header.mHash[0], &secureData[0], sizeof(secureData));

        SizeT corruptAckSize = PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE;
        SizeT okAckSize = PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE;
        ByteT corruptAck[PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE];
        ByteT okAck[PacketUtility::MAX_PACKET_ACKNOWLEDGEMENT_SIZE];

        TEST(PacketUtility::PrepareAckCorruptHeader(headerBytes, sizeof(header), corruptAck, corruptAckSize, serverKey));
        TEST(PacketUtility::IsAck(corruptAck, corruptAckSize));
        TEST(PacketUtility::GetAckSize(PacketUtility::GetHeaderType(corruptAck, corruptAckSize)) == AckSecureConnectedPacketHeader::ACTUAL_SIZE);
        TEST(PacketUtility::CalcCrc32(corruptAck, corruptAckSize) == reinterpret_cast<PacketHeader*>(corruptAck)->mCrc32);

        TEST(PacketUtility::PrepareAckOkHeader(headerBytes, sizeof(header), okAck, okAckSize, serverKey));
        TEST(PacketUtility::IsAck(okAck, okAckSize));
        TEST(PacketUtility::GetAckSize(PacketUtility::GetHeaderType(okAck, okAckSize)) == AckSecureConnectedPacketHeader::ACTUAL_SIZE);
        TEST(PacketUtility::CalcCrc32(okAck, okAckSize) == reinterpret_cast<PacketHeader*>(okAck)->mCrc32);

    }
}

struct TestReplicationDatumItem
{
    TestReplicationDatumItem() : mItemName(), mItemDurability(0) {}
    TestReplicationDatumItem(const char* itemName, int itemDurability) : mItemName(itemName), mItemDurability(itemDurability) {}

    String mItemName;
    int mItemDurability;
};

Stream& operator<<(Stream& s, TestReplicationDatumItem& self)
{
    SERIALIZE(s, self.mItemName, "");
    SERIALIZE(s, self.mItemDurability, "");
    return s;
}

struct TestReplicationDatum
{
    int mHealth;
    int mMana;
    TArray<TestReplicationDatumItem> mItems;
};

Stream& operator<<(Stream& s, TestReplicationDatum& self)
{
    SERIALIZE(s, self.mHealth, "");
    SERIALIZE(s, self.mMana, "");
    SERIALIZE_STRUCT_ARRAY(s, self.mItems, "");
    return s;
}

REGISTER_TEST(NetReplicationTest)
{
    NetTestInitializer init;

    TestReplicationDatum data;
    data.mHealth = 4066;
    data.mMana = 6305;
    data.mItems.Add(TestReplicationDatumItem("Greatsword", 65));
    data.mItems.Add(TestReplicationDatumItem("Greataxe", 35));
    data.mItems.Add(TestReplicationDatumItem("Egg", 0));
    data.mItems.Add(TestReplicationDatumItem("Leather Hide", 0));
    data.mItems.Add(TestReplicationDatumItem("Goat Milk", 0));

    MemoryBuffer mb;
    BinaryStream bs;
    bs.Open(Stream::MEMORY, &mb, Stream::SM_WRITE);
    bs.BeginObject("_", "_");
    bs << data;
    bs.EndObject();
    bs.Close();

    // How do I write this data to a packet?

    // How many packets do we need to send?
    // Lets assume we can write MTU.. 
    // RUNTIME_MTU = 2048;
    // RUNTIME_DATA_RATE = 400 Kbs 
    // FRAME_RATE = 20
    // maxPacketPerFrame = RUNTIME_DATA_RATE / FRAME_RATE / RUNTIME_MTU = 10
    // 
    
    // Game Code -> Replicator/Net Packet Allocator -> Net Transport
    
    // [Repeat for Reliable/Unreliable]
    // 1. Calculate all objects to be replicated
    // 2. Write objects to memory buffer
    // 3. Calculate # of packets to be sent for transport
    // 4. Allocate # of packets to be sent to transport
    // 5. Copy memory buffer into packets
    // 6. Send packets to transport
    // -------------------------------
    // [Unreliable]
    // 1. Send all packet data to connection
    // 2. Free allocated packets
    // -------------------------------
    // [Reliable]
    // 1. Begin Bulk Replication (agree on ID/data channel)
    // 2. Send all packets [0....N]
    // 3. Retransmit packets not ack'd within timeout period. (avg ping time?)
    // 4. Complete Bulk Replication (agree on ID/data channel)
    // 5. Free allocated packets
    //
    // 

    void* bytes = mb.GetData();
    (bytes);
    LF_DEBUG_BREAK;
}

REGISTER_TEST(NetTransportTest)
{
    NetTestInitializer init;
    
    NetTransportConfig config;
    config.SetPort(TEST_PORT);
    config.SetAppId(NetConfig::NET_APP_ID);
    config.SetAppVersion(NetConfig::NET_APP_VERSION);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_MESSAGE, LFNew<MessageTransportHandler>());

    NetTransport transport;
    transport.Start(std::move(config));
    // transport.Send(packet);

    // SendRemoteMethodCall( methodIndex, connection, serializedData )
    // SendReplicationData( connection, serializedData )
    // 

    

    SleepCallingThread(1000);

    // Fake some network traffic...
    ByteT packetBytes[256];
    SizeT packetSize = sizeof(packetBytes);
    Crypto::SecureRandomBytes(packetBytes, packetSize);
    PacketHeader* header = reinterpret_cast<PacketHeader*>(packetBytes);
    header->mAppID = NetConfig::NET_APP_ID;
    header->mAppVersion = NetConfig::NET_APP_VERSION;
    header->mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_RELIABILITY }).value;
    header->mType = NetPacketType::NET_PACKET_TYPE_MESSAGE;
    header->mCrc32 = Crc32(&packetBytes[PacketHeader::CRC_OFFSET], packetSize - PacketHeader::CRC_OFFSET);

    IPEndPointAny local;
    IPV4(local, "127.0.0.1", TEST_PORT);

    UDPSocket client;
    TEST(client.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP));
    TEST(client.SendTo(packetBytes, packetSize, local));

    SleepCallingThread(1000);
    transport.Stop();
}

REGISTER_TEST(NetTest)
{
    bool doTerminate = false;
    if (!IsNetInitialized())
    {
        TEST_CRITICAL(NetInitialize());
        TEST(IsNetInitialized());
        doTerminate = true;
    }

    auto config = TestFramework::GetConfig();
    TestFramework::ExecuteTest("IPEndPointTest", config);
    TestFramework::ExecuteTest("ByteOrderSwapTest", config);
    TestFramework::ExecuteTest("NetTransportTest", config);
    TestFramework::TestReset();

    if (doTerminate)
    {
        TEST(NetShutdown());
        TEST(!IsNetInitialized());
    }

}

}
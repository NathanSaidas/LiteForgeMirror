// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Runtime/PCH.h"
#include "NetSecureServerDriver.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/IO/BinaryStream.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Platform/Atomic.h"
#include "Core/Utility/Log.h"
#include "Runtime/Net/NetMessage.h"
#include "Runtime/Net/NetSerialization.h"
#include "Runtime/Net/PacketSerializer.h"
#include "Runtime/Net/Controllers/NetMessageController.h"
#include "Runtime/Net/Controllers/NullMessageController.h"

namespace lf {

NetSecureServerDriver::NetSecureServerDriver()
: Super()
, mCertificateKey()
, mAppID(0)
, mAppVersion(0)
, mMaxHeartbeatDelta(20.0f)
, mAckTimeout(3.0f)
, mMaxRetransmit(3)
// ** Internal Server Resources
, mSocket()
, mRunning(0)
, mThread()
// ** Connection Control
, mPrimaryConnectionMapLock()
, mPrimaryConnectionMap()
, mConnectionMapLock()
, mConnectionMap()
, mNewConnectionsLock()
, mNewConnections()
, mConnections()
// ** Message Processing
, mMessageControllerLocks()
, mMessageControllers()
, mMessageMapLock()
, mMessageMap()
, mNewMessagesLock()
, mNewMessages()
, mClientHelloTransmitBuffer()
, mPacketProcessLock()
, mStats()
{

}

NetSecureServerDriver::~NetSecureServerDriver()
{

}

bool NetSecureServerDriver::Initialize(const NetServerDriverConfig& config)
{
    if (IsRunning())
    {
        return false;
    }

    if (!config.mCertificate->HasPrivateKey() || config.mCertificate->GetKeySize() != Crypto::RSA_KEY_2048)
    {
        return false;
    }

    mAppID = config.mAppID;
    mAppVersion = config.mAppVersion;
    mCertificateKey = *config.mCertificate;

    if (!mSocket.Create(config.mProtocol))
    {
        gNetLog.Info(LogMessage("Failed to initialize inbound socket."));
        return false;
    }

    if (!mSocket.Bind(config.mPort))
    {
        gNetLog.Info(LogMessage("Failed to bind inbound socket."));
        mSocket.Close();
        return false;
    }

    // Scale this number up if you expect a lot of connection attempts.
    const SizeT NET_TRANSMIT_CLIENT_HELLO_SIZE = 100;
    mClientHelloTransmitBuffer.Resize(NET_TRANSMIT_CLIENT_HELLO_SIZE);

    SetRunning(true);

    mThread.Fork([](void* param) {
        reinterpret_cast<NetSecureServerDriver*>(param)->ProcessBackground();
    }, this);
    mThread.SetDebugName("NetServer_Background");

    return true;
}

void NetSecureServerDriver::Shutdown()
{
    SetRunning(false);
    bool closeSocket = true;
    if (mSocket.IsAwaitingReceive())
    {
        mSocket.Shutdown();
        closeSocket = false;
    }
    mThread.Join();
    if (closeSocket)
    {
        mSocket.Close();
    }

    for (SizeT i = 0; i < LF_ARRAY_SIZE(mMessageControllers); ++i)
    {
        ScopeRWSpinLockWrite lock(mMessageControllerLocks[i]);
        if (mMessageControllers[i])
        {
            mMessageControllers[i]->OnShutdown();
            mMessageControllers[i].Release();
        }
    }
}

void NetSecureServerDriver::Update()
{
    UpdateConnections();
    UpdateMessages();
}

void NetSecureServerDriver::SetMessageController(MessageType messageType, NetMessageController* controller)
{
    ScopeRWSpinLockWrite lock(mMessageControllerLocks[messageType]);
    if (mMessageControllers[messageType])
    {
        mMessageControllers[messageType]->OnShutdown();
    }
    mMessageControllers[messageType] = GetPointer(controller);
    if (mMessageControllers[messageType])
    {
        mMessageControllers[messageType]->OnInitialize(this);
    }
}

bool NetSecureServerDriver::Send(
    MessageType message,
    Options options,
    const ByteT* bytes,
    const SizeT numBytes,
    OnSendSuccess onSuccess,
    OnSendFailed onFailed)
{
    (message);
    (options);
    (bytes);
    (numBytes);
    (onSuccess);
    (onFailed);
    ReportBugMsg("Invalid operation, cannot send to 'no one' as a server.");
    return false;
}

bool NetSecureServerDriver::Send(
    MessageType messageType,
    Options options,
    const ByteT* bytes,
    const SizeT numBytes,
    NetConnection* connection,
    OnSendSuccess onSuccess,
    OnSendFailed onFailed)
{
    MessagePtr message(LFNew<NetMessage>());
    message->SetSuccessCallback(onSuccess);
    message->SetFailureCallback(onFailed);
    if (!message->Initialize(messageType, options, bytes, numBytes))
    {
        return false;
    }
    message->SetConnection(connection); // todo: Verify there is some association with this connection and this NetDriver

    ScopeLock lock(mNewMessagesLock);
    mNewMessages.push_back(message);
    return true;
}

bool NetSecureServerDriver::IsServer() const
{
    return true;
}
bool NetSecureServerDriver::IsClient() const
{
    return false;
}

void NetSecureServerDriver::ProcessBackground()
{
    ByteT bytes[2048];
    while (IsRunning())
    {
        SizeT receivedBytes = sizeof(bytes);
        IPEndPointAny sender;
        if (!mSocket.ReceiveFrom(bytes, receivedBytes, sender) || !IsRunning())
        {
            continue;
        }
        
        ProcessPacketData(bytes, receivedBytes, sender);
    }

    gSysLog.Info(LogMessage("Terminating NetSecureServerDriver::ProcessBackground"));
}

bool NetSecureServerDriver::IsRunning() const
{
    return AtomicLoad(&mRunning) != 0;
}

void NetSecureServerDriver::ProcessPacketData(const ByteT* bytes, const SizeT numBytes, const IPEndPointAny& endPoint)
{
    ScopeLock lock(mPacketProcessLock);

    if (mPacketFilter.IsValid() && mPacketFilter.Invoke(bytes, numBytes, endPoint))
    {
        return;
    }
    AtomicIncrement64(&mStats.mPacketsReceived);
    AtomicAdd64(&mStats.mBytesReceived, numBytes);

    PacketSerializer ps;
    if (!ps.SetBuffer(bytes, numBytes))
    {
        gSysLog.Info(LogMessage("Dropping packet, not enough bytes for header. Bytes=") << numBytes);
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    UInt16 appID = ps.GetAppId();
    UInt16 appVersion = ps.GetAppVersion();

    if (appID != mAppID)
    {
        gSysLog.Info(LogMessage("Dropping packet, invalid app ID. appID=") << appID);
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    if (appVersion != mAppVersion)
    {
        gSysLog.Info(LogMessage("Dropping packet, invalid app version. appVersion=") << appVersion);
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    UInt32 crc32 = ps.GetCrc32();
    UInt32 calcCrc32 = ps.CalcCrc32();
    if (crc32 != calcCrc32)
    {
        gSysLog.Info(LogMessage("Dropping packet, invalid CRC32. crc32=") << crc32 << ", calcCrc32=" << calcCrc32);
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    UInt8 packetType = ps.GetType();
    switch (packetType)
    {
        case NetPacketType::NET_PACKET_TYPE_CLIENT_HELLO:
        {
            if (mClientHelloTransmitBuffer.Update(NetTransmitInfo(ps.GetPacketUID(), ps.GetCrc32())))
            {
                AcceptConnection(bytes, numBytes, endPoint);
            }
            else
            {
                AtomicIncrement64(&mStats.mDroppedDuplicatePackets);
            }
        } break;
        case NetPacketType::NET_PACKET_TYPE_HEARTBEAT:
        {
            OnHeartbeat(bytes, numBytes, endPoint);
        } break;
        case NetPacketType::NET_PACKET_TYPE_SERVER_HELLO:
        {
            OnServerHelloAck(bytes, numBytes, endPoint);
        } break;
        case NetPacketType::NET_PACKET_TYPE_RESPONSE:
        {
            if (ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK))
            {
                OnResponseAck(bytes, numBytes);
            }
            else
            {
                OnResponse(bytes, numBytes);
            }
        } break;
        case NetPacketType::NET_PACKET_TYPE_REQUEST:
        {
            if (ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK))
            {
                OnRequestAck(bytes, numBytes);
            }
            else
            {
                OnRequest(bytes, numBytes);
            }
        } break;
        case NetPacketType::NET_PACKET_TYPE_MESSAGE:
        {
            if (ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK))
            {
                OnMessageAck(bytes, numBytes);
            }
            else
            {
                OnMessage(bytes, numBytes);
            }
        } break;
        default:
        {
            gSysLog.Info(LogMessage("Dropping packet, unsupported type. type=") << packetType);
            AtomicIncrement64(&mStats.mDroppedPackets);
        } break;
    }
}

NetConnectionAtomicPtr NetSecureServerDriver::FindConnection(const SessionID& sessionID) const
{
    ScopeRWSpinLockRead lock(mConnectionMapLock);
    auto iter = mConnectionMap.find(sessionID);
    return iter != mConnectionMap.end() ? iter->second : NULL_PTR;
}

SizeT NetSecureServerDriver::GetDroppedPackets() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mDroppedPackets));
}
SizeT NetSecureServerDriver::GetPacketsSent() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mPacketsSent));
}
SizeT NetSecureServerDriver::GetBytesSent() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mBytesSent));
}
SizeT NetSecureServerDriver::GetPacketsReceived() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mPacketsReceived));
}
SizeT NetSecureServerDriver::GetBytesReceived() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mBytesReceived));
}
SizeT NetSecureServerDriver::GetRetransmits() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mRetransmits));
}
SizeT NetSecureServerDriver::GetConnectionsAccepted() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mConnectionsAccepted));
}
SizeT NetSecureServerDriver::GetDroppedDuplicatePackets() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mDroppedDuplicatePackets));
}

void NetSecureServerDriver::LogStats(LoggerMessage& msg) const
{
    msg << "\n        Packets Sent= " << GetPacketsSent()
        << "\n          Bytes Sent= " << GetBytesSent()
        << "\n    Packets Received= " << GetPacketsReceived()
        << "\n      Bytes Received= " << GetBytesReceived()
        << "\n     Dropped Packets= " << GetDroppedPackets()
        << "\n         Retransmits= " << GetRetransmits() 
        << "\nConnections Accepted= " << GetConnectionsAccepted() 
        << "\n  Dropped Duplicates= " << GetDroppedDuplicatePackets() << "\n";
}

void NetSecureServerDriver::SetRunning(bool value)
{
    AtomicStore(&mRunning, value);
}

void NetSecureServerDriver::CreateSessionFromBytes(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint)
{
    ConnectionPtr connection = MakeConvertibleAtomicPtr<NetSecureServerConnection>();
    
    connection->SetState(ConnectionState::NetworkInit);
    if (!connection->SerializeClientHandshakeData(bytes, bytesSize))
    {
        return;
    }

    // Allocate Session ID
    {
        ScopeLock lock(mPrimaryConnectionMapLock);
        SessionID id;
        for (SizeT i = 0; i < 10; ++i)
        {
            Crypto::SecureRandomBytes(id.Bytes(), id.Size());
            if (mPrimaryConnectionMap.find(id) == mPrimaryConnectionMap.end())
            {
                break;
            }
            id = SessionID();
        }

        if (id.Empty()) // Failed to create session: Could not allocate an ID
        {
            return;
        }

        gSysLog.Info(LogMessage("Allocate session ") << BytesToHex(id.Bytes(), id.Size()));
        if (!connection->Initialize(id, &mCertificateKey, endPoint))
        {
            return;
        }
        mPrimaryConnectionMap.insert(std::make_pair(id, connection));
    }

    {
        ScopeLock lock(mNewConnectionsLock);
        mNewConnections.push_back(connection);
        AtomicIncrement64(&mStats.mConnectionsAccepted);
    }
}

void NetSecureServerDriver::AcceptConnection(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint)
{
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, bytesSize));
    SendAck(bytes, bytesSize, endPoint);
    
    ByteT data[1300];
    SizeT dataSize = sizeof(data);
    if (!ps.GetData(data, dataSize))
    {
        gNetLog.Error(LogMessage("Failed to get data from CLIENT_HELLO packet."));
        return;
    }
    
    const SizeT KEY_SIZE = mCertificateKey.GetKeySizeBytes();

    ByteT plainText[1300];
    SizeT plainTextSize = sizeof(plainText);

    if (dataSize < KEY_SIZE)
    {
        gNetLog.Error(LogMessage("CLIENT_HELLO contains invalid data size."));
        return; // Missing RSA Block
    }

    // todo: This is not thread-safe... We can duplicate the cert key foreach worker though.
    if (!Crypto::RSADecryptPrivate(&mCertificateKey, data, KEY_SIZE, plainText, plainTextSize))
    {
        gNetLog.Error(LogMessage("CLIENT_HELLO failed to decrypt data."));
        return;
    }

    Crypto::AESKey oneTimeKey;
    Crypto::AESIV  oneTimeIv;
    {
        NetOneTimeKeyMsg msg;
        msg.mOneTimeKey = Crypto::AES256KeySerialized(&oneTimeKey);
        msg.mOneTimeIV = Crypto::AESIVSerialized(&oneTimeIv);

        MemoryBuffer buffer(plainText, plainTextSize);
        Assert(buffer.Allocate(plainTextSize, 1));
        BinaryStream bs(Stream::MEMORY, &buffer, Stream::SM_READ);
        if (bs.BeginObject("o", "o"))
        {
            bs << msg;
            bs.EndObject();
        }
        bs.Close();
    }

    plainTextSize = sizeof(plainText) - KEY_SIZE;
    if (!Crypto::AESDecrypt(&oneTimeKey, oneTimeIv.mBytes, &data[KEY_SIZE], dataSize - KEY_SIZE, &plainText[KEY_SIZE], plainTextSize))
    {
        gNetLog.Error(LogMessage("CLIENT_HELLO failed to decrypt data-bulk."));
        return;
    }

    gNetLog.Info(LogMessage("Creating session from bytes..."));
    CreateSessionFromBytes(&plainText[KEY_SIZE], plainTextSize, endPoint);
}

void NetSecureServerDriver::OnHeartbeat(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint)
{
    (endPoint); // Unused

    PacketSerializer ps;
    ps.SetBuffer(bytes, bytesSize);

    if (ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK))
    {
        return; // Server shouldn't receive heartbeat acks
    }

    ConnectionPtr connection = StaticCast<ConnectionPtr>(FindConnection(ps.GetSessionID()));
    if (!connection)
    {
        return;
    }

    Crypto::HMACBuffer serverHMAC;
    if (!ps.ComputeHeaderHmac(&connection->GetDerivedHMAC(), serverHMAC) || serverHMAC != ps.GetEncryptedHMAC())
    {
        return;
    }

    connection->OnHeartbeatTick();
    SendAck(bytes, bytesSize, connection, nullptr, 0);
    gSysLog.Info(LogMessage("Server: Send heartbeat ") << connection->GetConnectionName());
}

void NetSecureServerDriver::OnServerHelloAck(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint)
{
    (endPoint); // unused;

    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, bytesSize));
    if (!ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK))
    {
        return;
    }

    ConnectionPtr connection = StaticCast<ConnectionPtr>(FindConnection(ps.GetSessionID()));
    if (!connection)
    {
        return;
    }

    Crypto::HMACBuffer serverHmac;
    if (!ps.ComputeHeaderHmac(&connection->GetDerivedHMAC(), serverHmac) || serverHmac != ps.GetEncryptedHMAC())
    {
        return;
    }

    connection->OnHeartbeatTick();
}

void NetSecureServerDriver::OnResponse(const ByteT* bytes, SizeT numBytes)
{
    ScopeRWSpinLockRead lock(mMessageControllerLocks[MESSAGE_RESPONSE]);
    ProcessMessage(bytes, numBytes, mMessageControllers[MESSAGE_RESPONSE]);
}
void NetSecureServerDriver::OnRequest(const ByteT* bytes, SizeT numBytes)
{
    ScopeRWSpinLockRead lock(mMessageControllerLocks[MESSAGE_REQUEST]);
    ProcessMessage(bytes, numBytes, mMessageControllers[MESSAGE_REQUEST]);
}
void NetSecureServerDriver::OnMessage(const ByteT* bytes, SizeT numBytes)
{
    ScopeRWSpinLockRead lock(mMessageControllerLocks[MESSAGE_GENERIC]);
    ProcessMessage(bytes, numBytes, mMessageControllers[MESSAGE_GENERIC]);
}
void NetSecureServerDriver::OnResponseAck(const ByteT* bytes, SizeT numBytes)
{
    ProcessMessageAck(bytes, numBytes);
}
void NetSecureServerDriver::OnRequestAck(const ByteT* bytes, SizeT numBytes)
{
    ProcessMessageAck(bytes, numBytes);
}
void NetSecureServerDriver::OnMessageAck(const ByteT* bytes, SizeT numBytes)
{
    ProcessMessageAck(bytes, numBytes);
}

void NetSecureServerDriver::ProcessMessage(const ByteT* bytes, SizeT numBytes, NetMessageController* controller)
{
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes));
    ConnectionPtr connection = StaticCast<ConnectionPtr>(FindConnection(ps.GetSessionID()));
    if (!connection)
    {
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    Crypto::HMACBuffer hmac;
    if (!ps.ComputeHeaderHmac(&connection->GetDerivedHMAC(), hmac) || hmac != ps.GetEncryptedHMAC())
    {
        if (controller)
        {
            NetMessageDataErrorArgs args;
            args.mPacketData = bytes;
            args.mPacketDataLength = numBytes;
            args.mConnection = connection;
            args.mError = NetMessageDataError::DATA_ERROR_INVALID_HEADER_HMAC;
            controller->OnMessageDataError(args);
        }
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    // Emit the Ack?
    NetTransmitInfo transmitID(ps.GetPacketUID(), ps.GetCrc32());
    UInt64 id = transmitID.Value();
    SendAck(bytes, numBytes, connection, reinterpret_cast<ByteT*>(&id), sizeof(id));
    if (!controller)
    {
        return;
    }

    // Already received?
    NetTransmitBuffer& buffer = connection->GetTransmitBuffer(static_cast<NetPacketType::Value>(ps.GetType()));
    if (!buffer.Update(transmitID))
    {
        // We ack anyways just so they know they got it, or rather have a better chance of knowing
        AtomicIncrement64(&mStats.mDroppedDuplicatePackets);
        return;
    }

    const bool signVerify = ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED);
    const bool hmacVerify = ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC);
    const bool encrypted = true; //  ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_SECURE); // todo: 
    if (signVerify && !ps.Verify(&connection->GetClientSigningKey()))
    {
        NetMessageDataErrorArgs args;
        args.mPacketData = bytes;
        args.mPacketDataLength = numBytes;
        args.mConnection = connection;
        args.mError = NetMessageDataError::DATA_ERROR_INVALID_SIGNATURE;
        controller->OnMessageDataError(args);
        return;
    }

    ByteT cipherText[1500] = { 0 };
    SizeT cipherTextLength = sizeof(cipherText);
    if (!ps.GetData(cipherText, cipherTextLength))
    {
        NetMessageDataErrorArgs args;
        args.mPacketData = bytes;
        args.mPacketDataLength = numBytes;
        args.mConnection = connection;
        args.mError = NetMessageDataError::DATA_ERROR_DATA_RETRIEVAL;
        controller->OnMessageDataError(args);
        return;
    }

    if (hmacVerify)
    {
        Crypto::HMACBuffer dataHmac;
        if (!ps.GetDataHMAC(dataHmac)
            || !connection->GetDerivedHMAC().Compute(cipherText, cipherTextLength, hmac)
            || dataHmac != hmac)
        {
            NetMessageDataErrorArgs args;
            args.mPacketData = bytes;
            args.mPacketDataLength = numBytes;
            args.mConnection = connection;
            args.mError = NetMessageDataError::DATA_ERROR_INVALID_HMAC;
            controller->OnMessageDataError(args);
            return;
        }
    }

    ByteT plainText[1500] = { 0 };
    SizeT plainTextLength = sizeof(plainText);
    ByteT* dataPtr = cipherText;
    SizeT  dataLength = cipherTextLength;
    if (encrypted)
    {
        if (!Crypto::AESDecrypt(&connection->GetDerivedSecretKey(), ps.GetIV().mBytes, cipherText, cipherTextLength, plainText, plainTextLength))
        {
            NetMessageDataErrorArgs args;
            args.mPacketData = bytes;
            args.mPacketDataLength = numBytes;
            args.mConnection = connection;
            args.mError = NetMessageDataError::DATA_ERROR_DATA_DECRYPTION;
            controller->OnMessageDataError(args);
            return;
        }
        dataPtr = plainText;
        dataLength = plainTextLength;
    }

    {
        NetMessageDataArgs args;
        args.mAppData = dataPtr;
        args.mAppDataSize = dataLength;
        args.mConnection = connection;
        args.mEncrypted = encrypted;
        args.mHmacVerified = hmacVerify;
        args.mSignatureVerified = signVerify;
        controller->OnMessageData(args);
    }
}

void NetSecureServerDriver::ProcessMessageAck(const ByteT* bytes, SizeT numBytes) 
{
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes));

    ConnectionPtr connection = StaticCast<ConnectionPtr>(FindConnection(ps.GetSessionID()));
    if (!connection)
    {
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    Crypto::HMACBuffer hmac;
    if (!ps.ComputeHeaderHmac(&connection->GetDerivedHMAC(), hmac) || hmac != ps.GetEncryptedHMAC())
    {
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }
    
    UInt64 id;
    SizeT  dataSize = sizeof(id);
    if (!ps.GetData(reinterpret_cast<ByteT*>(&id), dataSize))
    {
        return;
    }

    ScopeRWSpinLockRead lock(mMessageMapLock);
    auto iter = mMessageMap.find(id);
    if (iter != mMessageMap.end() && iter->second)
    {
        NetMessage* message = iter->second;
        message->SetState(NetMessage::Success);
    }
}

void NetSecureServerDriver::UpdateConnections()
{
    // Accept new connections
    {
        ScopeLock lock(mNewConnectionsLock);
        mConnections.insert(mConnections.end(), mNewConnections.begin(), mNewConnections.end());
        ScopeRWSpinLockWrite write(mConnectionMapLock);
        for (auto it = mNewConnections.begin(), end = mNewConnections.end(); it != end; ++it)
        {
            mConnectionMap.insert(std::make_pair((*it)->GetConnectionID(), *it));
        }
        mNewConnections.resize(0);
    }

    // Update all connections
    for (ConnectionType* connection : mConnections)
    {
        UpdateConnection(connection);
    }

    // Mark garbage
    TVector<ConnectionPtr> garbage;
    for (auto it = mConnections.begin(); it != mConnections.end();)
    {
        switch ((*it)->GetState())
        {
        case ConnectionState::Failed:
        case ConnectionState::Disconnected:
            garbage.push_back(*it);
            it = mConnections.swap_erase(it);
            break;
        default:
            ++it;
            break;
        }
    }

    // Sweep garbage
    if (!garbage.empty())
    {
        gSysLog.Info(LogMessage("Cleaning up ") << garbage.size() << " garbage connections.");
        {
            ScopeLock lock(mPrimaryConnectionMapLock);
            for (ConnectionPtr& connection : garbage)
            {
                Assert(mPrimaryConnectionMap.erase(connection->GetConnectionID()) == 1);
            }
        }
        {
            ScopeRWSpinLockWrite lock(mConnectionMapLock);
            for (ConnectionPtr& connection : garbage)
            {
                Assert(mConnectionMap.erase(connection->GetConnectionID()) == 1);
                Assert(connection.GetStrongRefs() == 1);
            }
        }
        {
            // Notify controllers of disconnections
            for (ConnectionType* connection : garbage)
            {
                for (SizeT i = 0; i < LF_ARRAY_SIZE(mMessageControllers); ++i)
                {
                    ScopeRWSpinLockRead lock(mMessageControllerLocks[i]);
                    if (mMessageControllers[i])
                    {
                        mMessageControllers[i]->OnDisconnect(connection);
                    }
                }
            }
        }
    }
}

void NetSecureServerDriver::UpdateConnection(ConnectionType* connection)
{
    switch (connection->GetState())
    {
        case ConnectionState::NetworkInit: UpdateNetworkInit(connection); break;
        case ConnectionState::ServerHello: UpdateServerHello(connection); break;
        case ConnectionState::ServerReady: UpdateServerReady(connection); break;
        default:
            break;

    }
}
void NetSecureServerDriver::UpdateNetworkInit(ConnectionType* connection)
{
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mMessageControllers); ++i)
    {
        ScopeRWSpinLockRead lock(mMessageControllerLocks[i]);
        if (mMessageControllers[i])
        {
            mMessageControllers[i]->OnConnect(connection);
        }
    }
    connection->SetState(ConnectionState::ServerHello);
}
void NetSecureServerDriver::UpdateServerHello(ConnectionType* connection)
{
    if (!connection->GenerateServerHandshakeData())
    {
        connection->SetState(ConnectionState::Failed);
        return;
    }

    NetServerDriverConfig config;
    config.mAppID = mAppID;
    config.mAppVersion = mAppVersion;
    config.mMaxRetransmit = mMaxRetransmit;

    if (!connection->GenerateServerHelloPacket(config))
    {
        connection->SetState(ConnectionState::Failed);
        return;
    }

    SizeT numBytes = 0;
    {
        ScopeLock connectionLock(connection->GetHandshakeLock());
        ServerHelloPacketData* packet = connection->GetServerHelloMsg();
        Assert(packet->mRetransmits > 0);
        --packet->mRetransmits;
        numBytes = packet->mSize;
        if (!mSocket.SendTo(packet->mBytes, numBytes, connection->GetEndPoint()) || numBytes != packet->mSize)
        {
            connection->SetState(ConnectionState::Failed);
            return;
        }

    }
    AtomicIncrement64(&mStats.mPacketsSent);
    AtomicAdd64(&mStats.mBytesSent, numBytes);
    connection->WaitHandshake();
    connection->SetState(ConnectionState::ServerReady);
}
void NetSecureServerDriver::UpdateServerReady(ConnectionType* connection)
{
    if (!connection->IsHandshakeComplete())
    {
        ScopeLock connectionLock(connection->GetHandshakeLock());
        if (!connection->IsHandshakeComplete())
        {
            ServerHelloPacketData* packet = connection->GetServerHelloMsg();
            if (connection->GetHeartbeatDelta() > mAckTimeout)
            {
                if (packet->mRetransmits == 0)
                {
                    connection->SetState(ConnectionState::Failed);
                    return;
                }

                --packet->mRetransmits;
                SizeT numBytes = packet->mSize;
                if (!mSocket.SendTo(packet->mBytes, numBytes, connection->GetEndPoint()) || numBytes != packet->mSize)
                {
                    connection->SetState(ConnectionState::Failed);
                    return;
                }
                AtomicIncrement64(&mStats.mPacketsSent);
                AtomicAdd64(&mStats.mBytesSent, numBytes);
                connection->WaitHandshake();
            }
        }
    }

    if (connection->GetHeartbeatDelta() > mMaxHeartbeatDelta)
    {
        gSysLog.Info(LogMessage("Server: Session disconnected ") << connection->GetConnectionName());
        connection->SetState(ConnectionState::Disconnected);
    }
}

void NetSecureServerDriver::UpdateMessages()
{
    // Accept new messages:
    {
        ScopeLock lock(mNewMessagesLock);
        mMessages.insert(mMessages.begin(), mNewMessages.begin(), mNewMessages.end());
        mNewMessages.resize(0);
    }

    // Update:
    TVector<MessagePtr> messages;
    for (MessagePtr& message : mMessages)
    {
        NetMessage::State oldState = message->GetState();
        UpdateMessage(message);
        NetMessage::State newState = message->GetState();
        if (oldState == NetMessage::SerializeData && newState == NetMessage::Register)
        {
            messages.push_back(message);
        }
    }

    // Register:
    if (!messages.empty())
    {
        ScopeRWSpinLockWrite lock(mMessageMapLock);
        for (MessagePtr& message : messages)
        {
            // todo: VERY unlikely but we could have 2 messages with same ID|Crc32...
            Assert(mMessageMap.find(message->GetID()) == mMessageMap.end());
            // For message tracking we could maybe offload it to the individual connections

            mMessageMap.insert(std::make_pair(message->GetID(), message));
            message->SetState(NetMessage::Transmit);
        }
    }

    // Mark
    messages.resize(0);
    for (auto iter = mMessages.begin(); iter != mMessages.end();)
    {
        if ((*iter)->GetState() == NetMessage::Garbage)
        {
            messages.push_back(*iter);
            iter = mMessages.swap_erase(iter);
        }
        else
        {
            ++iter;
        }
    }

    // Sweep:
    if (!messages.empty())
    {
        ScopeRWSpinLockWrite lock(mMessageMapLock);
        for (NetMessage* message : messages)
        {
            mMessageMap.erase(message->GetID());
        }
    }
}

void NetSecureServerDriver::UpdateMessage(NetMessage* message)
{
    switch (message->GetState())
    {
        case NetMessage::SerializeData: UpdateMessageSerialize(message); break;
        case NetMessage::Transmit: UpdateMessageTransmit(message); break;
        case NetMessage::Failed:
        case NetMessage::Success:
            UpdateMessageFinal(message);
            break;
        case NetMessage::Garbage:
            break;
        default:
            CriticalAssertMsg("Invalid message state.");
            break;
    }
}
void NetSecureServerDriver::UpdateMessageSerialize(NetMessage* message)
{
    NetSecureServerConnection* connection = static_cast<NetSecureServerConnection*>(message->GetConnection());
    if (!connection || connection->GetState() != NetSecureServerConnection::ServerReady)
    {
        message->SetState(NetMessage::Failed);
        return;
    }

    NetKeySet keySet;
    keySet.mDerivedSecretKey = &connection->GetDerivedSecretKey();
    keySet.mHmacKey = &connection->GetDerivedHMAC();
    keySet.mSigningKey = &connection->GetServerSigningKey();
    keySet.mVerifyKey = nullptr; // Shouldn't be needed for this op

    NetServerDriverConfig config;
    config.mAppID = mAppID;
    config.mAppVersion = mAppVersion;
    config.mMaxRetransmit = mMaxRetransmit;

    if (!message->Serialize(connection->GetPacketUID(), keySet, config))
    {
        message->SetState(NetMessage::Failed);
        return;
    }
    message->SetState(NetMessage::Register);
}
void NetSecureServerDriver::UpdateMessageTransmit(NetMessage* message)
{
    NetSecureServerConnection* connection = static_cast<NetSecureServerConnection*>(message->GetConnection());
    if (!connection || connection->GetState() != connection->ServerReady)
    {
        message->SetState(NetMessage::Failed);
        return;
    }

    if (!message->HasTransmitStarted() || (message->GetTransmitRemaining() > 0 && message->GetTransmitDelta() > mAckTimeout))
    {
        SizeT numBytes = message->GetPacketBytesSize();
        if (!mSocket.SendTo(message->GetPacketBytes(), numBytes, connection->GetEndPoint()) || numBytes != message->GetPacketBytesSize())
        {
            message->SetState(NetMessage::Failed);
            return;
        }
        message->OnTransmit();
    }
}
void NetSecureServerDriver::UpdateMessageFinal(NetMessage* message)
{
    if (message->GetState() == NetMessage::Failed)
    {
        message->OnFailed();
    }
    else if (message->GetState() == NetMessage::Success)
    {
        message->OnSuccess();
    }
    message->SetState(NetMessage::Garbage);
}

void NetSecureServerDriver::SendAck(const ByteT* bytes, SizeT numBytes, ConnectionType* connection, const ByteT* data, SizeT dataLength)
{
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes)); // This should not fail as we should've already passed basic header checks in the initial processing

    ByteT ackBytes[256] = { 0 };
    PacketSerializer ack;
    ack.SetBuffer(ackBytes, sizeof(ackBytes));
    ack.SetAppId(mAppID);
    ack.SetAppVersion(mAppVersion);
    ack.SetFlag(NetPacketFlag::NET_PACKET_FLAG_ACK);
    ack.SetType(ps.GetType());
    ack.SetPacketUID(ps.GetPacketUID());
    ack.SetSessionID(ps.GetSessionID());

    if (data && !ack.SetData(data, dataLength))
    {
        gNetLog.Error(LogMessage("Failed to send Ack to connection. Packet data could not be set."));
        connection->SetState(ConnectionState::Failed);
        return;
    }

    if (connection->GetState() == ConnectionState::ServerReady)
    {
        Crypto::AESIV iv;
        Crypto::HMACBuffer hmac;
        Crypto::SecureRandomBytes(iv.mBytes, sizeof(iv.mBytes));
        ack.SetIV(iv);
        if (!ack.ComputeHeaderHmac(&connection->GetDerivedHMAC(), hmac))
        {
            connection->SetState(ConnectionState::Failed);
            return;
        }
        ack.SetEncryptedHMAC(hmac);
    }
    else
    {
        gNetLog.Warning(LogMessage("The session is not ServerReady but we're trying to ack!"));
        ack.SetIV(Crypto::AESIV());
        ack.SetEncryptedHMAC(Crypto::HMACBuffer()); // todo: Someone can forge the Ack's giving the client/server a false positive
    }
    ack.SetCrc32(ack.CalcCrc32());

    // note: We don't retransmit acknowledgements
    SizeT ackSize = ack.GetPacketSize();
    if (!mSocket.SendTo(ackBytes, ackSize, connection->GetEndPoint()) || ackSize != ack.GetPacketSize())
    {
        connection->SetState(ConnectionState::Failed);
        return;
    }
    AtomicIncrement64(&mStats.mPacketsSent);
    AtomicAdd64(&mStats.mBytesSent, ackSize);
}

void NetSecureServerDriver::SendAck(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint)
{
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes)); // This should not fail as we should've already passed basic header checks in the initial processing

    ByteT ackBytes[256] = { 0 };
    PacketSerializer ack;
    ack.SetBuffer(ackBytes, sizeof(ackBytes));
    ack.SetAppId(mAppID);
    ack.SetAppVersion(mAppVersion);
    ack.SetFlag(NetPacketFlag::NET_PACKET_FLAG_ACK);
    ack.SetType(ps.GetType());
    ack.SetPacketUID(ps.GetPacketUID());
    ack.SetSessionID(ps.GetSessionID());
    ack.SetIV(Crypto::AESIV());
    ack.SetEncryptedHMAC(Crypto::HMACBuffer()); // todo: Someone can forge the Ack's giving the client/server a false positive
    ack.SetCrc32(ack.CalcCrc32());

    // note: We don't retransmit acknowledgements
    SizeT ackSize = ack.GetPacketSize();
    if (!mSocket.SendTo(ackBytes, ackSize, endPoint) || ackSize != ack.GetPacketSize())
    {
        return;
    }
    AtomicIncrement64(&mStats.mPacketsSent);
    AtomicAdd64(&mStats.mBytesSent, ackSize);
}

} // namesapce lf
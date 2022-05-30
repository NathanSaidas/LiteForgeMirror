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
#include "NetSecureClientDriver.h"
#include "Core/Common/Assert.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/IO/BinaryStream.h"
#include "Core/Memory/MemoryBuffer.h"
#include "Core/Net/NetFramework.h"
#include "Core/Platform/Atomic.h"
#include "Core/Utility/Log.h"
#include "Runtime/Net/NetMessage.h"
#include "Runtime/Net/Controllers/NetMessageController.h"
#include "Runtime/Net/Controllers/NullMessageController.h"
#include "Runtime/Net/NetSerialization.h"
#include "Runtime/Net/NetTransmit.h"
#include "Runtime/Net/PacketSerializer.h"

namespace lf {

// note: If we ever change the signature key size we should change this
static const SizeT SIGNATURE_KEY_SIZE = 256;


NetSecureClientDriver::NetSecureClientDriver()
: Super()
// ** Client Configuration
, mServerCertificateKey()
, mEndPoint()
, mAppID(0)
, mAppVersion(0)
, mAckTimeout(3.0f)
, mMaxRetransmit(3)
, mHeartbeatDelta(2.0f)
, mMaxHeartbeatDelta(20.0f)
// ** Keys
, mDerivedSecretKey()
, mDerivedHMAC()
, mClientSigningKey()
, mServerSigningKey()
, mSessionID()
, mLocalConnection()
, mProtocol()
// ** Internal Client Resources
, mSocket()
, mState(InitNetwork)
, mRunning(0)
, mPacketUID(0)
, mThread()
// ** Message Processing
, mMessageControllerLocks()
, mMessageControllers()
, mMessageMapLock()
, mMessageMap()
, mNewMessagesLock()
, mNewMessages()
, mMessages()
// ** Heartbeat Processing
, mHeartbeatTimer()
, mHeartbeatWait(false)
// ** Other
, mPacketProcessLock()
, mPacketFilter()
// ** Handshake
, mHandshakeLock()
, mHandshakeData()
, mWaitingHandshake(0)
// ** Stats
, mStats()
{
}

NetSecureClientDriver::~NetSecureClientDriver()
{

}

bool NetSecureClientDriver::Initialize(UInt16 appId, UInt16 appVersion, const IPEndPointAny& endPoint, const Crypto::RSAKey& serverCertificate)
{
    NetClientDriverConfig config;
    config.mAppID = appId;
    config.mAppVersion = appVersion;
    config.mEndPoint = endPoint;
    config.mCertificate = &serverCertificate;
    return Initialize(config);
}

bool NetSecureClientDriver::Initialize(const NetClientDriverConfig& config)
{
    if (IsRunning())
    {
        return false;
    }

    mAppID = config.mAppID;
    mAppVersion = config.mAppVersion;
    mEndPoint = config.mEndPoint;
    mServerCertificateKey.LoadPublicKey(config.mCertificate->GetPublicKey());
    if (!mServerCertificateKey.HasPublicKey() || mServerCertificateKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        SetState(Failed);
        return false;
    }
    mProtocol = config.mProtocol;
    Assert(mServerCertificateKey.GetKeySizeBytes() == SIGNATURE_KEY_SIZE);
    return true;
}

void NetSecureClientDriver::Shutdown()
{
    SetRunning(false);
    bool closeSocket = true;
    if (mSocket.IsAwaitingReceive())
    {
        mSocket.Shutdown();
        closeSocket = false;
    }
    if (mThread.IsRunning())
    {
        mThread.Join();
    }
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

void NetSecureClientDriver::Update()
{
    switch (GetState())
    {
    case InitNetwork: UpdateInitNetwork(); break;
    case ClientHello: UpdateClientHello(); break;
    case ClientWaitServerHello: UpdateWaitServerHello(); break;
    case ClientReady: UpdateClientReady(); break;
    default:
        break;
    }

    if (GetState() >= ClientWaitServerHello && GetState() < Disconnected)
    {
        if (mHeartbeatTimer.PeekDelta() > mMaxHeartbeatDelta)
        {
            gSysLog.Info(LogMessage("Client: Connection timed out."));
            SetState(Disconnected);
            for (SizeT i = 0; i < LF_ARRAY_SIZE(mMessageControllers); ++i)
            {
                ScopeRWSpinLockRead lock(mMessageControllerLocks[i]);
                if (mMessageControllers[i])
                {
                    mMessageControllers[i]->OnDisconnect(mLocalConnection);
                }
            }
        }
    }
}

void NetSecureClientDriver::SetMessageController(MessageType messageType, NetMessageController* controller)
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

bool NetSecureClientDriver::Send(MessageType messageType, Options options, const ByteT* bytes, SizeT numBytes, OnSendSuccess onSuccess, OnSendFailed onFailed)
{
    MessagePtr message(LFNew<NetMessage>());
    message->SetSuccessCallback(onSuccess);
    message->SetFailureCallback(onFailed);
    if (!message->Initialize(messageType, options, bytes, numBytes))
    {
        return false;
    }
    message->SetConnection(mLocalConnection); // todo: Verify there is some association with this connection and this NetDriver

    ScopeLock lock(mNewMessagesLock);
    mNewMessages.push_back(message);
    return true;
}

bool NetSecureClientDriver::Send(
    MessageType message,
    Options options,
    const ByteT* bytes,
    const SizeT numBytes,
    NetConnection* connection,
    OnSendSuccess onSuccess,
    OnSendFailed onFailed)
{
    (message);
    (options);
    (bytes);
    (numBytes);
    (connection);
    (onSuccess);
    (onFailed);

    // todo: This isn't implemented yet but perhaps we'll support 'Remote Connection' which means
    // we'll be aware of a client connected to the server but won't be able to directly message them.
    return false;
}

bool NetSecureClientDriver::IsServer() const
{
    return false;
}
bool NetSecureClientDriver::IsClient() const
{
    return true;
}

void NetSecureClientDriver::ProcessBackground()
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

    gSysLog.Info(LogMessage("Terminating NetSecureClientDriver::ProcessBackground"));
}

bool NetSecureClientDriver::IsRunning() const
{
    return AtomicLoad(&mRunning) != 0;
}

bool NetSecureClientDriver::IsConnected() const
{
    return GetState() == ClientReady;
}

bool NetSecureClientDriver::IsDisconnected() const
{
    return GetState() == Disconnected;
}

bool NetSecureClientDriver::IsFailed() const
{
    return GetState() == Failed;
}

void NetSecureClientDriver::ProcessPacketData(const ByteT* bytes, const SizeT numBytes, const IPEndPointAny& endPoint)
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
    case NetPacketType::NET_PACKET_TYPE_SERVER_HELLO:
    {
        OnServerHello(bytes, numBytes);
    } break;
    case NetPacketType::NET_PACKET_TYPE_HEARTBEAT:
    {
        OnServerHeartbeat(bytes, numBytes);
    } break;
    case NetPacketType::NET_PACKET_TYPE_CLIENT_HELLO:
    {
        OnClientHelloAck(bytes, numBytes);
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

SizeT NetSecureClientDriver::GetDroppedPackets() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mDroppedPackets));
}
SizeT NetSecureClientDriver::GetPacketsSent() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mPacketsSent));
}
SizeT NetSecureClientDriver::GetBytesSent() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mBytesSent));
}
SizeT NetSecureClientDriver::GetPacketsReceived() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mPacketsReceived));
}
SizeT NetSecureClientDriver::GetBytesReceived() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mBytesReceived));
}
SizeT NetSecureClientDriver::GetRetransmits() const
{
    return static_cast<SizeT>(AtomicLoad(&mStats.mRetransmits));
}
void NetSecureClientDriver::LogStats(LoggerMessage& msg) const
{
    msg << "\n        Packets Sent= " << GetPacketsSent()
        << "\n          Bytes Sent= " << GetBytesSent()
        << "\n    Packets Received= " << GetPacketsReceived()
        << "\n      Bytes Received= " << GetBytesReceived()
        << "\n     Dropped Packets= " << GetDroppedPackets()
        << "\n         Retransmits= " << GetRetransmits() << "\n";
}

void NetSecureClientDriver::SetRunning(bool value)
{
    AtomicStore(&mRunning, value);
}

void NetSecureClientDriver::SetState(State value)
{
    const char* STATE_STRINGS[] =
    {
        "InitNetwork",
        "ClientHello",
        "ClientWaitServerHello",
        "ClientReady",
        "Disconnected",
        "Failed"
    };
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(STATE_STRINGS) == Failed + 1);
    gSysLog.Info(LogMessage("NetSecureClientDriver::SetState[ ") << STATE_STRINGS[GetState()] << " -> " << STATE_STRINGS[value] << " ]");
    AtomicStore(&mState, value);
}
NetSecureClientDriver::State NetSecureClientDriver::GetState() const
{
    return static_cast<State>(AtomicLoad(&mState));
}

UInt32 NetSecureClientDriver::GetPacketUID()
{
    return AtomicIncrement32(&mPacketUID);
}

bool NetSecureClientDriver::IsHandshakeComplete() const
{
    return AtomicLoad(&mWaitingHandshake) == 0;
}

void NetSecureClientDriver::OnClientHelloAck(const ByteT* bytes, SizeT numBytes)
{
    if (GetState() > ClientWaitServerHello)
    {
        return;
    }

    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes));
    if (!ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK))
    {
        return;
    }

    ScopeLock lock(mHandshakeLock);
    if (mHandshakeData)
    {
        mHandshakeData->mPacketConnectionMessage.mRetransmits = 0;
    }
}

void NetSecureClientDriver::OnServerHello(const ByteT* bytes, SizeT numBytes)
{
    if (GetState() != ClientWaitServerHello)
    {
        gSysLog.Warning(LogMessage("Receiving a ServerHello message but the client is not in the state to receive it. State=") << GetState());
        return;
    }

    PacketSerializer ps;
    if (!ps.SetBuffer(bytes, numBytes))
    {
        SetState(Failed);
        return;
    }

    if (!ps.Verify(&mServerCertificateKey))
    {
        return;
    }

    ScopeLock lock(mHandshakeLock);
    // If we process 2 server hello messages at the same time (rare)
    if (GetState() != ClientWaitServerHello)
    {
        return;
    }

    ByteT data[1300];
    SizeT dataLength = sizeof(data);
    if (!ps.GetData(data, dataLength))
    {
        SetState(Failed);
        return;
    }

    Crypto::AESIV iv;
    SizeT offset = SIGNATURE_KEY_SIZE;
    if (!ReadServerHelloRSA(data, offset, iv))
    {
        SetState(Failed);
        return;
    }

    dataLength -= offset;
    if (!ReadServerHelloAES(&data[offset], dataLength, iv))
    {
        SetState(Failed);
        return;
    }

    Crypto::AESIV headerIV = ps.GetIV();

    // Detect: Malformed header
    // todo: We could just accept the encrypted iv since it's been signed.
    if (memcmp(iv.mBytes, headerIV.mBytes, sizeof(headerIV.mBytes)) != 0)
    {
        SetState(Failed);
        return;
    }

    mLocalConnection = MakeConvertibleAtomicPtr<NetSecureLocalClientConnection>();
    mLocalConnection->Initialize(mSessionID, mEndPoint);

    AtomicStore(&mWaitingHandshake, 0);
    mHandshakeData.Release();
    SetState(ClientReady);
    SendAck(bytes, numBytes, nullptr, 0); // todo: The server will expect us to send an ack that has the proper hmac header, but the client will have a delay
                              // since they must process the packet which might take some time...
    mHeartbeatTimer.Start();
    mHeartbeatWait = false;

    for (SizeT i = 0; i < LF_ARRAY_SIZE(mMessageControllers); ++i)
    {
        ScopeRWSpinLockRead readLock(mMessageControllerLocks[i]);
        if (mMessageControllers[i])
        {
            mMessageControllers[i]->OnConnect(mLocalConnection);
        }
    }

    // todo: We can zero this out now or maybe on the ack.
    // PacketData::SetZero(mPacketConnectionMessage);
}

void NetSecureClientDriver::UpdateInitNetwork()
{
    if (!mSocket.Create(mProtocol))
    {
        SetState(Failed);
        return;
    }

    if (!IPIsLocal(mEndPoint))
    {
        if (!mSocket.Bind(IPEndPointGetPort(mEndPoint)))
        {
            SetState(Failed);
            return;
        }
    }

    SetState(ClientHello);
    mStats = Stats();
}

void NetSecureClientDriver::OnServerHeartbeat(const ByteT* bytes, SizeT numBytes)
{
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes));

    if (!ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK))
    {
        return; // Client expects the server to ack this
    }

    Crypto::HMACBuffer clientHmac;
    if (!ps.ComputeHeaderHmac(&mDerivedHMAC, clientHmac) || clientHmac != ps.GetEncryptedHMAC())
    {
        return; // Malformed header?
    }

    mHeartbeatTimer.Start();
    mHeartbeatWait = false;
}

void NetSecureClientDriver::OnResponse(const ByteT* bytes, SizeT numBytes)
{
    ScopeRWSpinLockRead lock(mMessageControllerLocks[MESSAGE_RESPONSE]);
    ProcessMessage(bytes, numBytes, mMessageControllers[MESSAGE_RESPONSE]);
}
void NetSecureClientDriver::OnRequest(const ByteT* bytes, SizeT numBytes)
{
    ScopeRWSpinLockRead lock(mMessageControllerLocks[MESSAGE_REQUEST]);
    ProcessMessage(bytes, numBytes, mMessageControllers[MESSAGE_REQUEST]);
}
void NetSecureClientDriver::OnMessage(const ByteT* bytes, SizeT numBytes) 
{
    ScopeRWSpinLockRead lock(mMessageControllerLocks[MESSAGE_GENERIC]);
    ProcessMessage(bytes, numBytes, mMessageControllers[MESSAGE_GENERIC]);
}
void NetSecureClientDriver::OnResponseAck(const ByteT* bytes, SizeT numBytes)
{
    ProcessMessageAck(bytes, numBytes);
}
void NetSecureClientDriver::OnRequestAck(const ByteT* bytes, SizeT numBytes)
{
    ProcessMessageAck(bytes, numBytes);
}
void NetSecureClientDriver::OnMessageAck(const ByteT* bytes, SizeT numBytes)
{
    ProcessMessageAck(bytes, numBytes);
}

void NetSecureClientDriver::ProcessMessage(const ByteT* bytes, SizeT numBytes, NetMessageController* controller)
{
    (controller);
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes));

    Crypto::HMACBuffer hmac;
    if (!ps.ComputeHeaderHmac(&mDerivedHMAC, hmac) || hmac != ps.GetEncryptedHMAC())
    {
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    NetTransmitInfo transmitID(ps.GetPacketUID(), ps.GetCrc32());
    UInt64 id = transmitID.Value();
    SendAck(bytes, numBytes, reinterpret_cast<ByteT*>(&id), sizeof(id));

    // todo: 
    gNetLog.Info(LogMessage("TODO: Implement client side message handling"));

}

void NetSecureClientDriver::ProcessMessageAck(const ByteT* bytes, SizeT numBytes)
{
    PacketSerializer ps;
    Assert(ps.SetBuffer(bytes, numBytes));

    Crypto::HMACBuffer hmac;
    if (!ps.ComputeHeaderHmac(&mDerivedHMAC, hmac) || hmac != ps.GetEncryptedHMAC())
    {
        AtomicIncrement64(&mStats.mDroppedPackets);
        return;
    }

    UInt64 id;
    SizeT dataSize = sizeof(id);
    if (!ps.GetData(reinterpret_cast<ByteT*>(&id), dataSize)) // todo: We can put this is in the header?
    {
        return;
    }

    ScopeRWSpinLockRead lock(mMessageMapLock);
    auto iter = mMessageMap.find(id);
    if (iter != mMessageMap.end() && iter->second)
    {
        // todo: Verify packet type.
        NetMessage* message = iter->second;
        message->SetState(NetMessage::Success);
    }
}

void NetSecureClientDriver::UpdateClientHello()
{
    Crypto::AESKey oneTimeKey;
    Crypto::AESIV  oneTimeSalt;

    // Key generation
    if (!oneTimeKey.Generate(Crypto::AES_KEY_256))
    {
        gNetLog.Error(LogMessage("ClientHello failed to generate the one time key."));
        SetState(Failed);
        return;
    }
    Crypto::SecureRandomBytes(oneTimeSalt.mBytes, sizeof(oneTimeSalt.mBytes));

    if (!mHandshakeData)
    {
        mHandshakeData = TStrongPointer<HandshakeData>(LFNew<HandshakeData>());
        CriticalAssert(mHandshakeData);
    }

    if (!mHandshakeData->mClientHandshakeKey.Generate()
        || !mHandshakeData->mClientHandshakeHMAC.Generate()
        || !mClientSigningKey.GeneratePair(Crypto::RSA_KEY_2048))
    {
        gNetLog.Error(LogMessage("ClientHello failed to generate the necessary handshake keys."));
        SetState(Failed);
        return;
    }

    // Packet Data Serialization
    ByteT packetData[sizeof(ClientHelloPacketData::mBytes)] = { 0 };
    ByteT* encoded = packetData;
    SizeT encodedLength = sizeof(packetData);

    if (!GenerateClientHelloRSA(encoded, encodedLength, oneTimeKey, oneTimeSalt))
    {
        gNetLog.Error(LogMessage("ClientHello failed to generate the HelloRSA message"));
        SetState(Failed);
        return;
    }

    encoded += encodedLength;
    encodedLength = sizeof(packetData) - SIGNATURE_KEY_SIZE;
    if (!GenerateClientHelloAES(encoded, encodedLength, oneTimeKey, oneTimeSalt))
    {
        gNetLog.Error(LogMessage("ClientHello failed to generate the HelloAES message"));
        SetState(Failed);
        return;
    }

    // Generate Packet Data
    mHandshakeData->mPacketConnectionMessage.mType = NetPacketType::NET_PACKET_TYPE_CLIENT_HELLO;
    mHandshakeData->mPacketConnectionMessage.mRetransmits = static_cast<UInt16>(mMaxRetransmit);
    mHandshakeData->mPacketConnectionMessage.mSender = IPEndPointAny();

    PacketSerializer ps;
    ps.SetBuffer(mHandshakeData->mPacketConnectionMessage.mBytes, sizeof(mHandshakeData->mPacketConnectionMessage.mBytes));

    ps.SetAppId(mAppID);
    ps.SetAppVersion(mAppVersion);
    ps.SetFlags(0);
    ps.SetType(static_cast<UInt8>(mHandshakeData->mPacketConnectionMessage.mType));
    ps.SetPacketUID(GetPacketUID());
    ps.SetSessionID(SessionID());
    ps.SetIV(Crypto::AESIV());
    ps.SetEncryptedHMAC(Crypto::HMACBuffer());

    SizeT actualDataSize = encodedLength + SIGNATURE_KEY_SIZE;
    if (!ps.SetData(packetData, actualDataSize))
    {
        SetState(Failed);
        return;
    }
    ps.SetCrc32(ps.CalcCrc32());

    SizeT numBytes = ps.GetPacketSize();
    mHandshakeData->mPacketConnectionMessage.mSize = static_cast<UInt16>(numBytes);
    --mHandshakeData->mPacketConnectionMessage.mRetransmits;
    AtomicStore(&mWaitingHandshake, 1);
    if (!mSocket.SendTo(mHandshakeData->mPacketConnectionMessage.mBytes, numBytes, mEndPoint) || numBytes != ps.GetPacketSize())
    {
        SetState(Failed);
        return;
    }
    AtomicIncrement64(&mStats.mPacketsSent);
    AtomicAdd64(&mStats.mBytesSent, numBytes);

    SetState(ClientWaitServerHello);
    mHeartbeatTimer.Start();
    mHeartbeatWait = false;
    

    SetRunning(true);
    mThread.Fork([](void* param) {
        reinterpret_cast<NetSecureClientDriver*>(param)->ProcessBackground();
    }, this);
    mThread.SetDebugName("NetClient_Background");
}
void NetSecureClientDriver::UpdateWaitServerHello()
{
    if (mHeartbeatTimer.PeekDelta() < mAckTimeout)
    {
        return;
    }

    ScopeLock lock(mHandshakeLock);
    if (!mHandshakeData)
    {
        return;
    }
    if (mHandshakeData->mPacketConnectionMessage.mRetransmits == 0)
    {
        SetState(Failed); //Timed out
        return;
    }

    if (mHandshakeData->mPacketConnectionMessage.mRetransmits > 0)
    {
        SizeT numBytes = mHandshakeData->mPacketConnectionMessage.mSize;
        if (!mSocket.SendTo(mHandshakeData->mPacketConnectionMessage.mBytes, numBytes, mEndPoint) || numBytes != mHandshakeData->mPacketConnectionMessage.mSize)
        {
            SetState(Failed);
            return;
        }
        AtomicIncrement64(&mStats.mPacketsSent);
        AtomicAdd64(&mStats.mBytesSent, numBytes);
        mHeartbeatTimer.Start();
        --mHandshakeData->mPacketConnectionMessage.mRetransmits;
    }
}

void NetSecureClientDriver::UpdateClientReady()
{
    // Only send heartbeats if we're not waiting on one.
    if (!mHeartbeatWait && mHeartbeatTimer.PeekDelta() > mHeartbeatDelta)
    {
        gSysLog.Info(LogMessage("Client: Send heartbeat"));

        Crypto::AESIV iv;
        Crypto::SecureRandomBytes(iv.mBytes, sizeof(iv.mBytes));

        TPacketData<256> packet;
        packet.mType = NetPacketType::NET_PACKET_TYPE_HEARTBEAT;

        PacketSerializer ps;
        ps.SetBuffer(packet.mBytes, sizeof(packet.mBytes));

        ps.SetAppId(mAppID);
        ps.SetAppVersion(mAppVersion);
        ps.SetFlags(0);
        ps.SetType(static_cast<UInt8>(packet.mType));
        ps.SetPacketUID(GetPacketUID());
        ps.SetSessionID(mSessionID);
        ps.SetIV(iv);
        
        Crypto::HMACBuffer hmac;
        if (!ps.ComputeHeaderHmac(&mDerivedHMAC, hmac))
        {
            SetState(Failed);
            return;
        }
        ps.SetEncryptedHMAC(hmac);
        ps.SetCrc32(ps.CalcCrc32());

        // note: We don't retransmit heartbeats since we send them on timer
        SizeT numBytes = ps.GetPacketSize();
        if (!mSocket.SendTo(packet.mBytes, numBytes, mEndPoint) || numBytes != ps.GetPacketSize())
        {
            SetState(Failed);
            return;
        }
        AtomicIncrement64(&mStats.mPacketsSent);
        AtomicAdd64(&mStats.mBytesSent, numBytes);
        mHeartbeatWait = true;
    }

    UpdateMessages();
}

void NetSecureClientDriver::UpdateMessages()
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

void NetSecureClientDriver::UpdateMessage(NetMessage* message)
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

void NetSecureClientDriver::UpdateMessageSerialize(NetMessage* message)
{
    NetKeySet keySet;
    keySet.mDerivedSecretKey = &mDerivedSecretKey;
    keySet.mHmacKey = &mDerivedHMAC;
    keySet.mSigningKey = &mClientSigningKey;
    keySet.mVerifyKey = nullptr; // Shouldn't be needed for this op

    NetServerDriverConfig config;
    config.mAppID = mAppID;
    config.mAppVersion = mAppVersion;
    config.mMaxRetransmit = mMaxRetransmit;

    if (!message->Serialize(GetPacketUID(), keySet, config))
    {
        message->SetState(NetMessage::Failed);
        return;
    }
    message->SetState(NetMessage::Register);
}
void NetSecureClientDriver::UpdateMessageTransmit(NetMessage* message)
{
    if (!message->GetConnection())
    {
        message->SetState(NetMessage::Failed);
        return;
    }

    if (!message->HasTransmitStarted() || (message->GetTransmitRemaining() > 0 && message->GetTransmitDelta() > mAckTimeout))
    {
        IPEndPointAny endPoint = message->GetConnection()->GetEndPoint();
        SizeT numBytes = message->GetPacketBytesSize();
        if (!mSocket.SendTo(message->GetPacketBytes(), numBytes, endPoint) || numBytes != message->GetPacketBytesSize())
        {
            message->SetState(NetMessage::Failed);
            return;
        }
        message->OnTransmit();
    }
}
void NetSecureClientDriver::UpdateMessageFinal(NetMessage* message)
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


bool NetSecureClientDriver::GenerateClientHelloRSA(ByteT* encoded, SizeT& encodedSize, Crypto::AESKey& key, Crypto::AESIV& iv)
{
    NetOneTimeKeyMsg msg;
    msg.mOneTimeKey = Crypto::AES256KeySerialized(&key);
    msg.mOneTimeIV = Crypto::AESIVSerialized(&iv);

    ByteT plainText[SIGNATURE_KEY_SIZE] = { 0 };
    SizeT plainTextLength = sizeof(plainText);

    if (!NetSerialization::WriteAllBytes(plainText, plainTextLength, msg))
    {
        return false;
    }

    if (!Crypto::RSAEncryptPublic(&mServerCertificateKey, plainText, plainTextLength, encoded, encodedSize))
    {
        return false;
    }
    return true;
}
bool NetSecureClientDriver::GenerateClientHelloAES(ByteT* encoded, SizeT& encodedSize, Crypto::AESKey& key, Crypto::AESIV& iv)
{
    NetClientHelloMsg msg;
    msg.mClientHandshakeKey = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mClientHandshakeKey);
    msg.mClientHandshakeHmac = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mClientHandshakeHMAC);
    msg.mClientSigningKey = Crypto::RSA2048PublicKeySerialized(&mClientSigningKey);

    ByteT plainText[sizeof(ServerHelloPacketData::mBytes) - SIGNATURE_KEY_SIZE];
    SizeT plainTextLength = sizeof(plainText);

    if (!NetSerialization::WriteAllBytes(plainText, plainTextLength, msg))
    {
        return false;
    }

    if (!Crypto::AESEncrypt(&key, iv.mBytes, plainText, plainTextLength, encoded, encodedSize))
    {
        return false;
    }

    return true;
}

bool NetSecureClientDriver::ReadServerHelloRSA(const ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv)
{
    ByteT plainText[SIGNATURE_KEY_SIZE] = { 0 };
    SizeT plainTextLength = sizeof(plainText);

    if (!Crypto::RSADecryptPrivate(&mClientSigningKey, encoded, encodedSize, plainText, plainTextLength))
    {
        return false;
    }

    NetServerHelloRSAMsg msg;
    msg.mIV = Crypto::AESIVSerialized(&iv);
    msg.mServerHandshakeKey = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mServerHandshakeKey);

    if (!NetSerialization::ReadAllBytes(plainText, plainTextLength, msg))
    {
        return false;
    }

    ByteT scratch[32] = { 0 };
    if (!Crypto::ECDHDerive(&mHandshakeData->mClientHandshakeKey, &mHandshakeData->mServerHandshakeKey, scratch, sizeof(scratch))
        || !mDerivedSecretKey.Load(Crypto::AES_KEY_256, scratch))
    {
        return false;
    }
    
    return true;
}

bool NetSecureClientDriver::ReadServerHelloAES(const ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv)
{
    Assert(mDerivedSecretKey.GetKeySize() != Crypto::AES_KEY_Unknown);
    ByteT plainText[sizeof(ServerHelloPacketData::mBytes) - SIGNATURE_KEY_SIZE] = { 0 };
    SizeT plainTextLength = sizeof(plainText);

    if (!Crypto::AESDecrypt(&mDerivedSecretKey, iv.mBytes, encoded, encodedSize, plainText, plainTextLength))
    {
        return false;
    }

    NetServerHelloMsg msg;
    msg.mServerHandshakeHMAC = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mServerHandshakeHMAC);
    msg.mServerSigningKey = Crypto::RSA2048PublicKeySerialized(&mServerSigningKey);
    msg.mSessionID = SessionIDSerialized(&mSessionID);

    if (!NetSerialization::ReadAllBytes(plainText, plainTextLength, msg))
    {
        return false;
    }

    ByteT scratch[32] = { 0 };

    if (!Crypto::ECDHDerive(&mHandshakeData->mClientHandshakeHMAC, &mHandshakeData->mServerHandshakeHMAC, scratch, sizeof(scratch))
        || !mDerivedHMAC.Load(scratch, sizeof(scratch)))
    {
        return false;
    }
    return true;
}

void NetSecureClientDriver::SendAck(const ByteT* bytes, SizeT numBytes, const ByteT* data, SizeT dataLength)
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
        gNetLog.Error(LogMessage("Failed to send ack to server. Packet data could not be set."));
        SetState(Failed);
        return;
    }

    if (IsConnected())
    {
        Crypto::AESIV iv;
        Crypto::HMACBuffer hmac;
        Crypto::SecureRandomBytes(iv.mBytes, sizeof(iv.mBytes));
        ack.SetIV(iv);
        if (!ack.ComputeHeaderHmac(&mDerivedHMAC, hmac))
        {
            SetState(Failed);
            return;
        }
        ack.SetEncryptedHMAC(hmac);
    }
    else
    {
        ack.SetIV(Crypto::AESIV());
        ack.SetEncryptedHMAC(Crypto::HMACBuffer()); // todo: Someone can forge the Ack's giving the client/server a false positive
    }
    ack.SetCrc32(ack.CalcCrc32());

    // note: We don't retransmit acknowledgements
    SizeT ackSize = ack.GetPacketSize();
    if (!mSocket.SendTo(ackBytes, ackSize, mEndPoint) || ackSize != ack.GetPacketSize())
    {
        SetState(Failed);
        return;
    }
    AtomicIncrement64(&mStats.mPacketsSent);
    AtomicAdd64(&mStats.mBytesSent, ackSize);
}

} // namespace lf
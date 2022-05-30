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
#pragma once
#include "Runtime/Net/NetDriver.h"

#include "Core/Crypto/ECDH.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Net/UDPSocket.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Platform/Thread.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/Time.h"

#include "Runtime/Net/Client/NetSecureLocalClientConnection.h"

namespace lf {

struct LoggerMessage;
class NetMessage;
class NetMessageController;

class LF_RUNTIME_API NetSecureClientDriver : public NetDriver
{
public:
    using Super = NetDriver;
    using PacketFilter = TCallback<bool, const ByteT*, SizeT, const IPEndPointAny&>;

    NetSecureClientDriver();
    ~NetSecureClientDriver();
    bool Initialize(UInt16 appId, UInt16 appVersion, const IPEndPointAny& endPoint, const Crypto::RSAKey& serverCertificate);
    bool Initialize(const NetClientDriverConfig& config);
    void Shutdown();

    void Update();

    void SetMessageController(MessageType messageType, NetMessageController* controller) override;

    bool Send(
        MessageType messageType,
        Options options,
        const ByteT* bytes,
        const SizeT numBytes,
        OnSendSuccess onSuccess,
        OnSendFailed onFailed) override;

    bool Send(
        MessageType messageType,
        Options options,
        const ByteT* bytes,
        const SizeT numBytes,
        NetConnection* connection,
        OnSendSuccess onSuccess,
        OnSendFailed onFailed) override;

    bool IsServer() const override;
    bool IsClient() const override;

    void ProcessBackground();
    // Init: ServerCertificate, IP Address, Port
    // **********************************
    // Returns true if the client server background thread is running. This should be true after the 
    // internal network resources have been allocated/initialized and we've sent our first Client Hello
    // message.
    // **********************************
    bool IsRunning() const;
    // **********************************
    // Returns true once we've made a secure connection and have not disconnected or failed
    // **********************************
    bool IsConnected() const;
    // **********************************
    // Returns true if the connection has been disconnected.
    // **********************************
    bool IsDisconnected() const;
    // **********************************
    // Returns true if the driver expierenced a failure. (Unexpected message protocol/internal error)
    //
    // todo: We will want to provide more details on 'what' could've failed.
    // **********************************
    bool IsFailed() const;

    void ProcessPacketData(const ByteT* bytes, const SizeT numBytes, const IPEndPointAny& endPoint);
    void SetPacketFilter(const PacketFilter& filter) { mPacketFilter = filter; }

    // **********************************
    // Sets the 'Heartbeat Delta' variable, which is the time in seconds the client will emit a 
    // heartbeat message automatically to keep the connection between client/server alive.
    // 
    // note: Any other form of communication between the client/server may reset the heartbeat timer
    // so if there is a lot of network traffic between the 2 then there may be no heartbeats sent
    // back and forth.
    // **********************************
    void SetHeartbeatDelta(Float32 delta) { mHeartbeatDelta = delta; }
    Float32 GetHeartbeatDelta() const { return mHeartbeatDelta; }

    // **********************************
    // Sets the 'Timeout' variable, which is the time in seconds before the connection
    // is closed on the client after not receiving any legitimate network traffic from
    // the server.
    // **********************************
    void SetTimeout(Float32 timeout) { mMaxHeartbeatDelta = timeout; }
    Float32 GetTimeout() const { return mMaxHeartbeatDelta; }
    
    // **********************************
    // Sets the 'Ack Timeout' variable, which is the time in seconds before a message
    // can be retransmitted without receiving it's acknowledgement.
    // **********************************
    void SetAckTimeout(Float32 seconds) { mAckTimeout = seconds; }
    Float32 GetAckTimeout() const { return mAckTimeout; }

    const SessionID& GetSessionID() const { return mSessionID; }

    SizeT GetDroppedPackets() const;
    SizeT GetPacketsSent() const;
    SizeT GetBytesSent() const;
    SizeT GetPacketsReceived() const;
    SizeT GetBytesReceived() const;
    SizeT GetRetransmits() const;
    void LogStats(LoggerMessage& msg) const;
private:
    enum State
    {
        InitNetwork,
        ClientHello,
        ClientWaitServerHello,
        ClientReady,
        Disconnected,

        Failed
    };

    using LocalConnectionPtr = TAtomicStrongPointer<NetSecureLocalClientConnection>;

    using MessageID = UInt64;
    using MessagePtr = TAtomicStrongPointer<NetMessage>;
    using MessageMap = TMap<MessageID, MessagePtr>;

    void SetRunning(bool value);
    void SetState(State value);
    State GetState() const;
    UInt32 GetPacketUID();
    bool IsHandshakeComplete() const;

    void OnClientHelloAck(const ByteT* bytes, SizeT numBytes);

    void OnServerHello(const ByteT* bytes, SizeT numBytes);
    void OnServerHeartbeat(const ByteT* bytes, SizeT numBytes);

    void OnResponse(const ByteT* bytes, SizeT numBytes);
    void OnRequest(const ByteT* bytes, SizeT numBytes);
    void OnMessage(const ByteT* bytes, SizeT numBytes);
    void OnResponseAck(const ByteT* bytes, SizeT numBytes);
    void OnRequestAck(const ByteT* bytes, SizeT numBytes);
    void OnMessageAck(const ByteT* bytes, SizeT numBytes);

    void ProcessMessage(const ByteT* bytes, SizeT numBytes, NetMessageController* controller);
    void ProcessMessageAck(const ByteT* bytes, SizeT numBytes);

    void UpdateInitNetwork();
    void UpdateClientHello();
    void UpdateWaitServerHello();
    void UpdateClientReady();

    void UpdateMessages();
    void UpdateMessage(NetMessage* message);
    void UpdateMessageSerialize(NetMessage* message);
    void UpdateMessageTransmit(NetMessage* message);
    void UpdateMessageFinal(NetMessage* message);

    bool GenerateClientHelloRSA(ByteT* encoded, SizeT& encodedSize, Crypto::AESKey& key, Crypto::AESIV& iv);
    bool GenerateClientHelloAES(ByteT* encoded, SizeT& encodedSize, Crypto::AESKey& key, Crypto::AESIV& iv);
    bool ReadServerHelloRSA(const ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv);
    bool ReadServerHelloAES(const ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv);

    void SendAck(const ByteT* bytes, SizeT numBytes, const ByteT* data, SizeT dataLength);


    // ********************************** 
    // Client Configuration
    // ********************************** 
    Crypto::RSAKey  mServerCertificateKey;
    // ** The ip/port of the server the client tries to connect to
    IPEndPointAny   mEndPoint;
    // ** 
    UInt16          mAppID;
    // **
    UInt16          mAppVersion;
    // ** The time in seconds until a message is retransmitted.
    Float32         mAckTimeout;
    // ** The maximum number of times a message will be retransmitted
    SizeT           mMaxRetransmit;
    // ** The time in seconds until a heartbeat is sent out again. (note: Heartbeat timer will refresh upon any communication with the server)
    Float32         mHeartbeatDelta;
    // ** The maximum time allowed until the client is considered 'disconnected'.
    Float32         mMaxHeartbeatDelta;

    struct HandshakeData
    {
        // ** The clients private|public handshake key used to derive the 'Shared Secret'
        Crypto::ECDHKey mClientHandshakeKey;
        // ** The clients private|public handshake hmac used to derive the 'Shared Hmac'
        Crypto::ECDHKey mClientHandshakeHMAC;
        // ** The server public handshake key used to derive the 'Shared Secret'
        Crypto::ECDHKey mServerHandshakeKey;
        // ** The server public handshake key used to derive the 'Shared Hmac'
        Crypto::ECDHKey mServerHandshakeHMAC;
        // ** The cached ClientHello packet, this is retransmitted if the ack is not received.
        ClientHelloPacketData mPacketConnectionMessage;
    };

    Crypto::AESKey  mDerivedSecretKey;
    Crypto::HMACKey mDerivedHMAC;
    Crypto::RSAKey  mClientSigningKey;
    Crypto::RSAKey  mServerSigningKey;
    SessionID       mSessionID;
    LocalConnectionPtr mLocalConnection;
    NetProtocol::Value mProtocol;

    // ********************************** 
    // Internal Client Resources
    // ********************************** 
    UDPSocket mSocket;
    volatile Atomic32  mState;
    volatile Atomic32  mRunning;
    volatile Atomic32  mPacketUID;
    Thread    mThread;
    
    // ********************************** 
    // Message Processing
    // ********************************** 
    RWSpinLock         mMessageControllerLocks[MessageType::MAX_VALUE];
    TStrongPointer<NetMessageController> mMessageControllers[MessageType::MAX_VALUE];
    mutable RWSpinLock mMessageMapLock;
    MessageMap         mMessageMap;
    SpinLock           mNewMessagesLock;
    TVector<MessagePtr> mNewMessages;
    TVector<MessagePtr> mMessages;

    // ********************************** 
    // Heartbeat Processing
    // ********************************** 
    Timer   mHeartbeatTimer;
    bool    mHeartbeatWait;

    SpinLock     mPacketProcessLock;
    PacketFilter mPacketFilter;
    
    SpinLock                      mHandshakeLock;
    TStrongPointer<HandshakeData> mHandshakeData;
    volatile Atomic32             mWaitingHandshake;

    // Stats:
    struct Stats
    {
        Stats()
        : mDroppedPackets(0)
        , mPacketsSent(0)
        , mBytesSent(0)
        , mPacketsReceived(0)
        , mBytesReceived(0)
        , mRetransmits(0)
        {}

        // The number of received packets that we're dropped
        volatile Atomic64 mDroppedPackets;
        // The number of packets we sent
        volatile Atomic64 mPacketsSent;
        // The number of bytes from the packets we sent
        volatile Atomic64 mBytesSent;
        // The number of packets received
        volatile Atomic64 mPacketsReceived;
        // The number of bytes received
        volatile Atomic64 mBytesReceived;
        // The number of packets we had to retransmit.
        volatile Atomic64 mRetransmits;
    };
    Stats mStats;
};

} // namespace lf
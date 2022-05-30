#pragma once
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

#include "Runtime/Net/NetDriver.h"

#include "Core/Crypto/ECDH.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Platform/RWSpinLock.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Memory/AtomicSmartPointer.h"
#include "Core/Net/UDPSocket.h"
#include "Core/Utility/Array.h"
#include "Core/Utility/SmartCallback.h"
#include "Core/Utility/StdMap.h"
#include "Core/Utility/Time.h"

#include "Runtime/Net/NetTransmit.h"
#include "Runtime/Net/Server/NetSecureServerConnection.h"

namespace lf {

struct LoggerMessage;
class NetMessage;
class NetMessageController;

class LF_RUNTIME_API NetSecureServerDriver : public NetDriver
{
public:
    using Super = NetDriver;
    using PacketFilter = TCallback<bool, const ByteT*, SizeT, const IPEndPointAny&>;

    NetSecureServerDriver();
    ~NetSecureServerDriver();
    bool Initialize(const NetServerDriverConfig& config);
    void Shutdown();
    void Update();

    void SetMessageController(MessageType messageType, NetMessageController* controller) override;

    bool Send(
        MessageType message,
        Options options,
        const ByteT* bytes,
        const SizeT numBytes,
        OnSendSuccess onSuccess,
        OnSendFailed onFailed) override;

    bool Send(
        MessageType message,
        Options options,
        const ByteT* bytes,
        const SizeT numBytes,
        NetConnection* connection,
        OnSendSuccess onSuccess,
        OnSendFailed onFailed) override;

    bool IsServer() const override;
    bool IsClient() const override;
    
    void ProcessBackground();

    bool IsRunning() const;

    void ProcessPacketData(const ByteT* bytes, const SizeT numBytes, const IPEndPointAny& endPoint);
    void SetPacketFilter(const PacketFilter& filter) { mPacketFilter = filter; }

    void SetTimeout(Float32 seconds) { mMaxHeartbeatDelta = seconds; }
    Float32 GetTimeout() const { return mMaxHeartbeatDelta; }
    SizeT GetConnectionCount() const { return mConnections.size(); } // todo: Use atomic
    NetConnectionAtomicPtr FindConnection(const SessionID& sessionID) const;

    SizeT GetDroppedPackets() const;
    SizeT GetPacketsSent() const;
    SizeT GetBytesSent() const;
    SizeT GetPacketsReceived() const;
    SizeT GetBytesReceived() const;
    SizeT GetRetransmits() const;
    SizeT GetConnectionsAccepted() const;
    SizeT GetDroppedDuplicatePackets() const;
    void LogStats(LoggerMessage& msg) const;
private:
    using ConnectionState = NetSecureServerConnection::State;
    using ConnectionType = NetSecureServerConnection;
    using ConnectionPtr = NetSecureServerConnectionAtomicPtr;
    using ConnectionMap = TMap<SessionID, ConnectionPtr>;

    using MessageID = UInt64;
    using MessagePtr = TAtomicStrongPointer<NetMessage>;
    using MessageMap = TMap<MessageID, MessagePtr>;
    
    void SetRunning(bool value);

    void CreateSessionFromBytes(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint);
    void AcceptConnection(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint);
    void OnHeartbeat(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint);
    void OnServerHelloAck(const ByteT* bytes, SizeT bytesSize, const IPEndPointAny& endPoint);

    void OnResponse(const ByteT* bytes, SizeT numBytes);
    void OnRequest(const ByteT* bytes, SizeT numBytes);
    void OnMessage(const ByteT* bytes, SizeT numBytes);
    void OnResponseAck(const ByteT* bytes, SizeT numBytes);
    void OnRequestAck(const ByteT* bytes, SizeT numBytes);
    void OnMessageAck(const ByteT* bytes, SizeT numBytes);

    void ProcessMessage(const ByteT* bytes, SizeT numBytes, NetMessageController* controller);
    void ProcessMessageAck(const ByteT* bytes, SizeT numBytes);

    void UpdateConnections();
    void UpdateConnection(ConnectionType* connection);
    void UpdateNetworkInit(ConnectionType* connection);
    void UpdateServerHello(ConnectionType* connection);
    void UpdateServerReady(ConnectionType* connection);

    void UpdateMessages();
    void UpdateMessage(NetMessage* message);
    void UpdateMessageSerialize(NetMessage* message);
    void UpdateMessageTransmit(NetMessage* message);
    void UpdateMessageFinal(NetMessage* message);

    void SendAck(const ByteT* bytes, SizeT numBytes, ConnectionType* connection, const ByteT* data, SizeT dataLength);
    void SendAck(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint);

    // ********************************** 
    // Server Configuration
    // ********************************** 
    Crypto::RSAKey mCertificateKey; // Config on init

    UInt16 mAppID;       // Config on init
    UInt16 mAppVersion;  // Config on init
    Float32   mMaxHeartbeatDelta; // Config at runtime
    Float32   mAckTimeout;        // Config at runtime
    SizeT     mMaxRetransmit;     // Config at runtime

    // ********************************** 
    // Internal Server Resources
    // ********************************** 

    // ** The socket we use to receive all incoming traffic
    UDPSocket mSocket;

    // ** Variable that controls whether or not the background receiver thread runs.
    volatile Atomic32  mRunning;
    // ** The background receiver thread
    Thread    mThread;
    // ** A scheduler that dispatches packet processing tasks
    TaskScheduler mProcessPacketDispatcher;
    // ** A scheduler that dispatches tasks on the 'main' thread (when update is called)
    TaskScheduler mMainThreadDispatcher;
    
    
    // ********************************** 
    // Connection Control
    // ********************************** 

    // ** We use the primary connection map for 'connection allocation'
    // ** [ Lock on CreateSessionFromBytes(WT) | Garbage Collect(MT) ]
    mutable SpinLock mPrimaryConnectionMapLock;
    ConnectionMap    mPrimaryConnectionMap;
    // ** We use the connection map for querying 'connection id => connection'
    // ** [ READ - on any session query during packet processing ]
    // ** [ WRITE - New Connection(MT) | Garbage Collection(MT)]
    mutable RWSpinLock mConnectionMapLock;
    ConnectionMap      mConnectionMap;
    // ** We use 'new connection' list to distribute new connections to the 'connection map'
    SpinLock              mNewConnectionsLock;
    TVector<ConnectionPtr> mNewConnections;
    // ** We use the list of connections to just update. Only accessed on one thread
    TVector<ConnectionPtr> mConnections;

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

    NetTransmitBuffer mClientHelloTransmitBuffer;
    PacketFilter mPacketFilter;
    SpinLock     mPacketProcessLock;

    struct Stats
    {
        Stats()
            : mDroppedPackets(0)
            , mPacketsSent(0)
            , mBytesSent(0)
            , mPacketsReceived(0)
            , mBytesReceived(0)
            , mRetransmits(0)
            , mConnectionsAccepted(0)
            , mDroppedDuplicatePackets(0)
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
        // The number of connections we've accepted.
        volatile Atomic64 mConnectionsAccepted;
        // The number of packets that we're detected as a 'dupe' that we're dropped
        volatile Atomic64 mDroppedDuplicatePackets;
    };
    Stats mStats;
};

} // namespace lf
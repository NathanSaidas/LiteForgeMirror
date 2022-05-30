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

#include "Runtime/Net/NetConnection.h"
#include "Core/Crypto/ECDH.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Crypto/RSA.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Platform/SpinLock.h"
#include "Core/Utility/Time.h"
#include "Runtime/Net/NetTransmit.h"

namespace lf
{

// ********************************** 
// This is the implementation of the NetConnection but for the NetSecureServerDriver
//
// Order of operations:
// Initialize(...)
// SerializeClientHandshakeData(...)
// GenerateServerHandshakeData(...)
// GenerateServerHelloPacket(...)
// ********************************** 
class LF_RUNTIME_API NetSecureServerConnection : public NetConnection
{
public:
    using Super = NetConnection;

    enum State
    {
        NetworkInit,
        ServerHello,
        ServerReady,
        Disconnected,

        Failed
    };

    NetSecureServerConnection();

    String GetConnectionName() const;
    const SessionID& GetConnectionID() const override;
    const IPEndPointAny& GetEndPoint() const override;
    void OnHeartbeatTick();

    void SetState(State state);
    State GetState() const;
    PacketUID GetPacketUID();
    Float64 GetHeartbeatDelta() const { return mHeartbeatTimer.PeekDelta(); }
    bool IsHandshakeComplete() const;

    // ********************************** 
    // Initializes some basic information for the NetConnection
    // 
    // note: This method may not be called on multiple threads at once.
    // note: This method should only be called during NetworkInit state
    // ********************************** 
    bool Initialize(const SessionID& connectionID, Crypto::RSAKey* serverCertificate, const IPEndPointAny& endPoint);
    // ********************************** 
    // Serializes client handshake information from the given bytes.
    // 
    // note: This method may not be called on multiple threads at once.
    // note: This method should only be called during NetworkInit state
    // ********************************** 
    bool SerializeClientHandshakeData(const ByteT* bytes, SizeT numBytes);
    // ********************************** 
    // Generate the server handshake keys
    // 
    // note: This method may not be called on multiple threads at once.
    // note: This method should only be called during ServerHello state
    // ********************************** 
    bool GenerateServerHandshakeData();
    // ********************************** 
    // Generate the server handshake packet data
    // 
    // note: This method may not be called on multiple threads at once.
    // note: This method should only be called during ServerHello state
    // ********************************** 
    bool GenerateServerHelloPacket(const NetServerDriverConfig& config);

    void WaitHandshake();

    LF_INLINE Crypto::RSAKey& GetClientSigningKey() { return mClientSigningKey; }
    LF_INLINE const Crypto::RSAKey& GetClientSigningKey() const { return mClientSigningKey; }
    LF_INLINE Crypto::RSAKey& GetServerSigningKey() { return mServerSigningKey; }
    LF_INLINE const Crypto::RSAKey& GetServerSigningKey() const { return mServerSigningKey; }
    LF_INLINE Crypto::AESKey& GetDerivedSecretKey() { return mDerivedSecretKey; }
    LF_INLINE const Crypto::AESKey& GetDerivedSecretKey() const { return mDerivedSecretKey; }
    LF_INLINE Crypto::HMACKey& GetDerivedHMAC() { return mDerivedHMAC; }
    LF_INLINE const Crypto::HMACKey& GetDerivedHMAC() const { return mDerivedHMAC; }
    LF_INLINE NetTransmitBuffer& GetTransmitBuffer(NetPacketType::Value packetType) { return mTransmitBuffers[packetType]; }

    // Handshake Data -- This data is not thread safe, acquire and release the lock accordingly
    // note: You CANNOT update the heartbeat tick while you have the handshake data lock acquired
    LF_INLINE SpinLock& GetHandshakeLock() { return mHandshakeLock; }
    LF_INLINE Crypto::ECDHKey* GetClientHandshakeKey() { return mHandshakeData ? &mHandshakeData->mClientHandshakeKey : nullptr; }
    LF_INLINE Crypto::ECDHKey* GetClientHandshakeHMAC() { return mHandshakeData ? &mHandshakeData->mClientHandshakeHMAC : nullptr; }
    LF_INLINE Crypto::ECDHKey* GetServerHandshakeKey() { return mHandshakeData ? &mHandshakeData->mServerHandshakeKey : nullptr; }
    LF_INLINE Crypto::ECDHKey* GetServerHandshakeHMAC() { return mHandshakeData ? &mHandshakeData->mServerHandshakeHMAC : nullptr; }
    LF_INLINE ServerHelloPacketData* GetServerHelloMsg() { return mHandshakeData ? &mHandshakeData->mServerHelloMsg : nullptr; }
private:

    // Data contained in the handshake is not persistent and will be released after we establish a connection
    struct HandshakeData
    {
        // ** The clients public handshake key used to derive the 'Shared Secret'
        Crypto::ECDHKey mClientHandshakeKey;
        // ** The clients public handshake hmac used to derive the 'Shared Hmac'
        Crypto::ECDHKey mClientHandshakeHMAC;
        // ** The server private|public handshake key used to derive the 'Shared Secret'
        Crypto::ECDHKey mServerHandshakeKey;
        // ** The server private|public handshake key used to derive the 'Shared Hmac'
        Crypto::ECDHKey mServerHandshakeHMAC;
        // ** The cached ServerHello packet, this is retransmitted if the ack is not received.
        ServerHelloPacketData mServerHelloMsg;
    };

    bool GenerateRSAPacketData(ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv);
    bool GenerateAESPacketData(ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv);

    // ** Clients public key used to verify messages
    Crypto::RSAKey  mClientSigningKey;
    // ** Server public|private key used to sign data.
    Crypto::RSAKey  mServerSigningKey;
    // ** The derived secret key used for data encryption
    Crypto::AESKey  mDerivedSecretKey;
    // ** The derived hmac used to sign headers and data.
    Crypto::HMACKey mDerivedHMAC;
    // ** Certificate used to decrypt ServerHello messages
    Crypto::RSAKey* mServerCertificate;
    // ** The ipaddress/port of the client
    IPEndPointAny   mEndPoint;
    // ** The id of the connection
    SessionID       mConnectionID;
    // ** A timer that ticks to indiciates the last received heartbeat.
    Timer           mHeartbeatTimer;
    // ** Transmit buffer (per packet type) that provide protection/resistance to duplicate packets
    NetTransmitBuffer mTransmitBuffers[NetPacketType::MAX_VALUE];

    volatile Atomic32 mState;
    volatile Atomic32 mPacketUID;

    // ** This is a lock we use only when completing/terminating the handshake
    SpinLock                      mHandshakeLock;
    TStrongPointer<HandshakeData> mHandshakeData;
    volatile Atomic32             mWaitingHandshake;

};
DECLARE_ATOMIC_PTR(NetSecureServerConnection);
DECLARE_ATOMIC_WPTR(NetSecureServerConnection);


}
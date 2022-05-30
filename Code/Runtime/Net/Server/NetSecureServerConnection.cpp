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
#include "NetSecureServerConnection.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/String/StringCommon.h"
#include "Core/Utility/Log.h"
#include "Runtime/Net/NetSerialization.h"
#include "Runtime/Net/PacketSerializer.h"

namespace lf {

// note: If we ever change the signature key size we should change this
static const SizeT SIGNATURE_KEY_SIZE = 256;

NetSecureServerConnection::NetSecureServerConnection()
: Super()
, mClientSigningKey()
, mServerSigningKey()
, mDerivedSecretKey()
, mDerivedHMAC()
, mServerCertificate(nullptr)
, mEndPoint()
, mConnectionID()
, mHeartbeatTimer()
, mTransmitBuffers()
, mState(0)
, mPacketUID(0)
, mHandshakeLock()
, mHandshakeData()
, mWaitingHandshake(1)
{}

String NetSecureServerConnection::GetConnectionName() const
{
    return BytesToHex(mConnectionID.Bytes(), mConnectionID.Size());
}
const SessionID& NetSecureServerConnection::GetConnectionID() const
{
    return mConnectionID;
}

const IPEndPointAny& NetSecureServerConnection::GetEndPoint() const
{
    return mEndPoint;
}

// We prepare this method to be called...
// [Worker Thread] Client Acks the SeverHello
// [Worker Thread] Client Sends Heartbeat while State == ServerReady
// [Worker Thread] Client Sends User Message while State == ServerReady
// [Main Thread|Frame Thread] Server terminates the connection (timeout/kick)
void NetSecureServerConnection::OnHeartbeatTick()
{
    if (!IsHandshakeComplete())
    {
        ScopeLock lock(mHandshakeLock);
        if (!IsHandshakeComplete())
        {
            if (mHandshakeData)
            {
                mHandshakeData.Release();
            }
            AtomicStore(&mWaitingHandshake, 0);
        }
    }
    mHeartbeatTimer.Start();
}

void NetSecureServerConnection::SetState(State state)
{
    const char* SESSION_STATE_STRINGS[]
    {
        "NetworkInit",
        "ServerHello",
        "ServerReady",
        "Disconnected",
        "Failed"
    };
    LF_STATIC_ASSERT(LF_ARRAY_SIZE(SESSION_STATE_STRINGS) == Failed + 1);
    gNetLog.Info(LogMessage("NetSecureServerDriver::SetState[ ") << SESSION_STATE_STRINGS[GetState()] << " -> " << SESSION_STATE_STRINGS[state] << "]");


    AtomicStore(&mState, state);
}
NetSecureServerConnection::State NetSecureServerConnection::GetState() const
{
    return static_cast<State>(AtomicLoad(&mState));
}
PacketUID NetSecureServerConnection::GetPacketUID()
{
    return AtomicIncrement32(&mPacketUID);
}
bool NetSecureServerConnection::IsHandshakeComplete() const
{
    return AtomicLoad(&mWaitingHandshake) == 0;
}


bool NetSecureServerConnection::Initialize(const SessionID& connectionID, Crypto::RSAKey* serverCertificate, const IPEndPointAny& endPoint)
{
    if (connectionID.Empty())
    {
        ReportBugMsg("Invalid argument 'connectionID'");
        return false;
    }

    if (serverCertificate == nullptr || !serverCertificate->HasPrivateKey() || serverCertificate->GetKeySize() != Crypto::RSA_KEY_2048)
    {
        ReportBugMsg("Invalid argument 'serverCertificate'");
        return false;
    }

    if (GetState() != NetworkInit)
    {
        ReportBugMsg("Invalid operation 'NetSecureServerConnection cannot be initialized after it's been NetworkInitialized.'");
        return false;
    }

    mConnectionID = connectionID;
    mServerCertificate = serverCertificate;
    mEndPoint = endPoint;
    return true;
}

bool NetSecureServerConnection::SerializeClientHandshakeData(const ByteT* bytes, SizeT numBytes)
{
    if (GetState() != NetworkInit)
    {
        ReportBugMsg("Invalid operation 'NetSecureServerConnection cannot serialize client handshake after it's been NetworkInitialized.");
        return false;
    }

    if (!mHandshakeData)
    {
        mHandshakeData = TStrongPointer<HandshakeData>(LFNew<HandshakeData>());
        CriticalAssert(mHandshakeData);
    }

    NetClientHelloMsg msg;
    msg.mClientHandshakeKey = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mClientHandshakeKey);
    msg.mClientHandshakeHmac = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mClientHandshakeHMAC);
    msg.mClientSigningKey = Crypto::RSA2048PublicKeySerialized(&mClientSigningKey);

    if (!NetSerialization::ReadAllBytes(bytes, numBytes, msg))
    {
        gNetLog.Warning(LogMessage("Failed to serialize the client handshake data."));
        return false;
    }

    if (msg.mClientHandshakeKey.mError || msg.mClientHandshakeHmac.mError || msg.mClientSigningKey.mError)
    {
        gNetLog.Warning(LogMessage("Failed to serialize the client handshake keys."));
        return false;
    }
    return true;
}

bool NetSecureServerConnection::GenerateServerHandshakeData()
{
    if (GetState() != ServerHello || !mHandshakeData)
    {
        ReportBugMsg("Invalid operation 'NetSecureServerConnection cannot generate server handshake data. Must be in the ServerHello state.");
        return false;
    }

    if (!mHandshakeData->mServerHandshakeKey.Generate()
        || !mHandshakeData->mServerHandshakeHMAC.Generate()
        || !mServerSigningKey.GeneratePair(Crypto::RSA_KEY_2048))
    {
        gNetLog.Error(LogMessage("GenerateServerHandshakeData failed to generate the necessary handshake keys."));
        return false;
    }

    ByteT scratch[32] = { 0 };
    if (!Crypto::ECDHDerive(&mHandshakeData->mServerHandshakeKey, &mHandshakeData->mClientHandshakeKey, scratch, sizeof(scratch))
        || !mDerivedSecretKey.Load(Crypto::AES_KEY_256, scratch)
        || !Crypto::ECDHDerive(&mHandshakeData->mServerHandshakeHMAC, &mHandshakeData->mClientHandshakeHMAC, scratch, sizeof(scratch))
        || !mDerivedHMAC.Load(scratch, sizeof(scratch)))
    {
        gNetLog.Error(LogMessage("GenerateServerHandshakeData failed to derive the shared secret."));
        return false;
    }

    return true;
}

bool NetSecureServerConnection::GenerateServerHelloPacket(const NetServerDriverConfig& config)
{
    ByteT encoded[sizeof(ServerHelloPacketData::mBytes)];
    SizeT encodedLength = sizeof(encoded);

    Crypto::AESIV iv;
    Crypto::SecureRandomBytes(iv.mBytes, sizeof(iv.mBytes));

    // Generate the RSA message
    ByteT* cursor = encoded;
    if (!GenerateRSAPacketData(cursor, encodedLength, iv))
    {
        return false;
    }
    Assert(SIGNATURE_KEY_SIZE == encodedLength);
    cursor += encodedLength;
    encodedLength = sizeof(encoded) - encodedLength;
    
    // Generate the AES message
    if (!GenerateAESPacketData(cursor, encodedLength, iv))
    {
        return false;
    }

    mHandshakeData->mServerHelloMsg.mType = NetPacketType::NET_PACKET_TYPE_SERVER_HELLO;
    mHandshakeData->mServerHelloMsg.mRetransmits = static_cast<UInt16>(config.mMaxRetransmit);
    memset(mHandshakeData->mServerHelloMsg.mBytes, 0, sizeof(mHandshakeData->mServerHelloMsg.mBytes));

    PacketSerializer ps;
    ps.SetBuffer(mHandshakeData->mServerHelloMsg.mBytes, sizeof(mHandshakeData->mServerHelloMsg.mBytes));

    ps.SetAppId(config.mAppID);
    ps.SetAppVersion(config.mAppVersion);
    ps.SetFlags(0);
    ps.SetType(static_cast<UInt8>(mHandshakeData->mServerHelloMsg.mType));
    ps.SetPacketUID(GetPacketUID());
    ps.SetSessionID(mConnectionID);
    ps.SetIV(iv);
    ps.SetEncryptedHMAC(Crypto::HMACBuffer());

    SizeT actualDataSize = encodedLength + SIGNATURE_KEY_SIZE;
    if (!ps.SetData(encoded, actualDataSize))
    {
        return false;
    }

    if (!ps.Sign(mServerCertificate))
    {
        return false;
    }
    ps.SetCrc32(ps.CalcCrc32());

    mHandshakeData->mServerHelloMsg.mSize = static_cast<UInt16>(ps.GetPacketSize());
    return true;
}

void NetSecureServerConnection::WaitHandshake()
{
    AtomicStore(&mWaitingHandshake, 1);
    mHeartbeatTimer.Start();
}


bool NetSecureServerConnection::GenerateRSAPacketData(ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv)
{
    CriticalAssert(mHandshakeData && GetState() == ServerHello);

    NetServerHelloRSAMsg msg;
    msg.mIV = Crypto::AESIVSerialized(&iv);
    msg.mServerHandshakeKey = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mServerHandshakeKey);

    ByteT plainText[SIGNATURE_KEY_SIZE] = { 0 };
    SizeT plainTextLength = sizeof(plainText);

    if (!NetSerialization::WriteAllBytes(plainText, plainTextLength, msg))
    {
        return false;
    }

    if (!Crypto::RSAEncryptPublic(&mClientSigningKey, plainText, plainTextLength, encoded, encodedSize))
    {
        return false;
    }

    return true;
}

bool NetSecureServerConnection::GenerateAESPacketData(ByteT* encoded, SizeT& encodedSize, Crypto::AESIV& iv)
{
    CriticalAssert(mHandshakeData && GetState() == ServerHello);
    NetServerHelloMsg msg;
    msg.mSessionID = SessionIDSerialized(&mConnectionID);
    msg.mServerHandshakeHMAC = Crypto::ECDHPublicKeySerialized(&mHandshakeData->mServerHandshakeHMAC);
    msg.mServerSigningKey = Crypto::RSA2048PublicKeySerialized(&mServerSigningKey);

    ByteT plainText[sizeof(ServerHelloPacketData::mBytes) - SIGNATURE_KEY_SIZE];
    SizeT plainTextLength = sizeof(plainText);

    if (!NetSerialization::WriteAllBytes(plainText, plainTextLength, msg))
    {
        return false;
    }

    if (!Crypto::AESEncrypt(&mDerivedSecretKey, iv.mBytes, plainText, plainTextLength, encoded, encodedSize))
    {
        return false;
    }

    return true;
}

}
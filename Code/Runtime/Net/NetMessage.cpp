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
#include "NetMessage.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Crypto/RSA.h"
#include "Core/Crypto/SecureRandom.h"
#include "Runtime/Net/PacketSerializer.h"
#include "Runtime/Net/NetConnection.h"
#include "Runtime/Net/NetTransmit.h"

namespace lf {

// note: If we ever change the signature key size we should change this
static const SizeT SIGNATURE_KEY_SIZE = 256;
// todo: This should be in NetTypes?
static const SizeT MAX_NET_MESSAGE_SIZE = 1500;

NetMessage::NetMessage()
: mState(SerializeData)
, mPacketData()
, mRetransmits()
, mRetransmitTimer()
, mID(0)
, mSuccessCallback()
, mFailureCallback()
, mMessageType(NetDriver::MESSAGE_GENERIC)
, mOptions(static_cast<NetDriver::Options>(0))
, mApplicationData()
, mConnection()
{
}

bool NetMessage::Initialize(NetDriver::MessageType messageType, NetDriver::Options options, const ByteT* bytes, SizeT numBytes)
{
    if (GetState() != SerializeData)
    {
        ReportBugMsg("Invalid operation, The NetMessage may have already been initialized, the state is not valid.");
        return false;
    }
    mMessageType = messageType;
    mOptions = options;
    mApplicationData.resize(numBytes);
    memcpy(mApplicationData.data(), bytes, numBytes);
    return true;
}

bool NetMessage::Serialize(UInt32 packetUID, const NetKeySet& keySet, const NetServerDriverConfig& config)
{
    if (GetState() != SerializeData || mApplicationData.empty())
    {
        return false;
    }

    // Verify Arguments:
    const bool encrypt = true; // todo: ((mOptions & NetDriver::OPTION_ENCRYPT) > 0);
    const bool signCompute = ((mOptions & NetDriver::OPTION_SIGNED) > 0);
    const bool hmacCompute = ((mOptions & NetDriver::OPTION_HMAC) > 0);
    if (encrypt && (!keySet.mDerivedSecretKey || keySet.mDerivedSecretKey->GetKeySize() == Crypto::AES_KEY_Unknown))
    {
        return false;
    }
    if (signCompute && (!keySet.mSigningKey || keySet.mSigningKey->GetKeySize() != Crypto::RSA_KEY_2048 || !keySet.mSigningKey->HasPrivateKey()))
    {
        return false;
    }
    if (hmacCompute && (!keySet.mHmacKey || keySet.mHmacKey->Empty()))
    {
        return false;
    }
    
    // Setup internals:
    NetConnectionAtomicPtr connectionLifeLock = mConnection;
    if (!connectionLifeLock)
    {
        return false;
    }
    SizeT estimateSize = GetSizeEstimate();
    mPacketData.resize(estimateSize);
    memset(mPacketData.data(), 0, estimateSize);
    Crypto::AESIV iv;
    Crypto::SecureRandomBytes(iv.mBytes, sizeof(iv.mBytes));

    // Encrypt the data
    ByteT cipherText[MAX_NET_MESSAGE_SIZE] = { 0 };
    SizeT cipherTextLength = sizeof(cipherText);
    ByteT* packetDataPtr = mApplicationData.data();
    SizeT packetDataSize = mApplicationData.size();
    if (encrypt)
    {
        if (Crypto::AESEncrypt(keySet.mDerivedSecretKey, iv.mBytes, mApplicationData.data(), mApplicationData.size(), cipherText, cipherTextLength))
        {
            packetDataPtr = cipherText;
            packetDataSize = cipherTextLength;
        }
        else
        {
            return false;
        }
    }

    // Serialize the packet
    PacketSerializer ps;
    if (!ps.SetBuffer(mPacketData.data(), mPacketData.size()))
    {
        return false;
    }
    ps.SetAppId(config.mAppID);
    ps.SetAppVersion(config.mAppVersion);
    ps.SetFlags(0);
    ps.SetType(static_cast<UInt8>(GetPacketType()));
    ps.SetPacketUID(packetUID);
    ps.SetSessionID(mConnection->GetConnectionID());
    ps.SetIV(iv);
    if (!ps.SetData(packetDataPtr, packetDataSize))
    {
        return false;
    }
    if (hmacCompute)
    {
        Crypto::HMACBuffer hmac;
        if (!keySet.mHmacKey->Compute(packetDataPtr, packetDataSize, hmac) || !ps.SetDataHMAC(hmac))
        {
            return false;
        }
    }
    if (signCompute)
    {
        if (!ps.Sign(keySet.mSigningKey))
        {
            return false;
        }
    }
    {
        Crypto::HMACBuffer hmac;
        if (!ps.ComputeHeaderHmac(keySet.mHmacKey, hmac))
        {
            return false;
        }
        ps.SetEncryptedHMAC(hmac);
    }
    ps.SetCrc32(ps.CalcCrc32());
    mPacketData.resize(ps.GetPacketSize());

    NetTransmitInfo transmitInfo(ps.GetPacketUID(), ps.GetCrc32());
    mID = transmitInfo.Value();
    mRetransmits = config.mMaxRetransmit;

    return true;
}

void NetMessage::OnTransmit()
{
    --mRetransmits;
    mRetransmitTimer.Start();
}

void NetMessage::OnSuccess()
{
    if (mSuccessCallback.IsValid())
    {
        mSuccessCallback.Invoke();
    }
}
void NetMessage::OnFailed()
{
    if (mFailureCallback.IsValid())
    {
        mFailureCallback.Invoke();
    }
}

void NetMessage::SetState(State value)
{
    AtomicStore(&mState, value);
}
NetMessage::State NetMessage::GetState() const
{
    return static_cast<State>(AtomicLoad(&mState));
}

SizeT NetMessage::GetSizeEstimate() const
{
    SizeT estimatedPacketSize = PacketSerializer::GetFullHeaderSize() + mApplicationData.size() + 16;
    if ((mOptions & NetDriver::OPTION_HMAC) > 0)
    {
        estimatedPacketSize += sizeof(Crypto::HMACBuffer);
    }
    if ((mOptions & NetDriver::OPTION_SIGNED) > 0)
    {
        estimatedPacketSize += SIGNATURE_KEY_SIZE;
    }
    return estimatedPacketSize;
}
NetPacketType::Value NetMessage::GetPacketType() const
{
    switch (mMessageType)
    {
        case NetDriver::MESSAGE_REQUEST: return NetPacketType::NET_PACKET_TYPE_REQUEST;
        case NetDriver::MESSAGE_RESPONSE: return NetPacketType::NET_PACKET_TYPE_RESPONSE;
        case NetDriver::MESSAGE_GENERIC: return NetPacketType::NET_PACKET_TYPE_MESSAGE;
        default:
            CriticalAssertMsg("Invalid message type!");
            break;
    }
    return NetPacketType::INVALID_ENUM;
}

} // namespace lf 
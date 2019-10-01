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
#include "PacketUtility.h"
#include "Core/Common/Assert.h"
#include "Core/Crypto/SecureRandom.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Crypto/RSA.h"
#include "Core/Utility/Crc32.h"
#include "Core/Utility/ErrorCore.h"

#include <utility>

namespace lf {


bool PacketUtility::IsConnected(const ByteT* packetBytes, SizeT packetBytesLength)
{
    if (packetBytesLength < PacketHeader::ACTUAL_SIZE)
    {
        return false;
    }

    // If this packet is secure then it will be a 'secure connected' not just 'connected'
    const PacketHeader* header = reinterpret_cast<const PacketHeader*>(packetBytes);
    if ((header->mFlags & (1 << NetPacketFlag::NET_PACKET_FLAG_SECURE)) > 0)
    {
        return false;
    }
    return header->mType != NetPacketType::NET_PACKET_TYPE_CONNECT 
        && header->mType != NetPacketType::NET_PACKET_TYPE_HEARTBEAT
        && header->mType != NetPacketType::NET_PACKET_TYPE_DISCONNECT;
}
bool PacketUtility::IsSecureConnected(const ByteT* packetBytes, SizeT packetBytesLength)
{
    if (packetBytesLength < PacketHeader::ACTUAL_SIZE)
    {
        return true;
    }
    const PacketHeader* header = reinterpret_cast<const PacketHeader*>(packetBytes);
    if ((header->mFlags & (1 << NetPacketFlag::NET_PACKET_FLAG_SECURE)) == 0)
    {
        return false;
    }
    return header->mType != NetPacketType::NET_PACKET_TYPE_CONNECT
        && header->mType != NetPacketType::NET_PACKET_TYPE_HEARTBEAT
        && header->mType != NetPacketType::NET_PACKET_TYPE_DISCONNECT;
}
NetPacketHeaderType::Value PacketUtility::GetHeaderType(const ByteT* packetBytes, SizeT packetBytesLength)
{
    if (IsConnected(packetBytes, packetBytesLength))
    {
        return NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED;
    }
    else if (IsSecureConnected(packetBytes, packetBytesLength))
    {
        return NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED;
    }
    return NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE;
}
bool PacketUtility::IsAck(const ByteT* packetBytes, SizeT packetBytesLength)
{
    CriticalAssertEx(packetBytesLength >= PacketHeader::ACTUAL_SIZE, LF_ERROR_INVALID_ARGUMENT, ERROR_API_CORE);
    return (reinterpret_cast<const PacketHeader*>(packetBytes)->mFlags & (1 << NetPacketFlag::NET_PACKET_FLAG_ACK)) > 0;
}
SizeT PacketUtility::GetHeaderSize(NetPacketHeaderType::Value headerType)
{
    switch (headerType)
    {
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE: return PacketHeader::ACTUAL_SIZE;
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED: return ConnectedPacketHeader::ACTUAL_SIZE;
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED: return SecureConnectedPacketHeader::ACTUAL_SIZE;
    }
    return 0;
}

SizeT PacketUtility::GetAckSize(NetPacketHeaderType::Value headerType)
{
    switch (headerType)
    {
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE: return AckPacketHeader::ACTUAL_SIZE;
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED: return AckConnectedPacketHeader::ACTUAL_SIZE;
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED: return AckSecureConnectedPacketHeader::ACTUAL_SIZE;
    }
    return 0;
}

static bool PrepareAckHeader(
    NetAckStatus::Value ackStatus,
    const ByteT* packetBytes,
    SizeT packetBytesLength,
    ByteT* outPacketBytes,
    SizeT& inOutPacketBytesLength,
    const Crypto::RSAKey& publicKey)
{
    NetPacketHeaderType::Value headerType = PacketUtility::GetHeaderType(packetBytes, packetBytesLength);
    SizeT headerSize = PacketUtility::GetHeaderSize(headerType);
    SizeT ackSize = PacketUtility::GetAckSize(headerType);
    if (ackSize == 0 || ackSize > inOutPacketBytesLength || headerSize > packetBytesLength)
    {
        return false;
    }
    inOutPacketBytesLength = ackSize;
    switch (headerType)
    {
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE:
        {
            const PacketHeader* header = reinterpret_cast<const PacketHeader*>(packetBytes);
            AckPacketHeader* ack = reinterpret_cast<AckPacketHeader*>(outPacketBytes);
            ack->mAppID = header->mAppID;
            ack->mAppVersion = header->mAppVersion;
            ack->mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
            ack->mType = header->mType;
            ack->mStatus = static_cast<UInt8>(ackStatus);
            ack->mCrc32 = Crc32(&outPacketBytes[AckPacketHeader::CRC_OFFSET], inOutPacketBytesLength - AckPacketHeader::CRC_OFFSET);

        } break;
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED:
        {
            const ConnectedPacketHeader* header = reinterpret_cast<const ConnectedPacketHeader*>(packetBytes);
            AckConnectedPacketHeader* ack = reinterpret_cast<AckConnectedPacketHeader*>(outPacketBytes);
            ack->mAppID = header->mAppID;
            ack->mAppVersion = header->mAppVersion;
            ack->mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
            ack->mType = header->mType;
            ack->mStatus = static_cast<UInt8>(ackStatus);
            *reinterpret_cast<UInt16*>(&ack->mConnectionID[0]) = header->mConnectionID;
            *reinterpret_cast<UInt32*>(&ack->mPacketUID[0]) = header->mPacketUID;
            ack->mCrc32 = Crc32(&outPacketBytes[AckConnectedPacketHeader::CRC_OFFSET], inOutPacketBytesLength - AckConnectedPacketHeader::CRC_OFFSET);

        } break;
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED:
        {
            if (publicKey.GetKeySize() != Crypto::RSA_KEY_2048 || !publicKey.HasPublicKey())
            {
                return false;
            }

            const SecureConnectedPacketHeader* header = reinterpret_cast<const SecureConnectedPacketHeader*>(packetBytes);
            AckSecureConnectedPacketHeader* ack = reinterpret_cast<AckSecureConnectedPacketHeader*>(outPacketBytes);
            ack->mAppID = header->mAppID;
            ack->mAppVersion = header->mAppVersion;
            ack->mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_ACK, NetPacketFlag::NET_PACKET_FLAG_SECURE }).value;
            ack->mType = header->mType;

            AckSecureConnectedPacketHeader::SecureBlock secureBlock;
            secureBlock.mConnectionID = header->mConnectionID;
            secureBlock.mPacketUID = header->mPacketUID;
            secureBlock.mStatus = static_cast<UInt8>(ackStatus);

            SizeT capacity = sizeof(ack->mData);
            if (!Crypto::RSAEncryptPublic(&publicKey, reinterpret_cast<const ByteT*>(&secureBlock), sizeof(secureBlock), ack->mData, capacity))
            {
                return false;
            }
            Assert(capacity <= sizeof(ack->mData));
            ack->mPadding[0] = ack->mPadding[1] = 0;
            ack->mCrc32 = Crc32(&outPacketBytes[AckSecureConnectedPacketHeader::CRC_OFFSET], inOutPacketBytesLength - AckSecureConnectedPacketHeader::CRC_OFFSET);
        } break;
        default:
            CriticalAssertMsgEx("Unknown ack packet header type.", LF_ERROR_INTERNAL, ERROR_API_CORE);
            return false;
    }
    return true;
}

bool PacketUtility::PrepareAckCorruptHeader(
    const ByteT* packetBytes, 
    SizeT packetBytesLength,
    ByteT* outPacketBytes, 
    SizeT& inOutPacketBytesLength,
    const Crypto::RSAKey& publicKey)
{
    return PrepareAckHeader(NetAckStatus::NET_ACK_STATUS_CORRUPT, packetBytes, packetBytesLength, outPacketBytes, inOutPacketBytesLength, publicKey);
}
bool PacketUtility::PrepareAckOkHeader(
    const ByteT* packetBytes, 
    SizeT packetBytesLength, 
    ByteT* outPacketBytes, 
    SizeT& inOutPacketBytesLength,
    const Crypto::RSAKey& publicKey)
{
    return PrepareAckHeader(NetAckStatus::NET_ACK_STATUS_OK, packetBytes, packetBytesLength, outPacketBytes, inOutPacketBytesLength, publicKey);
}

UInt32 PacketUtility::CalcCrc32(const ByteT* packetBytes, SizeT packetBytesLength)
{
    switch (GetHeaderType(packetBytes, packetBytesLength))
    {
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE: 
            return IsAck(packetBytes, packetBytesLength) ?
                    Crc32(&packetBytes[PacketHeader::CRC_OFFSET], packetBytesLength - PacketHeader::CRC_OFFSET)
                  : Crc32(&packetBytes[AckPacketHeader::CRC_OFFSET], packetBytesLength - PacketHeader::CRC_OFFSET);
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED:
            return IsAck(packetBytes, packetBytesLength) ?
                    Crc32(&packetBytes[ConnectedPacketHeader::CRC_OFFSET], packetBytesLength - ConnectedPacketHeader::CRC_OFFSET)
                  : Crc32(&packetBytes[AckConnectedPacketHeader::CRC_OFFSET], packetBytesLength - AckConnectedPacketHeader::CRC_OFFSET);
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED:
            return IsAck(packetBytes, packetBytesLength) ?
                    Crc32(&packetBytes[SecureConnectedPacketHeader::CRC_OFFSET], packetBytesLength - SecureConnectedPacketHeader::CRC_OFFSET)
                  : Crc32(&packetBytes[AckSecureConnectedPacketHeader::CRC_OFFSET], packetBytesLength - AckSecureConnectedPacketHeader::CRC_OFFSET);
    }
    ReportBugMsgEx("Unknown packet header type", LF_ERROR_INTERNAL, ERROR_API_CORE);
    return 0;
}

void* PacketUtility::GetData(ByteT* packetBytes, SizeT packetBytesLength)
{
    switch (GetHeaderType(packetBytes, packetBytesLength))
    {
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE:
            return IsAck(packetBytes, packetBytesLength) ?
                &packetBytes[PacketHeader::ACTUAL_SIZE]
                : &packetBytes[AckPacketHeader::ACTUAL_SIZE];
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_CONNECTED:
            return IsAck(packetBytes, packetBytesLength) ?
                &packetBytes[ConnectedPacketHeader::ACTUAL_SIZE]
                : &packetBytes[AckConnectedPacketHeader::ACTUAL_SIZE];
        case NetPacketHeaderType::NET_PACKET_HEADER_TYPE_SECURE_CONNECTED:
            return IsAck(packetBytes, packetBytesLength) ?
                &packetBytes[SecureConnectedPacketHeader::ACTUAL_SIZE]
                : &packetBytes[AckSecureConnectedPacketHeader::ACTUAL_SIZE];
    }
    ReportBugMsgEx("Unknown packet header type", LF_ERROR_INTERNAL, ERROR_API_CORE);
    return 0;
}

const void* PacketUtility::GetData(const ByteT* packetBytes, SizeT packetBytesLength)
{
    return GetData(const_cast<ByteT*>(packetBytes), packetBytesLength);
}


} // namespace lf
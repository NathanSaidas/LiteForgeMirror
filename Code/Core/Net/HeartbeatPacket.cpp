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
#include "HeartbeatPacket.h"
#include "Core/Common/Assert.h"
#include "Core/Crypto/RSA.h"
#include "Core/Net/PacketUtility.h"

namespace lf {

struct RSAMsg
{
    ByteT clientPing[HeartbeatPacket::MESSAGE_SIZE];
    ByteT serverPing[HeartbeatPacket::MESSAGE_SIZE];
};

bool HeartbeatPacket::EncodePacket(
ByteT* packetBytes,
SizeT& packetBytesLength,
const Crypto::RSAKey& uniqueKey,
const ByteT clientMessage[MESSAGE_SIZE],
const ByteT serverMessage[MESSAGE_SIZE],
ConnectionID connectionID,
UInt32 packetUID)
{
    if (!packetBytes)
    {
        return false;
    }

    if (uniqueKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        ReportBugMsg("Failed to encode heartbeat packet. Unique key is not the correct size.");
        return false;
    }

    const SizeT KEY_SIZE_BYTES = uniqueKey.GetKeySizeBytes();
    CriticalAssert(KEY_SIZE_BYTES == 256);

    if (packetBytesLength < (HeaderType::ACTUAL_SIZE + KEY_SIZE_BYTES))
    {
        ReportBugMsg("Failed to encode heartbeat packet. Packet is not large enough to contain the message.");
        return false;
    }

    HeaderType* header = reinterpret_cast<HeaderType*>(packetBytes);
    header->mAppID = NetConfig::NET_APP_ID;
    header->mAppVersion = NetConfig::NET_APP_VERSION;
    header->mType = NetPacketType::NET_PACKET_TYPE_HEARTBEAT;
    header->mFlags = 0; // implicit Reliability/Security
    header->mConnectionID = static_cast<UInt16>(connectionID); // todo: ConnectionID is u16 in packet but u32 in client-storage. (Is this OK?)
    header->mPacketUID = packetUID;

    // If this triggers it means we have to adjust the 'rsa bytes' base pointer.
    LF_STATIC_ASSERT(HeaderType::ACTUAL_SIZE == 16);

    RSAMsg msg;
    memcpy(msg.clientPing, clientMessage, MESSAGE_SIZE);
    memcpy(msg.serverPing, serverMessage, MESSAGE_SIZE);

    ByteT* rsaBytes = &packetBytes[HeaderType::ACTUAL_SIZE];
    SizeT rsaBytesLength = packetBytesLength - HeaderType::ACTUAL_SIZE;

    if (!Crypto::RSAEncryptPublic(&uniqueKey, reinterpret_cast<const ByteT*>(&msg), sizeof(RSAMsg), rsaBytes, rsaBytesLength))
    {
        return false;
    }

    if (rsaBytesLength != KEY_SIZE_BYTES)
    {
        ReportBugMsg("Unexpected RSA cipher text size.");
    }
    
    packetBytesLength = rsaBytesLength + HeaderType::ACTUAL_SIZE;
    header->mCrc32 = PacketUtility::CalcCrc32(packetBytes, packetBytesLength);
    return true;
}

bool HeartbeatPacket::DecodePacket(
const ByteT* packetBytes,
SizeT packetBytesLength,
const Crypto::RSAKey& uniqueKey,
ByteT clientMessage[MESSAGE_SIZE],
ByteT serverMessage[MESSAGE_SIZE],
HeaderType& header)
{
    if (!packetBytes)
    {
        return false;
    }

    if (uniqueKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        ReportBugMsg("Failed to decode heartbeat packet. Unique key is not the correct size.");
        return false;
    }

    const SizeT KEY_SIZE_BYTES = uniqueKey.GetKeySizeBytes();
    CriticalAssert(KEY_SIZE_BYTES == 256);

    if (packetBytesLength < (HeaderType::ACTUAL_SIZE + KEY_SIZE_BYTES))
    {
        ReportBugMsg("Failed to decode heartbeat packet. Packet is not large enough to contain the message.");
        return false;
    }

    const HeaderType* packetHeader = reinterpret_cast<const HeaderType*>(packetBytes);
    if (packetHeader->mAppID != NetConfig::NET_APP_ID)
    {
        return false;
    }

    if (packetHeader->mAppVersion != NetConfig::NET_APP_VERSION)
    {
        return false;
    }

    if (packetHeader->mCrc32 != PacketUtility::CalcCrc32(packetBytes, packetBytesLength))
    {
        return false;
    }

    if (packetHeader->mType != NetPacketType::NET_PACKET_TYPE_HEARTBEAT)
    {
        return false;
    }

    if (NetPacketFlag::BitfieldType(packetHeader->mFlags).Has(NetPacketFlag::NET_PACKET_FLAG_ACK))
    {
        return false;
    }

    // If this triggers it means we have to adjust the 'rsa bytes' base pointer.
    LF_STATIC_ASSERT(HeaderType::ACTUAL_SIZE == 16);
    const ByteT* rsaBytes = &packetBytes[HeaderType::ACTUAL_SIZE];

    ByteT rsaPlainText[257];
    SizeT rsaPlainTextSize = sizeof(rsaPlainText) - 1;

    if (!Crypto::RSADecryptPrivate(&uniqueKey, rsaBytes, KEY_SIZE_BYTES, rsaPlainText, rsaPlainTextSize))
    {
        return false;
    }

    RSAMsg* rsa = reinterpret_cast<RSAMsg*>(rsaPlainText);
    memcpy(clientMessage, rsa->clientPing, MESSAGE_SIZE);
    memcpy(serverMessage, rsa->serverPing, MESSAGE_SIZE);
    header = *packetHeader;
    return true;
}

bool HeartbeatPacket::EncodeAckPacket(
ByteT* packetBytes,
SizeT& packetBytesLength,
const Crypto::RSAKey& clientKey,
const ByteT clientMessage[MESSAGE_SIZE],
const ByteT serverMessage[MESSAGE_SIZE],
UInt32 packetUID)
{
    if (!packetBytes)
    {
        return false;
    }

    if (clientKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        ReportBugMsg("Failed to encode heartbeat ack packet. Client key is not the correct size.");
        return false;
    }

    const SizeT KEY_SIZE_BYTES = clientKey.GetKeySizeBytes();
    CriticalAssert(KEY_SIZE_BYTES == 256);

    if (packetBytesLength < (AckHeaderType::ACTUAL_SIZE + KEY_SIZE_BYTES))
    {
        ReportBugMsg("Failed to encode heartbeat ack packet. Packet is not large enough to contain the message.");
        return false;
    }

    AckHeaderType* header = reinterpret_cast<AckHeaderType*>(packetBytes);
    header->mAppID = NetConfig::NET_APP_ID;
    header->mAppVersion = NetConfig::NET_APP_VERSION;
    header->mType = NetPacketType::NET_PACKET_TYPE_HEARTBEAT;
    header->mFlags = NetPacketFlag::BitfieldType({ NetPacketFlag::NET_PACKET_FLAG_ACK }).value;
    SetPacketUID(*header, packetUID);
    header->mStatus = NetAckStatus::NET_ACK_STATUS_OK;

    RSAMsg msg;
    memcpy(msg.clientPing, clientMessage, MESSAGE_SIZE);
    memcpy(msg.serverPing, serverMessage, MESSAGE_SIZE);

    ByteT* rsaBytes = &packetBytes[AckHeaderType::ACTUAL_SIZE];
    SizeT rsaBytesLength = packetBytesLength - AckHeaderType::ACTUAL_SIZE;

    if (!Crypto::RSAEncryptPublic(&clientKey, reinterpret_cast<const ByteT*>(&msg), sizeof(RSAMsg), rsaBytes, rsaBytesLength))
    {
        return false;
    }

    if (rsaBytesLength != KEY_SIZE_BYTES)
    {
        ReportBugMsg("Unexpected RSA cipher text size.");
    }

    packetBytesLength = rsaBytesLength + AckHeaderType::ACTUAL_SIZE;
    header->mCrc32 = PacketUtility::CalcCrc32(packetBytes, packetBytesLength);
    return true;
}

bool HeartbeatPacket::DecodeAckPacket(
const ByteT* packetBytes,
SizeT packetBytesLength,
const Crypto::RSAKey& clientKey,
ByteT clientMessage[MESSAGE_SIZE],
ByteT serverMessage[MESSAGE_SIZE],
UInt32& packetUID,
AckHeaderType& header)
{
    if (!packetBytes)
    {
        return false;
    }

    if (clientKey.GetKeySize() != Crypto::RSA_KEY_2048)
    {
        ReportBugMsg("Failed to decode heartbeat ack packet. Client key is not the correct size.");
        return false;
    }

    const SizeT KEY_SIZE_BYTES = clientKey.GetKeySizeBytes();
    CriticalAssert(KEY_SIZE_BYTES == 256);

    if (packetBytesLength < (AckHeaderType::ACTUAL_SIZE + KEY_SIZE_BYTES))
    {
        ReportBugMsg("Failed to decode heartbeat ack packet. Packet is not large enough to contain the message.");
        return false;
    }

    const AckHeaderType* packetHeader = reinterpret_cast<const AckHeaderType*>(packetBytes);
    if (packetHeader->mAppID != NetConfig::NET_APP_ID)
    {
        return false;
    }

    if (packetHeader->mAppVersion != NetConfig::NET_APP_VERSION)
    {
        return false;
    }

    if (packetHeader->mCrc32 != PacketUtility::CalcCrc32(packetBytes, packetBytesLength))
    {
        return false;
    }

    if (packetHeader->mType != NetPacketType::NET_PACKET_TYPE_HEARTBEAT)
    {
        return false;
    }

    if (!NetPacketFlag::BitfieldType(packetHeader->mFlags).Has(NetPacketFlag::NET_PACKET_FLAG_ACK))
    {
        return false;
    }

    // We can receive a 'valid' packet that has a not OK status. In this case we don't have any data in the RSA Block
    // Make sure as the 'caller' to check the status of the packet.
    if (packetHeader->mStatus != NetAckStatus::NET_ACK_STATUS_OK)
    {
        header = *packetHeader;
        packetUID = GetPacketUID(*packetHeader);
        return true;
    }

    const ByteT* rsaBytes = &packetBytes[AckHeaderType::ACTUAL_SIZE];

    ByteT rsaPlainText[257];
    SizeT rsaPlainTextSize = sizeof(rsaPlainText) - 1;

    if (!Crypto::RSADecryptPrivate(&clientKey, rsaBytes, KEY_SIZE_BYTES, rsaPlainText, rsaPlainTextSize))
    {
        return false;
    }

    RSAMsg* rsa = reinterpret_cast<RSAMsg*>(rsaPlainText);
    memcpy(clientMessage, rsa->clientPing, MESSAGE_SIZE);
    memcpy(serverMessage, rsa->serverPing, MESSAGE_SIZE);
    header = *packetHeader;
    packetUID = GetPacketUID(*packetHeader);
    return true;
}

} // namespace 
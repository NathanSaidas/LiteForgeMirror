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
#ifndef LF_CORE_HEARTBEAT_PACKET_H
#define LF_CORE_HEARTBEAT_PACKET_H
#include "Core/Net/NetTypes.h"

namespace lf {
namespace Crypto {
class RSAKey;
}

class LF_CORE_API HeartbeatPacket
{
public:
    using HeaderType = ConnectedPacketHeader;
    using AckHeaderType = AckConnectedPacketHeader;

    static const SizeT MESSAGE_SIZE = 32;

    // Static class cannot be instantiated.
    HeartbeatPacket() = delete;
    HeartbeatPacket(const HeartbeatPacket&) = delete;
    HeartbeatPacket(HeartbeatPacket&& other) = delete;
    ~HeartbeatPacket() = delete;

    static bool EncodePacket(
        ByteT* packetBytes,
        SizeT& packetBytesLength,
        const Crypto::RSAKey& uniqueKey,
        const ByteT clientMessage[MESSAGE_SIZE],
        const ByteT serverMessage[MESSAGE_SIZE],
        ConnectionID id,
        UInt32 packetUID);

    static bool DecodePacket(
        const ByteT* packetBytes,
        SizeT packetBytesLength,
        const Crypto::RSAKey& uniqueKey,
        ByteT clientMessage[MESSAGE_SIZE],
        ByteT serverMessage[MESSAGE_SIZE],
        HeaderType& header);

    static bool EncodeAckPacket(
        ByteT* packetBytes,
        SizeT& packetBytesLength,
        const Crypto::RSAKey& clientKey,
        const ByteT clientMessage[MESSAGE_SIZE],
        const ByteT serverMessage[MESSAGE_SIZE],
        UInt32 packetUID);

    static bool DecodeAckPacket(
        const ByteT* packetBytes,
        SizeT packetBytesLength,
        const Crypto::RSAKey& clientKey,
        ByteT clientMessage[MESSAGE_SIZE],
        ByteT serverMessage[MESSAGE_SIZE],
        UInt32& packetUID,
        AckHeaderType& header);
};

} // namespace lf

#endif // LF_CORE_HEARTBEAT_PACKET_H
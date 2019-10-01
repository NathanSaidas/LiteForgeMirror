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
#ifndef LF_CORE_PACKET_UTILITY_H
#define LF_CORE_PACKET_UTILITY_H

#include "Core/Net/NetTypes.h"
#include "Core/Utility/Utility.h"

namespace lf {

namespace Crypto {
class RSAKey;
} // namespace Crypto

class LF_CORE_API PacketUtility
{
public:
    PacketUtility() = delete;
    ~PacketUtility() = delete;

    // **********************************
    // The size of the largest acknowledgement packet.
    // **********************************
    static const SizeT MAX_PACKET_ACKNOWLEDGEMENT_SIZE = Max(Max(sizeof(AckPacketHeader), sizeof(AckConnectedPacketHeader)), sizeof(AckSecureConnectedPacketHeader));
    
    // **********************************
    // Checks if the bytes of a packet resemble something that should be 'connected'
    // 
    // note: This is does not take into account the size of the packet. I merely checks flags and data on the 'base' packet header.
    // **********************************
    static bool IsConnected(const ByteT* packetBytes, SizeT packetBytesLength);
    // **********************************
    // Checks if the bytes of a packet resemble something that should be 'secure' and 'connected'
    //
    // note: This is does not take into account the size of the packet. I merely checks flags and data on the 'base' packet header.
    // **********************************
    static bool IsSecureConnected(const ByteT* packetBytes, SizeT packetBytesLength);
    // **********************************
    // Determines what type of packet header is used in the given packet bytes.
    // 
    // note: This is does not take into account the size of the packet. I merely checks flags and data on the 'base' packet header.
    // **********************************
    static NetPacketHeaderType::Value GetHeaderType(const ByteT* packetBytes, SizeT packetBytesLength);
    // **********************************
    // Checks the header flag for whether or not the packet is an acknowledgement.
    // **********************************
    static bool IsAck(const ByteT* packetBytes, SizeT packetBytesLength);
    // **********************************
    // Determines the size of the header based on header type
    // **********************************
    static SizeT GetHeaderSize(NetPacketHeaderType::Value headerType);
    // **********************************
    // Determines the size of the acknowledgement based on header type
    // **********************************
    static SizeT GetAckSize(NetPacketHeaderType::Value headerType);
    // **********************************
    // Prepares the acknowledgement packet header for a corrupt packet.
    //
    // note: This method assumes the packetBytes are decrypted for secure packets.
    // **********************************
    static bool PrepareAckCorruptHeader(
        const ByteT* packetBytes, 
        SizeT packetBytesLength, 
        ByteT* outPacketBytes, 
        SizeT& inOutPacketBytesLength, 
        const Crypto::RSAKey& publicKey);
    // **********************************
    // Prepares the acknowledgement packet header for a good packet.
    //
    // note: This method assumes the packetBytes are decrypted for secure packets.
    // **********************************
    static bool PrepareAckOkHeader(
        const ByteT* packetBytes, 
        SizeT packetBytesLength, 
        ByteT* outPacketBytes, 
        SizeT& inOutPacketBytesLength, 
        const Crypto::RSAKey& publicKey);
    // **********************************
    // Utility to calculate the Crc32 of a packet. Deduces the packet type and calculates with the correct offset.
    // **********************************
    static UInt32 CalcCrc32(const ByteT* packetBytes, SizeT packetBytesLength);
    
    static void* GetData(ByteT* packetBytes, SizeT packetBytesLength);
    static const void* GetData(const ByteT* packetBytes, SizeT packetBytesLength);
};

} // namespace lf

#endif // LF_CORE_PACKET_UTILITY_H
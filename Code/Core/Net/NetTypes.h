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
#ifndef LF_CORE_NET_TYPES_H
#define LF_CORE_NET_TYPES_H

#include "Core/Common/Types.h"
#include "Core/Common/API.h"
#include "Core/Utility/Bitfield.h"

#include <utility>

namespace lf {

namespace NetConfig {
const UInt16 NET_APP_ID = 0x0001;
const UInt16 NET_APP_VERSION = 0x0001;
}

namespace NetProtocol
{
    enum Value
    {
        NET_PROTOCOL_IPV4_UDP,
        NET_PROTOCOL_IPV6_UDP,
        NET_PROTOCOL_UDP,       // Agnostic to IPV6/IPV4 traffic, IPV4 traffic is converted to IPV6 mapped address.

        MAX_VALUE,
        INVALID_ENUM = NetProtocol::MAX_VALUE
    };
}

namespace NetAddressFamily
{
    enum Value
    {
        NET_ADDRESS_FAMILY_IPV4,
        NET_ADDRESS_FAMILY_IPV6,

        MAX_VALUE,
        INVALID_ENUM = NetAddressFamily::MAX_VALUE
    };
}

namespace NetPacketType
{
    // Different packet types use different 'Packet Protocols'
    // 
    // Packet Protocol Request/Response: 
    //                 Client.Request -> Server.Ack -> Server.Response -> Client.Ack
    // Packet Protocol Message:
    //                 Client.Message -> Server.Ack
    //                 Server.Message -> Client.Ack
    // Packet Protocol Handshake:
    //                 Client.Message -> Server.Ack -> Client.Ack
    //                 Server.Message -> Client.Ack -> Server.Ack
    enum Value
    {
        // Connection is a Request/Reponse protocol
        NET_PACKET_TYPE_CONNECT,
        // Disconnect is an Unreliable message.
        NET_PACKET_TYPE_DISCONNECT,
        // Heartbeat is an Reliable handshake sent from clients.
        NET_PACKET_TYPE_HEARTBEAT,
        // ???
        NET_PACKET_TYPE_MESSAGE,

        MAX_VALUE,
        INVALID_ENUM = NetPacketType::MAX_VALUE
    };
}

namespace NetPacketFlag
{
    enum Value
    {
        NET_PACKET_FLAG_RELIABILITY, // If this flag is turned on and the message is not the receiver sends back an ACK
        NET_PACKET_FLAG_ORDER_WEAK,  // If this flag is turned on the packet is being sent in 'weak order'
        NET_PACKET_FLAG_ORDER_STRICT,// If this flag is turned on the packet is being sent in 'strict order'
        NET_PACKET_FLAG_COMPRESSION, // If this flag is turned on the packet was compressed and must be decompressed before processing
        NET_PACKET_FLAG_ACK,         // If this flag is turned on the packet was sent as an ACK corresponding with the packet type
        NET_PACKET_FLAG_SYNC,        // If this flag is turned on the packet is to be processed ASAP vs at end/begin of frame.
        NET_PACKET_FLAG_SECURE,      // If this flag is turned on the packet contains a security header that must be used to decrypt/verify the rest of the packet (connected peers only)
        NET_PACKET_FLAG_IPV4,        // If this flag is turned on the packet was sent by someone using IPV4 and must be translated back.

        MAX_VALUE,
        INVALID_ENUM = NetPacketFlag::MAX_VALUE
    };
    using BitfieldType = Bitfield<Value, UInt8>;
}

namespace NetPacketHeaderType
{
    enum Value
    {
        NET_PACKET_HEADER_TYPE_BASE,
        NET_PACKET_HEADER_TYPE_CONNECTED,
        NET_PACKET_HEADER_TYPE_SECURE_CONNECTED,

        MAX_VALUE,
        INVALID_ENUM = NetPacketFlag::MAX_VALUE
    };
}

namespace NetAckStatus
{
    enum Value
    {
        NET_ACK_STATUS_OK,
        NET_ACK_STATUS_CORRUPT,
        NET_ACK_STATUS_REJECTED,
        NET_ACK_STATUS_FORBIDDEN,
        NET_ACK_STATUS_NOT_FOUND,
        NET_ACK_STATUS_UNAUTHORIZED,
        NET_ACK_STATUS_INVALID_REQUEST,

        MAX_VALUE,
        INVALID_ENUM = NetPacketFlag::MAX_VALUE
    };
}

// **********************************
// The absolute base data structure for
// packets. All packets will use this
// format.
// **********************************
struct PacketHeader
{
    static const SizeT CRC_OFFSET = sizeof(UInt16) + sizeof(UInt16) + sizeof(UInt32);
    static const SizeT RUNTIME_SIZE = 12;
    static const SizeT ACTUAL_SIZE = RUNTIME_SIZE - 2;

    UInt16 mAppID;      // 0, 2 => 2
    UInt16 mAppVersion; // 2, 2 => 4
    UInt32 mCrc32;      // 4, 4 => 8
    UInt8  mFlags;      // 8, 1 => 9
    UInt8  mType;       // 9, 1 => 10

    UInt8  mPadding[2];
};
LF_STATIC_ASSERT(sizeof(PacketHeader) == PacketHeader::RUNTIME_SIZE);

// **********************************
// If you're sending/receiving a packet type 
// that requires a connection (which is almost
// all of them) then this packet is used.
// **********************************
struct ConnectedPacketHeader
{
    using Base = PacketHeader;
    static const SizeT CRC_OFFSET = PacketHeader::CRC_OFFSET;
    static const SizeT RUNTIME_SIZE = 16;
    static const SizeT ACTUAL_SIZE = RUNTIME_SIZE - 0;

    UInt16 mAppID;        //  0, 2 => 2
    UInt16 mAppVersion;   //  2, 2 => 4
    UInt32 mCrc32;        //  4, 4 => 8
    UInt8  mFlags;        //  8, 1 => 9
    UInt8  mType;         //  9, 1 => 10
    UInt16 mConnectionID; // 10, 2 => 12
    UInt32 mPacketUID;    // 12, 4 => 16
};
LF_STATIC_ASSERT(sizeof(ConnectedPacketHeader) == ConnectedPacketHeader::RUNTIME_SIZE);

// **********************************
// If you're sending/receiving a packet type
// that requires a connection and is flagged
// with SECURE then use this packet.
// 
// note: All data past the hash is used using
// SHA256
// note: All data from that hash to the end is
// encrypted using the client 'shared' key.
// **********************************
struct SecureConnectedPacketHeader
{
    using Base = PacketHeader;
    static const SizeT CRC_OFFSET = PacketHeader::CRC_OFFSET;
    static const SizeT RUNTIME_SIZE = 60;
    static const SizeT ACTUAL_SIZE = RUNTIME_SIZE - 2;

    UInt16 mAppID;        //  0,  2 => 2
    UInt16 mAppVersion;   //  2,  2 => 4
    UInt32 mCrc32;        //  4,  4 => 8
    UInt8  mFlags;        //  8,  1 => 9
    UInt8  mType;         //  9,  1 => 10
    UInt8  mHash[32];     // 10, 32 => 42
    UInt16 mConnectionID; // 42,  2 => 44
    UInt32 mPacketUID;    // 44,  4 => 48
    UInt8  mReservedPadding[16 - 6];
    UInt8  mPadding[2];
};
LF_STATIC_ASSERT(sizeof(SecureConnectedPacketHeader) == SecureConnectedPacketHeader::RUNTIME_SIZE);

// **********************************
// If you're sending/receiving a packet type
// that requires a connection and is flagged
// with SECURE then use this packet.
// 
// note: All data past the hash is used using
// SHA256
// note: All data from that hash to the end is
// encrypted using the client 'shared' key.
// **********************************
struct AckPacketHeader
{
    using Base = PacketHeader;
    static const SizeT CRC_OFFSET = PacketHeader::CRC_OFFSET;
    static const SizeT RUNTIME_SIZE = 12;
    static const SizeT ACTUAL_SIZE = RUNTIME_SIZE - 1;

    UInt16 mAppID;        //  0,  2 => 2
    UInt16 mAppVersion;   //  2,  2 => 4
    UInt32 mCrc32;        //  4,  4 => 8
    UInt8  mFlags;        //  8,  1 => 9
    UInt8  mType;         //  9,  1 => 10
    UInt8  mStatus;       // 10,  1 => 11
    UInt8  mPadding;
};
LF_STATIC_ASSERT(sizeof(AckPacketHeader) == AckPacketHeader::RUNTIME_SIZE);

struct AckConnectedPacketHeader
{
    using Base = AckPacketHeader;
    static const SizeT CRC_OFFSET = PacketHeader::CRC_OFFSET;
    static const SizeT RUNTIME_SIZE = 16;
    static const SizeT ACTUAL_SIZE = RUNTIME_SIZE - 1;

    UInt16 mAppID;          //  0,  2 => 2
    UInt16 mAppVersion;     //  2,  2 => 4
    UInt32 mCrc32;          //  4,  4 => 8
    UInt8  mFlags;          //  8,  1 => 9
    UInt8  mType;           //  9,  1 => 10
    UInt8  mStatus;         // 10,  1 => 11
    UInt8  mPacketUID[4];   // 11,  4 => 15
    UInt8  mPadding[1];
};
LF_STATIC_ASSERT(sizeof(AckConnectedPacketHeader) == AckConnectedPacketHeader::RUNTIME_SIZE);

struct AckSecureConnectedPacketHeader
{
    using Base = AckPacketHeader;
    struct SecureBlock
    {
        UInt32 mPacketUID;
        UInt16 mConnectionID;
        UInt8  mStatus;
    };

    static const SizeT CRC_OFFSET = PacketHeader::CRC_OFFSET;
    static const SizeT RUNTIME_SIZE = 268;
    static const SizeT ACTUAL_SIZE = RUNTIME_SIZE - 2;
    // static const SizeT SECURE_BLOCK_SIZE = 16;

    UInt16 mAppID;          //  0,   2 => 2
    UInt16 mAppVersion;     //  2,   2 => 4
    UInt32 mCrc32;          //  4,   4 => 8
    UInt8  mFlags;          //  8,   1 => 9
    UInt8  mType;           //  9,   1 => 10
    ByteT  mData[256];      // 10, 256 => 266
    UInt8  mPadding[2];     // 266,  2 => 268
};
LF_STATIC_ASSERT(sizeof(AckSecureConnectedPacketHeader) == AckSecureConnectedPacketHeader::RUNTIME_SIZE);

struct LF_ALIGN(4) IPv4EndPoint
{
    LF_INLINE IPv4EndPoint()
    : mAddressFamily(NetAddressFamily::INVALID_ENUM)
    , mPort(0)
    , mAddress()
    {
        mAddress.mWord = 0;
    }

    LF_INLINE IPv4EndPoint(const IPv4EndPoint& other)
    : mAddressFamily(other.mAddressFamily)
    , mPort(other.mPort)
    , mAddress(other.mAddress)
    {}

    LF_INLINE IPv4EndPoint(IPv4EndPoint&& other)
        : mAddressFamily(other.mAddressFamily)
        , mPort(other.mPort)
        , mAddress(other.mAddress)
    {
        other.mAddressFamily = NetAddressFamily::INVALID_ENUM;
        other.mPort = 0;
        other.mAddress.mWord = 0;
    }

    LF_INLINE IPv4EndPoint& operator=(const IPv4EndPoint& other)
    {
        mAddressFamily = other.mAddressFamily;
        mPort = other.mPort;
        mAddress.mWord = other.mAddress.mWord;
        return *this;
    }

    LF_INLINE IPv4EndPoint& operator=(IPv4EndPoint&& other)
    {
        mAddressFamily = other.mAddressFamily;
        mPort = other.mPort;
        mAddress.mWord = other.mAddress.mWord;
        other.mAddressFamily = NetAddressFamily::INVALID_ENUM;
        other.mPort = 0;
        other.mAddress.mWord = 0;
        return *this;
    }

    bool operator==(const IPv4EndPoint& other) const
    {
        return mAddressFamily == other.mAddressFamily
            && mPort == other.mPort
            && mAddress.mWord == other.mAddress.mWord;
    }

    bool operator!=(const IPv4EndPoint& other) const
    {
        return mAddressFamily != other.mAddressFamily
            || mPort != other.mPort
            || mAddress.mWord != other.mAddress.mWord;
    }

    UInt16 mAddressFamily;
    // The port of the end point in network byte order.
    UInt16 mPort;
    union {
        UInt8  mBytes[4];
        UInt32 mWord;
    } mAddress;
};

struct LF_ALIGN(4) IPv6EndPoint
{
    LF_INLINE IPv6EndPoint()
        : mAddressFamily(NetAddressFamily::INVALID_ENUM)
        , mPort(0)
        , mAddress()
    {
        reinterpret_cast<UInt64*>(mAddress.mWord)[0] = 0;
        reinterpret_cast<UInt64*>(mAddress.mWord)[1] = 0;
    }

    LF_INLINE IPv6EndPoint(const IPv6EndPoint& other)
        : mAddressFamily(other.mAddressFamily)
        , mPort(other.mPort)
        , mAddress()
    {
        reinterpret_cast<UInt64*>(mAddress.mWord)[0] = reinterpret_cast<const UInt64*>(other.mAddress.mWord)[0];
        reinterpret_cast<UInt64*>(mAddress.mWord)[1] = reinterpret_cast<const UInt64*>(other.mAddress.mWord)[1];
    }
    
    LF_INLINE IPv6EndPoint(IPv6EndPoint&& other)
        : mAddressFamily(other.mAddressFamily)
        , mPort(other.mPort)
        , mAddress()
    {
        reinterpret_cast<UInt64*>(mAddress.mWord)[0] = reinterpret_cast<UInt64*>(other.mAddress.mWord)[0];
        reinterpret_cast<UInt64*>(mAddress.mWord)[1] = reinterpret_cast<UInt64*>(other.mAddress.mWord)[1];
        other.mAddressFamily = NetAddressFamily::INVALID_ENUM;
        other.mPort = 0;
        reinterpret_cast<UInt64*>(other.mAddress.mWord)[0] = 0;
        reinterpret_cast<UInt64*>(other.mAddress.mWord)[1] = 0;
    }
    
    LF_INLINE IPv6EndPoint& operator=(const IPv6EndPoint& other)
    {
        mAddressFamily = other.mAddressFamily;
        mPort = other.mPort;
        reinterpret_cast<UInt64*>(mAddress.mWord)[0] = reinterpret_cast<const UInt64*>(other.mAddress.mWord)[0];
        reinterpret_cast<UInt64*>(mAddress.mWord)[1] = reinterpret_cast<const UInt64*>(other.mAddress.mWord)[1];
        return *this;
    }
    
    LF_INLINE IPv6EndPoint& operator=(IPv6EndPoint&& other)
    {
        mAddressFamily = other.mAddressFamily;
        mPort = other.mPort;
        reinterpret_cast<UInt64*>(mAddress.mWord)[0] = reinterpret_cast<UInt64*>(other.mAddress.mWord)[0];
        reinterpret_cast<UInt64*>(mAddress.mWord)[1] = reinterpret_cast<UInt64*>(other.mAddress.mWord)[1];
        other.mAddressFamily = NetAddressFamily::INVALID_ENUM;
        other.mPort = 0;
        reinterpret_cast<UInt64*>(other.mAddress.mWord)[0] = 0;
        reinterpret_cast<UInt64*>(other.mAddress.mWord)[1] = 0;
        return *this;
    }
    
    bool operator==(const IPv6EndPoint& other) const
    {
        return mAddressFamily == other.mAddressFamily
            && mPort == other.mPort
            && reinterpret_cast<const UInt64*>(mAddress.mWord)[0] == reinterpret_cast<const UInt64*>(other.mAddress.mWord)[0]
            && reinterpret_cast<const UInt64*>(mAddress.mWord)[1] == reinterpret_cast<const UInt64*>(other.mAddress.mWord)[1];
    }
    
    bool operator!=(const IPv6EndPoint& other) const
    {
        return mAddressFamily != other.mAddressFamily
            || mPort != other.mPort
            || reinterpret_cast<const UInt64*>(mAddress.mWord)[0] != reinterpret_cast<const UInt64*>(other.mAddress.mWord)[0]
            || reinterpret_cast<const UInt64*>(mAddress.mWord)[1] != reinterpret_cast<const UInt64*>(other.mAddress.mWord)[1];
    }

    UInt16 mAddressFamily;
    // The port of the end point in network byte order.
    UInt16 mPort;
    union {
        UInt8  mBytes[16];
        UInt16 mWord[8];
    } mAddress;
};

struct LF_ALIGN(4) IPEndPointAny
{
    IPEndPointAny()
    : mAddressFamily(NetAddressFamily::INVALID_ENUM)
    , mPort(0)
    , mPadding()
    {
        memset(mPadding.mBytes, 0, sizeof(mPadding));
    }

    IPEndPointAny(const IPEndPointAny& other)
    : mAddressFamily(other.mAddressFamily)
    , mPort(other.mPort)
    , mPadding()
    {
        memcpy(mPadding.mBytes, other.mPadding.mBytes, sizeof(mPadding));
    }

    IPEndPointAny(IPEndPointAny&& other)
    : mAddressFamily(other.mAddressFamily)
    , mPort(other.mPort)
    , mPadding()
    {
        memcpy(mPadding.mBytes, other.mPadding.mBytes, sizeof(mPadding.mBytes));
        other.mAddressFamily = NetAddressFamily::INVALID_ENUM;
        other.mPort = 0;
        memset(other.mPadding.mBytes, 0, sizeof(other.mPadding));
    }

    IPEndPointAny& operator=(const IPEndPointAny& other)
    {
        if (this != &other)
        {
            mAddressFamily = other.mAddressFamily;
            mPort = other.mPort;
            memcpy(mPadding.mBytes, other.mPadding.mBytes, sizeof(mPadding));
        }
        return *this;
    }

    IPEndPointAny& operator=(IPEndPointAny&& other)
    {
        if (this != &other)
        {
            mAddressFamily = other.mAddressFamily;
            other.mAddressFamily = NetAddressFamily::INVALID_ENUM;
            mPort = other.mPort;
            other.mPort = 0;
            memcpy(mPadding.mBytes, other.mPadding.mBytes, sizeof(mPadding));
            memset(other.mPadding.mBytes, 0, sizeof(other.mPadding));
        }
        return *this;
    }

    bool operator==(const IPEndPointAny& other) const
    {
        return mAddressFamily == other.mAddressFamily && mPort == other.mPort && memcmp(mPadding.mBytes, other.mPadding.mBytes, sizeof(mPadding)) == 0;
    }

    bool operator!=(const IPEndPointAny& other) const
    {
        return mAddressFamily != other.mAddressFamily || mPort != other.mPort || memcmp(mPadding.mBytes, other.mPadding.mBytes, sizeof(mPadding)) != 0;
    }

    UInt16 mAddressFamily;
    // The port of the end point in network byte order.
    UInt16 mPort;
    union {
        UInt8  mBytes[16];
        UInt32 mWord[16 / 4];
    } mPadding;
};

namespace ConnectionFailureMsg
{
    enum Value
    {
        // The server may explicitly reject us or we we're unable to decode the 
        // server's message.
        CFM_UNKNOWN,
        // The server did not respond (either our request never made it to them
        // or they actively chose to not respond).
        CFM_TIMED_OUT,
        // The server received our message but explicitly denied us because they
        // have reached the maximum number of connections they support.
        CFM_SERVER_FULL,

        MAX_VALUE,
        INVALID_ENUM = ConnectionFailureMsg::MAX_VALUE
    };
}

namespace PacketDataType
{
    enum Value
    {
        PDT_4096,
        PDT_2048,
        PDT_1024,
        PDT_768,
        PDT_512,

        MAX_VALUE,
        INVALID_ENUM = PacketDataType::MAX_VALUE
    };
}

struct PacketData
{
    UInt32 mType;
    UInt16 mSize;
    UInt16 mRetransmits;
    IPEndPointAny mSender;

    template<typename T>
    static void SetZero(T& packet)
    {
        memset(&packet, 0, sizeof(T));
    }
};

template<SizeT PacketSizeT>
struct TPacketData : PacketData
{
    ByteT mBytes[PacketSizeT];
};
using PacketData4096 = TPacketData<4096>;
using PacketData2048 = TPacketData<2048>;
using PacketData1024 = TPacketData<1024>;
using PacketData768 = TPacketData<768>;
using PacketData512 = TPacketData<512>;
using ConnectionID = Int32;

namespace PacketDataType
{
    using ConnectPacketData = PacketData1024;
    using ConnectAckPacketData = PacketData1024;
}

const ConnectionID INVALID_CONNECTION = INVALID32;
const SizeT NET_CLIENT_CHALLENGE_SIZE = 32;
const SizeT NET_HEARTBEAT_NONCE_SIZE = 32;

#if defined(LF_OS_WINDOWS)
class LF_IMPL_OPAQUE(UDPSocketWindows);
using LF_IMPL_OPAQUE(UDPSocket) = LF_IMPL_OPAQUE(UDPSocketWindows);
#else
#error Missing platform implementation.
#endif

class LF_IMPL_OPAQUE(NetTransport);

LF_INLINE void SetPacketUID(ConnectedPacketHeader& header, UInt32 uid)
{
    LF_STATIC_ASSERT(sizeof(header.mPacketUID) == sizeof(uid));
    header.mPacketUID = uid;
}
LF_INLINE void SetPacketUID(AckConnectedPacketHeader& header, UInt32 uid)
{
    LF_STATIC_ASSERT(sizeof(header.mPacketUID) == sizeof(uid));
    memcpy(header.mPacketUID, &uid, sizeof(uid));
}
LF_INLINE void SetPacketUID(SecureConnectedPacketHeader& header, UInt32 uid)
{
    LF_STATIC_ASSERT(sizeof(header.mPacketUID) == sizeof(uid));
    header.mPacketUID = uid;
    // memcpy(header.mPacketUID, &uid, sizeof(uid));
}
// void SetPacketUID(AckSecureConnectedPacketHeader& header, UInt32 uid)
// {
//     // todo:
//     // LF_STATIC_ASSERT(sizeof(header.mPacketUID) == sizeof(uid));
//     (header);
//     (uid);
//     
// }

LF_INLINE UInt32 GetPacketUID(const ConnectedPacketHeader& header)
{
    LF_STATIC_ASSERT(sizeof(header.mPacketUID) == sizeof(UInt32));
    return header.mPacketUID;
}

LF_INLINE UInt32 GetPacketUID(const AckConnectedPacketHeader& header)
{
    LF_STATIC_ASSERT(sizeof(header.mPacketUID) == sizeof(UInt32));
    UInt32 value;
    memcpy(&value, header.mPacketUID, sizeof(UInt32));
    return value;
}

LF_INLINE UInt32 GetPacketUID(const SecureConnectedPacketHeader& header)
{
    LF_STATIC_ASSERT(sizeof(header.mPacketUID) == sizeof(UInt32));
    return header.mPacketUID;
}

// todo: 
// void GetPacketUID(const AckSecureConnectedPacketHeader& header)



}

#endif // LF_CORE_NET_TYPES_H
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
#include "UDPSocket.h"
#if !defined(LF_IMPL_OPAQUE_OPTIMIZE)
#include "UDPSocketWindows.h"
#endif
#include "Core/Common/Assert.h"
#include "Core/String/String.h"
#include "Core/Platform/Atomic.h" 
#include "Core/Net/NetFramework.h"
#include "Core/Utility/ErrorCore.h"

#if defined(LF_OS_WINDOWS)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <mstcpip.h>

#pragma comment(lib, "Ws2_32.lib")

namespace lf {

const SizeT LF_MAX_MTU = 2048;

// Create a socket
// @param af -- Address Family Specification -
//      AF_UNSPEC -- Unspecified
//      AF_INET   -- IPv4
//      AF_INET6  -- IPv6
//      AF_IRDA   -- Infrared
//      AF_BTH    -- Bluetooth
//
// @param type
//      SOCK_STREAM -- TCP w/ IPv4 or IPv6
//      SOCK_DGRAM  -- UDP w/ IPv4 or IPv6
//
// @param protocol
//      IPPROTO_TCP
//      IPPROTO_UDP
//
//

// sockaddr_in
//   ushort    : 2 : 2 : 0  : family
//   ushort    : 2 : 2 : 2  : port
//   in_addr   : 4 : 4 : 4  : address
//   char[8]   : 8 : 1 : 8  : 
//
// sockaddr_in6
//   ushort    : 2 : 2 : 0  : family
//   ushort    : 2 : 2 : 2  : port
//   ulong     : 4 : 4 : 4  : flow information
//   in6_addr  : 16 : 2 : 8 : 
//   ulong     : 4 : 4 : 24 : 

// [ OK] IPV4 => IPV4
// [ OK] IPV6 => IPV6
// [ OK] IPV4 => IPV6-MappedIPV4
// [BAD] IPV4 => IPV6
// [BAD] IPV6 => IPV4
// [BAD] IPV6-MappedIPV4 => IPV6

LF_IMPL_OPAQUE(UDPSocketWindows)::LF_IMPL_OPAQUE(UDPSocketWindows)()
: mSocket(INVALID_SOCKET)
, mProtocol(NetProtocol::INVALID_ENUM)
, mBoundPort(0)
, mReceiving(0)
{
    LF_STATIC_ASSERT(sizeof(sockaddr_in6::sin6_addr) == sizeof(IPv6EndPoint::mAddress));
    LF_STATIC_ASSERT(sizeof(sockaddr_in::sin_addr) == sizeof(IPv4EndPoint::mAddress));
    LF_STATIC_ASSERT(sizeof(sockaddr_in::sin_addr.s_addr) == sizeof(IPv4EndPoint::mAddress.mWord));
}
LF_IMPL_OPAQUE(UDPSocketWindows)::LF_IMPL_OPAQUE(UDPSocketWindows)(LF_IMPL_OPAQUE(UDPSocketWindows) && other)
: mSocket(other.mSocket)
, mProtocol(other.mProtocol)
, mBoundPort(other.mBoundPort)
, mReceiving(other.mReceiving)
{
    other.mSocket = INVALID_SOCKET;
    other.mProtocol = NetProtocol::INVALID_ENUM;
    other.mBoundPort = 0;
#if !defined(LF_FINAL)
    other.mReceiving = 0;
#endif
}
LF_IMPL_OPAQUE(UDPSocketWindows)::~LF_IMPL_OPAQUE(UDPSocketWindows)()
{
    Close();
}

LF_IMPL_OPAQUE(UDPSocketWindows)& LF_IMPL_OPAQUE(UDPSocketWindows)::operator=(LF_IMPL_OPAQUE(UDPSocketWindows) && other)
{
    if (this == &other)
    {
        return *this;
    }

    mSocket = other.mSocket;
    mProtocol = other.mProtocol;
    mBoundPort = other.mBoundPort;
    other.mSocket = INVALID_SOCKET;
    other.mProtocol = NetProtocol::INVALID_ENUM;
    other.mBoundPort = 0;
#if !defined(LF_FINAL)
    mReceiving = other.mReceiving;
    other.mReceiving = 0;
#endif
    return *this;
}

bool LF_IMPL_OPAQUE(UDPSocketWindows)::Create(NetProtocol::Value protocol)
{
    if (protocol != NetProtocol::NET_PROTOCOL_IPV4_UDP && protocol != NetProtocol::NET_PROTOCOL_IPV6_UDP && protocol != NetProtocol::NET_PROTOCOL_UDP)
    {
        return false;
    }

    if (mSocket != INVALID_SOCKET)
    {
        return false;
    }

    mSocket = socket(protocol == NetProtocol::NET_PROTOCOL_IPV4_UDP ? AF_INET : AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
    if (mSocket == INVALID_SOCKET)
    {
        LogSocketOperationFailure("socket");
        return false;
    }

    DWORD value = FALSE;
    if (protocol == NetProtocol::NET_PROTOCOL_UDP)
    {
        value = FALSE;
        if (setsockopt(mSocket, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<char*>(&value), sizeof(DWORD)) == SOCKET_ERROR)
        {
            LogSocketOperationFailure("setsocketopt -- IPPROTO_IPV6 - IPV6_V6ONLY - FALSE");
            if (closesocket(mSocket) != 0)
            {
                LogSocketOperationFailure("closesocket");
            }
            mSocket = INVALID_SOCKET;
            return false;
        }
    }

    mProtocol = protocol;
    return true;
}

bool LF_IMPL_OPAQUE(UDPSocketWindows)::Close()
{
    AssertEx(!IsAwaitingReceive(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    bool success = true;
    if (mSocket != INVALID_SOCKET)
    {
        if (closesocket(mSocket) != 0)
        {
            LogSocketOperationFailure("closesocket");
            success = false;
        }
        mSocket = INVALID_SOCKET;
        mProtocol = NetProtocol::INVALID_ENUM;
        mBoundPort = 0;
    }
    return success;
}

bool LF_IMPL_OPAQUE(UDPSocketWindows)::Bind(UInt16 port)
{
    if (mSocket == INVALID_SOCKET)
    {
        return false;
    }

    sockaddr_storage addr;
    ZeroMemory(&addr, sizeof(addr));
    int addrSize = 0;

    if (mProtocol == NetProtocol::NET_PROTOCOL_IPV6_UDP || mProtocol == NetProtocol::NET_PROTOCOL_UDP)
    {
        sockaddr_in6* v6 = reinterpret_cast<sockaddr_in6*>(&addr);
        v6->sin6_family = AF_INET6;
        v6->sin6_port = htons(port);
        v6->sin6_flowinfo = 0;
        // bind to ipv6 localhost
        if (inet_pton(AF_INET6, "::", &v6->sin6_addr) != TRUE)
        {
            LogSocketOperationFailure("inet_pton");
            return false;
        }
        addrSize = sizeof(sockaddr_in6);
    }
    else if (mProtocol == NetProtocol::NET_PROTOCOL_IPV4_UDP)
    {
        sockaddr_in* v4 = reinterpret_cast<sockaddr_in*>(&addr);
        v4->sin_family = AF_INET;
        v4->sin_port = htons(port);
        v4->sin_addr.s_addr = htons(INADDR_ANY);
        addrSize = sizeof(sockaddr_in);
    }
    else
    {
        CriticalAssertMsgEx("Unexpected network protocol for UDPSocket", LF_ERROR_MISSING_IMPLEMENTATION, ERROR_API_CORE)
    }

    if (bind(mSocket, reinterpret_cast<sockaddr*>(&addr), addrSize) == SOCKET_ERROR)
    {
        LogSocketOperationFailure("bind");
        return false;
    }

    mBoundPort = port;
    return true;
}

bool LF_IMPL_OPAQUE(UDPSocketWindows)::ReceiveFrom(ByteT* outBytes, SizeT& inOutBytes, IPEndPointAny& outEndPoint)
{
    if (outBytes == nullptr || inOutBytes == 0)
    {
        return false;
    }

    AssertEx(!IsAwaitingReceive(), LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);

    sockaddr_storage sender;
    int senderAddrSize = sizeof(sender);

    AtomicIncrement32(&mReceiving);
    int result = recvfrom(mSocket, reinterpret_cast<char*>(outBytes), static_cast<int>(inOutBytes), 0, reinterpret_cast<sockaddr*>(&sender), &senderAddrSize);
    AtomicDecrement32(&mReceiving);
    inOutBytes = 0;
    if (result == SOCKET_ERROR)
    {
        LogSocketOperationFailure("recvfrom");
        return false;
    }
    CriticalAssertEx(result >= 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // We expect the result to be >= 0 at this point.

    if (sender.ss_family == AF_INET)
    {
        sockaddr_in* v4In = reinterpret_cast<sockaddr_in*>(&sender);
        IPv4EndPoint* v4Out = reinterpret_cast<IPv4EndPoint*>(&outEndPoint);

        v4Out->mAddressFamily = NetAddressFamily::NET_ADDRESS_FAMILY_IPV4;
        v4Out->mPort = v4In->sin_port;
        v4Out->mAddress.mWord = v4In->sin_addr.s_addr;
        inOutBytes = static_cast<SizeT>(result);
        return true;
    }
    else if (sender.ss_family == AF_INET6)
    {
        sockaddr_in6* v6In = reinterpret_cast<sockaddr_in6*>(&sender);
        IPv6EndPoint* v6Out = reinterpret_cast<IPv6EndPoint*>(&outEndPoint);

        v6Out->mAddressFamily = NetAddressFamily::NET_ADDRESS_FAMILY_IPV6;
        v6Out->mPort = v6In->sin6_port;
        memcpy(&v6Out->mAddress.mBytes[0], &v6In->sin6_addr.u.Byte[0], 16);
        inOutBytes = static_cast<SizeT>(result);
        return true;
    }
    return false; // failed: Unsupported address family
}

bool LF_IMPL_OPAQUE(UDPSocketWindows)::SendTo(const ByteT* bytes, SizeT& inOutBytes, const IPEndPointAny& endPoint)
{
    if (bytes == 0 || inOutBytes == 0)
    {
        return false;
    }
    // todo: Calculate an actual MTU
    ReportBug(inOutBytes <= LF_MAX_MTU); // Greater than 'MTU', sanity check for now. 
    if (inOutBytes > LF_MAX_MTU)
    {
        return false;
    }

    sockaddr_storage receiver;
    ZeroMemory(&receiver, sizeof(receiver));
    int receiverSize = 0;

    if (endPoint.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV6)
    {
        if (mProtocol == NetProtocol::NET_PROTOCOL_IPV4_UDP)
        {
            LogSocketError("SendTo", "Cannot send to IPV6 address family as the socket has been created for the IPV6 address family.");
            return false;
        }

        sockaddr_in6* v6 = reinterpret_cast<sockaddr_in6*>(&receiver);
        const IPv6EndPoint* v6EndPoint = reinterpret_cast<const IPv6EndPoint*>(&endPoint);
        v6->sin6_family = AF_INET6;
        v6->sin6_port = v6EndPoint->mPort; // The port should be in network byte order already.
        v6->sin6_flowinfo = 0;
        v6->sin6_scope_id = 0; // todo: (Nathan) What is scope_id
        v6->sin6_scope_struct = { 0 }; // todo: (Nathan) what is scope_struct
        memcpy(&v6->sin6_addr.u.Byte[0], &v6EndPoint->mAddress.mBytes[0], sizeof(v6EndPoint->mAddress.mBytes));
        receiverSize = sizeof(sockaddr_in6);
    }
    else if(endPoint.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4)
    {
        if (mProtocol == NetProtocol::NET_PROTOCOL_IPV6_UDP)
        {
            LogSocketError("SendTo", "Cannot send to IPV4 address family as the socket has been created for the IPV6 address family.");
            return INVALID;
        }
        else if (mProtocol == NetProtocol::NET_PROTOCOL_UDP)
        {
            sockaddr_in6* v6 = reinterpret_cast<sockaddr_in6*>(&receiver);
            const IPv4EndPoint* v4EndPoint = reinterpret_cast<const IPv4EndPoint*>(&endPoint);

            sockaddr_in v4map;
            v4map.sin_family = AF_INET;
            v4map.sin_port = v4EndPoint->mPort; // The port should be in network byte order already.
            v4map.sin_addr.s_addr = v4EndPoint->mAddress.mWord;

            SCOPE_ID scopeID = INETADDR_SCOPE_ID(reinterpret_cast<sockaddr*>(&v4map));
            IN6ADDR_SETV4MAPPED(v6, &v4map.sin_addr, scopeID, ntohs(v4EndPoint->mPort));

            receiverSize = sizeof(sockaddr_in6);
        }
        else
        {
            sockaddr_in* v4 = reinterpret_cast<sockaddr_in*>(&receiver);
            const IPv4EndPoint* v4EndPoint = reinterpret_cast<const IPv4EndPoint*>(&endPoint);

            v4->sin_family = AF_INET;
            v4->sin_port = v4EndPoint->mPort; // The port should be in network byte order already.
            v4->sin_addr.s_addr = v4EndPoint->mAddress.mWord;
            receiverSize = sizeof(sockaddr_in);
        }
    }
    else
    {
        CriticalAssertMsgEx("SendTo failed to sent to endPoint, Unknown endpoint address family.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return false;
    }

    int result = sendto(mSocket, reinterpret_cast<const char*>(bytes), static_cast<int>(inOutBytes), 0, reinterpret_cast<sockaddr*>(&receiver), receiverSize);
    inOutBytes = 0;
    if (result == SOCKET_ERROR)
    {
        LogSocketOperationFailure("sendto");
        return false;
    }
    CriticalAssertEx(result >= 0, LF_ERROR_BAD_STATE, ERROR_API_CORE); // We expect the result to be >= 0 at this point.
    inOutBytes = static_cast<SizeT>(result);
    return true;
}

UInt16 LF_IMPL_OPAQUE(UDPSocketWindows)::GetBoundPort() const
{
    Atomic32 port = AtomicLoad(&mBoundPort);
    if (port > 0)
    {
        return static_cast<UInt16>(port);
    }

    sockaddr_storage address;
    socklen_t addressLength = sizeof(address);
    if (getsockname(mSocket, reinterpret_cast<sockaddr*>(&address), &addressLength) == -1)
    {
        LogSocketOperationFailure("getsockname");
        return 0;
    }
    if (address.ss_family == AF_INET)
    {
        AtomicStore(&mBoundPort, reinterpret_cast<sockaddr_in*>(&address)->sin_port);
    }
    else if (address.ss_family == AF_INET6)
    {
        AtomicStore(&mBoundPort, reinterpret_cast<sockaddr_in6*>(&address)->sin6_port);
    }
    else
    {
        ReportBugMsgEx("Unexpected socket family.", LF_ERROR_INTERNAL, ERROR_API_CORE);
    }
    return static_cast<UInt16>(AtomicLoad(&mBoundPort));
}

bool LF_IMPL_OPAQUE(UDPSocketWindows)::IsAwaitingReceive() const
{
    return AtomicLoad(&mReceiving) > 0;
}

} // namespace lf

#endif // LF_OS_WINDOWS
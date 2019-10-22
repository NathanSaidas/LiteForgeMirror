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

#include "NetFramework.h"
#include "Core/Common/Assert.h"
#include "Core/String/StringCommon.h"
#include "Core/Utility/ErrorCore.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/StackTrace.h"
#if defined (LF_OS_WINDOWS)

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <Windows.h>
#include <WinSock2.h>
#include <WS2tcpip.h>
// #include <mstcpip.h>

#pragma comment(lib, "Ws2_32.lib")

namespace lf {

void SetCTitle(const char* title)
{
    SetConsoleTitleA(title);
}


WSADATA gWSAData;
bool gNetInitialized = false;

bool NetInitialize()
{
    LF_STATIC_ASSERT(sizeof(IPv4EndPoint) <= sizeof(IPEndPointAny));
    LF_STATIC_ASSERT(sizeof(IPv6EndPoint) <= sizeof(IPEndPointAny));
    LF_STATIC_ASSERT(alignof(IPv4EndPoint) == alignof(IPEndPointAny));
    LF_STATIC_ASSERT(alignof(IPv6EndPoint) == alignof(IPEndPointAny));
    LF_STATIC_ASSERT(sizeof(IPv6EndPoint::mAddress) == sizeof(IPEndPointAny::mPadding));
    LF_STATIC_ASSERT(sizeof(IPv4EndPoint::mAddress) <= sizeof(IPEndPointAny::mPadding));

    if (IsNetInitialized())
    {
        CriticalAssertMsgEx("Network is already initialized", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return false;
    }

    WORD version = MAKEWORD(2, 2);
    int result = WSAStartup(version, &gWSAData);
    switch (result)
    {
        case 0: gNetInitialized = true; return true;
        case WSASYSNOTREADY: gSysLog.Error(LogMessage("NetFrameworkWindows::NetInitialize failed. System is not ready."));
        case WSAVERNOTSUPPORTED: gSysLog.Error(LogMessage("NetFrameworkWindows::NetInitialize failed. The version requested is not supported."));
        case WSAEINPROGRESS: gSysLog.Error(LogMessage("NetFrameworkWindows::NetInitialize failed. A blocking Windows Socket operation is in progress."));
        case WSAEPROCLIM: gSysLog.Error(LogMessage("NetFrameworkWindows::NetInitialize failed. A limit of the number of tasks supported by the Windows Socket implementation has been reached."));
        case WSAEFAULT:gSysLog.Error(LogMessage("NetFrameworkWindows::NetInitialize failed. Invalid WSAData."));
        default:
            return false;
    }
}

bool NetShutdown()
{
    if (!IsNetInitialized())
    {
        CriticalAssertMsgEx("Network is not initialized and cannot cleanup.", LF_ERROR_INVALID_OPERATION, ERROR_API_CORE);
        return false;
    }

    if (WSACleanup() == SOCKET_ERROR)
    {
        gSysLog.Error(LogMessage("Failed to execute WSACleanup: Reason=") << GetNetworkErrorString(GetNetworkErrorCode()) << "(0x" << ToHexString(GetNetworkErrorCode()) << ")");
        return false;
    }
    gWSAData = WSADATA();
    gNetInitialized = false;
    return true;
}

bool IsNetInitialized()
{
    return gNetInitialized;
}


Int32 GetNetworkErrorCode()
{
    return WSAGetLastError();
}

#define LF_WSA_ERROR_STRING(error_) case error_: return #error_;
const char* GetNetworkErrorString(Int32 errorCode)
{
    switch (errorCode)
    {
        LF_WSA_ERROR_STRING(WSANOTINITIALISED); // WSAStartup must be called
        LF_WSA_ERROR_STRING(WSAENETDOWN);       // Network Subsystem failed
        LF_WSA_ERROR_STRING(WSAEACCES);
        LF_WSA_ERROR_STRING(WSAEADDRINUSE);     // 
        LF_WSA_ERROR_STRING(WSAEADDRNOTAVAIL);
        LF_WSA_ERROR_STRING(WSAEFAULT);
        LF_WSA_ERROR_STRING(WSAEINPROGRESS);
        LF_WSA_ERROR_STRING(WSAEINVAL);
        LF_WSA_ERROR_STRING(WSAENOBUFS);
        LF_WSA_ERROR_STRING(WSAENOTSOCK);
        LF_WSA_ERROR_STRING(WSAEAFNOSUPPORT);
        LF_WSA_ERROR_STRING(WSAEMFILE);
        LF_WSA_ERROR_STRING(WSAEPROTOTYPE);
        LF_WSA_ERROR_STRING(WSAEPROTONOSUPPORT);
        LF_WSA_ERROR_STRING(WSAESOCKTNOSUPPORT);
        LF_WSA_ERROR_STRING(WSAEWOULDBLOCK);
        LF_WSA_ERROR_STRING(WSAEISCONN);
        LF_WSA_ERROR_STRING(WSAEOPNOTSUPP);
        LF_WSA_ERROR_STRING(WSAEMSGSIZE);
        LF_WSA_ERROR_STRING(WSAEINTR);
        default:
            return "Unknown socket error.";
    }
}
#undef LF_WSA_ERROR_STRING

void LogSocketOperationFailure(const char* operation)
{
    int code = GetNetworkErrorCode();
    auto msg = (LogMessage("WSA socket operation \"") << operation << "\" failed. Error=" << GetNetworkErrorString(code) << "(0x" << ToHexString(code) << ")");

    ScopedStackTrace trace;
    CaptureStackTrace(trace, 45);

    for (size_t i = 0; i < trace.frameCount; ++i)
    {
        msg << "\n  " << trace.frames[i].function;
    }
    gSysLog.Error(msg);
}

void LogSocketError(const char* operation, const char* message)
{
    auto msg = LogMessage("Socket error during operation \"") << operation << "\". Error=" << message << "\n";
    ScopedStackTrace trace;
    CaptureStackTrace(trace, 45);

    for (size_t i = 0; i < trace.frameCount; ++i)
    {
        msg << "\n  " << trace.frames[i].function;
    }
    gSysLog.Error(msg);
}

IPv4EndPoint IPV4(const char* address, UInt16 port)
{
    IPv4EndPoint endPoint;
    IPV4(reinterpret_cast<IPEndPointAny&>(endPoint), address, port);
    return endPoint;
}

IPv6EndPoint IPV6(const char* address, UInt16 port)
{
    IPv6EndPoint endPoint;
    IPV6(reinterpret_cast<IPEndPointAny&>(endPoint), address, port);
    return endPoint;
}

bool IPV4(IPEndPointAny& endPoint, const char* address, UInt16 port)
{
    IPv4EndPoint* ipv4 = reinterpret_cast<IPv4EndPoint*>(&endPoint);
    int result = inet_pton(AF_INET, address, &ipv4->mAddress);
    if (result == 1)
    {
        ipv4->mAddressFamily = NetAddressFamily::NET_ADDRESS_FAMILY_IPV4;
        ipv4->mPort = htons(port);
        return true;
    }
    else if (result == 0)
    {
        return false; // Invalid string format. must be IPv4 or IPv6
    }
    else if (result == -1)
    {
        LogSocketOperationFailure("inet_pton");
    }
    return false;
}

bool IPV6(IPEndPointAny& endPoint, const char* address, UInt16 port)
{
    IPv6EndPoint* ipv6 = reinterpret_cast<IPv6EndPoint*>(&endPoint);
    
    int result = inet_pton(AF_INET6, address, &ipv6->mAddress);
    if (result == 1)
    {
        ipv6->mAddressFamily = NetAddressFamily::NET_ADDRESS_FAMILY_IPV6;
        ipv6->mPort = htons(port);
        return true;
    }
    else if (result == 0)
    {
        return false; // Invalid string format. must be IPv4 or IPv6
    }
    else if (result == -1)
    {
        LogSocketOperationFailure("inet_pton");
    }
    return false;
}

bool IPEmpty(const IPv4EndPoint& endPoint)
{
    return InvalidEnum(static_cast<NetAddressFamily::Value>(endPoint.mAddressFamily));
}

bool IPEmpty(const IPv6EndPoint& endPoint)
{
    return InvalidEnum(static_cast<NetAddressFamily::Value>(endPoint.mAddressFamily));
}

bool IPEmpty(const IPEndPointAny& endPoint)
{
    return InvalidEnum(static_cast<NetAddressFamily::Value>(endPoint.mAddressFamily));
}

bool IPCast(const IPEndPointAny& endPoint, IPv4EndPoint& outEndPoint)
{
    if (endPoint.mAddressFamily != NetAddressFamily::NET_ADDRESS_FAMILY_IPV4)
    {
        return false;
    }
    outEndPoint.mAddressFamily = endPoint.mAddressFamily;
    outEndPoint.mPort = endPoint.mPort;
    outEndPoint.mAddress.mWord = endPoint.mPadding.mWord[0];
    return true;
}
bool IPCast(const IPEndPointAny& endPoint, IPv6EndPoint& outEndPoint)
{
    if (endPoint.mAddressFamily != NetAddressFamily::NET_ADDRESS_FAMILY_IPV6)
    {
        return false;
    }
    outEndPoint.mAddressFamily = endPoint.mAddressFamily;
    outEndPoint.mPort = endPoint.mPort;
    memcpy(outEndPoint.mAddress.mBytes, endPoint.mPadding.mBytes, sizeof(outEndPoint.mAddress));
    return true;
}
bool IPCast(const IPv4EndPoint& endPoint, IPEndPointAny& outEndPoint)
{
    outEndPoint = IPEndPointAny();
    outEndPoint.mAddressFamily = endPoint.mAddressFamily;
    outEndPoint.mPort = endPoint.mPort;
    outEndPoint.mPadding.mWord[0] = endPoint.mAddress.mWord;
    return true;
}
bool IPCast(const IPv6EndPoint& endPoint, IPEndPointAny& outEndPoint)
{
    outEndPoint = IPEndPointAny();
    outEndPoint.mAddressFamily = endPoint.mAddressFamily;
    outEndPoint.mPort = endPoint.mPort;
    memcpy(outEndPoint.mPadding.mBytes, endPoint.mAddress.mBytes, sizeof(outEndPoint.mPadding));
    return true;
}

String IPToString(const IPEndPointAny& endPoint)
{
    if (IPEmpty(endPoint))
    {
        return String();
    }

    switch (endPoint.mAddressFamily)
    {
        case NetAddressFamily::NET_ADDRESS_FAMILY_IPV4:
        {
            in_addr addr;
            char buffer[64];

            memset(buffer, 0, sizeof(buffer));
            memcpy(&addr, endPoint.mPadding.mBytes, sizeof(addr));

            if (inet_ntop(AF_INET, &addr, buffer, sizeof(buffer)) == NULL)
            {
                return String();
            }
            return String(buffer) + ":" + ToString(endPoint.mPort);
        } break;
        case NetAddressFamily::NET_ADDRESS_FAMILY_IPV6:
        {
            in_addr6 addr;
            char buffer[64];

            memset(buffer, 0, sizeof(buffer));
            memcpy(&addr, endPoint.mPadding.mBytes, sizeof(addr));

            if (inet_ntop(AF_INET6, &addr, buffer, sizeof(buffer)) == NULL)
            {
                return String();
            }
            return String(buffer) + ":" + ToString(endPoint.mPort);
        } break;
    }
    return String();
}

} // namespace lf
#endif // LF_OS_WINDOWS
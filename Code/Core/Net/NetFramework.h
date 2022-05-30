// ********************************************************************
// Copyright (c) 2019-2020 Nathan Hanlan
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
#pragma once
#include "Core/Net/NetTypes.h"

namespace lf {
class String;

LF_CORE_API void SetCTitle(const char* title);

// **********************************
// This must be called before using any networking functions/objects as it will
// initialize the Network Sub Systems
// **********************************
LF_CORE_API bool NetInitialize();
// **********************************
// This must be called on shutdown to cleanup any networking sub systems.
// **********************************
LF_CORE_API bool NetShutdown();
// **********************************
// @return Returns true if the networking sub systems have been initialized.
// **********************************
LF_CORE_API bool IsNetInitialized();

// **********************************
// @return Returns the platform specific network error code.
// **********************************
LF_CORE_API Int32 GetNetworkErrorCode();
// **********************************
// @return Returns the platform specific error code in string format.
// **********************************
LF_CORE_API const char* GetNetworkErrorString(Int32 errorCode);
// **********************************
// For use internally to log socket operation failures.
// **********************************
LF_CORE_API void LogSocketOperationFailure(const char* operation);
// **********************************
// For use internally to log socket runtime errors.
// **********************************
LF_CORE_API void LogSocketError(const char* operation, const char* message);

// **********************************
// Constructs an IPv4Endpoint from an address/port
// @param address -- Must be a valid IPv4 address eg 0.0.0.0 or 127.0.0.1
// @param port    -- The port number of the end point.
// @return Returns an IPv4EndPoint usable with sockets
// **********************************
LF_CORE_API IPv4EndPoint IPV4(const char* address, UInt16 port);
// **********************************
// Constructs an IPv6Endpoint from an address/port
// @param address -- Must be a valid IPv6 address eg ::1 or 2001:0db8:85a3:0000:0000:8a2e:0370:7334
// @param port    -- The port number of the end point.
// @return Returns an IPv4EndPoint usable with sockets
// **********************************
LF_CORE_API IPv6EndPoint IPV6(const char* address, UInt16 port);
// **********************************
// Initializes the IPv4Endpoint from an address/port
// @param endPoint -- The endPoint to initialize as IPv4EndPoint
// @param address -- Must be a valid IPv4 address eg 0.0.0.0 or 127.0.0.1
// @param port    -- The port number of the end point.
// @return Returns true if the address was successfully parsed.
// **********************************
LF_CORE_API bool IPV4(IPEndPointAny& endPoint, const char* address, UInt16 port);
// **********************************
// Initializes the IPv4Endpoint from an address/port
// @param endPoint -- The endPoint to initialize as IPv4EndPoint
// @param address -- Must be a valid IPv6 address eg ::1 or 2001:0db8:85a3:0000:0000:8a2e:0370:7334
// @param port    -- The port number of the end point.
// @return Returns true if the address was successfully parsed.
// **********************************
LF_CORE_API bool IPV6(IPEndPointAny& endPoint, const char* address, UInt16 port);

LF_CORE_API bool IPEmpty(const IPv4EndPoint& endPoint);
LF_CORE_API bool IPEmpty(const IPv6EndPoint& endPoint);
LF_CORE_API bool IPEmpty(const IPEndPointAny& endPoint);

LF_CORE_API bool IPCast(const IPEndPointAny& endPoint, IPv4EndPoint& outEndPoint);
LF_CORE_API bool IPCast(const IPEndPointAny& endPoint, IPv6EndPoint& outEndPoint);
LF_CORE_API bool IPCast(const IPv4EndPoint& endPoint, IPEndPointAny& outEndPoint);
LF_CORE_API bool IPCast(const IPv6EndPoint& endPoint, IPEndPointAny& outEndPoint);

LF_CORE_API String IPToString(const IPEndPointAny& endPoint);

LF_CORE_API UInt16 IPEndPointGetPort(const IPEndPointAny& endPoint);
LF_CORE_API UInt16 IPEndPointGetPort(const IPv4EndPoint& endPoint);
LF_CORE_API UInt16 IPEndPointGetPort(const IPv6EndPoint& endPoint);

LF_CORE_API bool IPIsLocal(const IPEndPointAny& endPoint);
LF_CORE_API bool IPIsLocal(const IPv4EndPoint& endPoint);
LF_CORE_API bool IPIsLocal(const IPv6EndPoint& endPoint);


}
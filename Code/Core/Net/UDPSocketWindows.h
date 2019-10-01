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

#include "Core/Common/API.h"
#include "Core/Common/Types.h"
#include "Core/Net/NetTypes.h"

namespace lf {

// **********************************
// Socket implementation using the WinSock2 api.
// **********************************
class LF_CORE_API LF_IMPL_OPAQUE(UDPSocketWindows)
{
public:
    LF_IMPL_OPAQUE(UDPSocketWindows)(const LF_IMPL_OPAQUE(UDPSocketWindows)&) = delete;
    LF_IMPL_OPAQUE(UDPSocketWindows)& operator=(const LF_IMPL_OPAQUE(UDPSocketWindows)&) = delete;
    LF_IMPL_OPAQUE(UDPSocketWindows)();
    LF_IMPL_OPAQUE(UDPSocketWindows)(LF_IMPL_OPAQUE(UDPSocketWindows) && other);
    ~LF_IMPL_OPAQUE(UDPSocketWindows)();
    LF_IMPL_OPAQUE(UDPSocketWindows)& operator=(LF_IMPL_OPAQUE(UDPSocketWindows) && other);
    bool Create(NetProtocol::Value protocol);
    bool Close();
    bool Bind(UInt16 port);
    bool ReceiveFrom(ByteT* outBytes, SizeT& inOutBytes, IPEndPointAny& outEndPoint);
    bool SendTo(const ByteT* bytes, SizeT& inOutBytes, const IPEndPointAny& endPoint);
    NetProtocol::Value GetProtocol() const { return mProtocol; }
    UInt16 GetBoundPort() const;
    bool IsAwaitingReceive() const; 
private:
    using SocketType = UIntPtrT;
    SocketType         mSocket;
    NetProtocol::Value mProtocol;
    volatile mutable Atomic32 mBoundPort;
    volatile Atomic32  mReceiving;
};

}
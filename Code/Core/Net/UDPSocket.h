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
#ifndef LF_CORE_UDP_SOCKET_H
#define LF_CORE_UDP_SOCKET_H

#include "Core/Net/NetTypes.h"
#include "Core/Memory/SmartPointer.h"

#if defined(LF_IMPL_OPAQUE_OPTIMIZE)
#if defined(LF_OS_WINDOWS)
#include "Core/Net/UDPSocketWindows.h"
#else
#error Missing platform implementation.
#endif
#else
namespace lf {
// **********************************
// Implementation of a network socket using the UDP protocol.
// **********************************
class LF_CORE_API UDPSocket
{
public:
    // **********************************
    // UDPSocket copy constructor is deleted.
    // **********************************
    UDPSocket(const UDPSocket&) = delete;
    // **********************************
    // UDPSocket copy assignment is deleted.
    // **********************************
    UDPSocket& operator=(const UDPSocket&) = delete;
    // **********************************
    // UDPSocketWindows default constructor initializes the socket
    // without allocating any internal resources.
    // **********************************
    UDPSocket();
    // **********************************
    // UDPSocket move constructor swaps 'this' with 'other' and 
    // resets 'other' back to default constructed state.
    // **********************************
    UDPSocket(UDPSocket&& other);
    // **********************************
    // UDPSocket destructor ensures all resources are freed.
    // **********************************
    ~UDPSocket();
    // **********************************
    // UDPSocket move assignment operator swaps 'this' with 'other
    // and resets 'other' back to the default constructed state.
    // **********************************
    UDPSocket& operator=(UDPSocket&& other);
    // **********************************
    // Verifies the 'protocol' is correct and the socket is in state in which
    // it can allocate internal resources to create an actual socket. It will
    // then proceed to create a socket. The socket can then be used to send
    // data but must be bound before receiving data.
    //
    // @param protocol -- The network protocol to use (UDP only)
    // @return Returns true if the socket resources we're successfully created.
    // **********************************
    bool Create(NetProtocol::Value protocol);
    // **********************************
    // Closes the socket releasing all resources. (It's advised to 'flush' the
    // socket so that it is no longer receiving before closing.)
    //
    // @return Returns true if the socket resources we're released without errors.
    // **********************************
    bool Close();
    // **********************************
    // Binds the socket to listen on a port. This should be used for receiving sockets
    // only, and must be used before calling ReceiveFrom
    // 
    // @param port -- The port to listen on
    // @return Returns true if the socket was bound without any errors.
    // **********************************
    bool Bind(UInt16 port);
    // **********************************
    // Blocks and receives data. The socket must be bound before it can receive data.
    // 
    // note: If this method returns false, it doesn't necessarilly mean that it failed,
    //       it could have simply been unblocked by a shutdown operation.
    //
    // @param outBytes   -- A pointer to memory that the received bytes will be written to
    // @param inOutBytes -- The number of bytes indicating the size of the buffer. If the
    //                   function succeeds the number of bytes received is written to this
    //                   argument.
    // @param outEndPoint -- IPEndPoint of the sender.
    // @return Returns true if there we're not error receiving bytes.
    bool ReceiveFrom(ByteT* outBytes, SizeT& inOutBytes, IPEndPointAny& outEndPoint);
    // **********************************
    // Sends bytes to the specified endPoint. The socket should not be bound for this to work
    // correctly.
    // 
    // @param bytes      -- A pointer to the memory that must be sent to the target endPoint
    // @param inOutBytes -- The number of bytes to be sent, when the function returns this holds
    //                   the value of the number of bytes actually sent.
    // @param endPoint   -- The target ip endpoint the data is to be sent to.
    // @return
    // **********************************
    bool SendTo(const ByteT* bytes, SizeT& inOutBytes, const IPEndPointAny& endPoint);
    // **********************************
    // @return Returns the protocol the socket is using
    // **********************************
    NetProtocol::Value GetProtocol() const;
    // **********************************
    // @return Returns the bound port of the socket. (Unbound sockets this return 0)
    // **********************************
    UInt16 GetBoundPort() const;
    // **********************************
    // @return Returns true if the socket is currently calling receive.
    // **********************************
    bool IsAwaitingReceive() const;
    // **********************************
    // Closes the socket under the assumption that it is currently awaiting to receive some data. (Blocking)
    // 
    // @return Returns true if the socket was successfully closed.
    // **********************************
    bool Shutdown();
private:
    using ImplType = LF_IMPL_OPAQUE(UDPSocket);
    using ImplPtr = TStrongPointer<ImplType>;
    ImplPtr mImpl;
};
} // namespace lf
#endif // LF_IMPL_OPAQUE_OPTIMIZED


#endif // LF_CORE_UDP_SOCKET_H
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
#include "Core/PCH.h"
#include "UDPSocket.h"
#if !defined(LF_IMPL_OPAQUE_OPTIMIZE)
#include "Core/Net/UDPSocketWindows.h"

namespace lf {

UDPSocket::UDPSocket()
: mImpl(LFNew<ImplType>())
{}
UDPSocket::UDPSocket(UDPSocket&& other)
: mImpl(std::move(other.mImpl))
{}
UDPSocket::~UDPSocket()
{}
UDPSocket& UDPSocket::operator=(UDPSocket&& other)
{
    mImpl = std::move(other.mImpl);
    return *this;
}

bool UDPSocket::Create(NetProtocol::Value protocol)
{
    return mImpl->Create(protocol);
}
bool UDPSocket::Close()
{
    return mImpl->Close();
}
bool UDPSocket::Bind(UInt16 port)
{
    return mImpl->Bind(port);
}

bool UDPSocket::ReceiveFrom(ByteT* outBytes, SizeT& inOutBytes, IPEndPointAny& outEndPoint)
{
    return mImpl->ReceiveFrom(outBytes, inOutBytes, outEndPoint);
}
bool UDPSocket::SendTo(const ByteT* bytes, SizeT& inOutBytes, const IPEndPointAny& endPoint)
{
    return mImpl->SendTo(bytes, inOutBytes, endPoint);
}

NetProtocol::Value UDPSocket::GetProtocol() const { return mImpl->GetProtocol(); }
UInt16 UDPSocket::GetBoundPort() const { return mImpl->GetBoundPort(); }
bool UDPSocket::IsAwaitingReceive() const { return mImpl->IsAwaitingReceive(); }

bool UDPSocket::Shutdown() { return mImpl->Shutdown(); }

} // namespace lf
#endif // LF_IMPL_OPAQUE_OPTIMIZE
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
#ifndef LF_CORE_NET_TRANSPORT_H
#define LF_CORE_NET_TRANSPORT_H

#include "Core/Net/NetTypes.h"
#include "Core/Memory/SmartPointer.h"


namespace lf {
class NetTransportConfig;
}

#if defined(LF_IMPL_OPAQUE_OPTIMIZE)
#include "Core/Net/NetTransportImpl.h"
#else
namespace lf {
class LF_CORE_API NetTransport
{
public:
    NetTransport(const NetTransport&) = delete;
    NetTransport& operator=(const NetTransport&) = delete;

    NetTransport();
    NetTransport(NetTransport&& other);
    ~NetTransport();

    NetTransport& operator=(NetTransport&& other);

    void Start(NetTransportConfig&& config);
    void Stop();

    bool IsRunning() const;

private:
    using ImplType = LF_IMPL_OPAQUE(NetTransport);
    using ImplPtr = TStrongPointer<ImplType>;
    ImplPtr mImpl;
};
} // namespace lf
#endif

#endif // LF_CORE_NET_TRANSPORT_H
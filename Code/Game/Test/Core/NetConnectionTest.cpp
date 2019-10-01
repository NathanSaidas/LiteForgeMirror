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

#include "Core/Test/Test.h"

#include "Core/Net/NetConnection.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/NetTransport.h"
#include "Core/Net/NetTransportHandler.h"
#include "Core/Net/PacketUtility.h"

#include "Core/Net/ConnectPacket.h"
#include "Core/Memory/PoolHeap.h"

namespace lf {

const SizeT TEST_PORT = 27015;
const char* TEST_IPV4_TARGET = "127.0.0.1";
const char* TEST_IPV6_TARGET = "::1";

class NetConnectionHandler : public NetTransportHandler
{
public:

protected:
    void OnInitialize() override
    {

    }

    void OnShutdown() override
    {

    }

    void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) override
    {
        // Connector

        NetPacketHeaderType::Value headerType = PacketUtility::GetHeaderType(bytes, byteLength);

        // Connection requests cannot be sent as any header but the 'base'
        if (headerType != NetPacketHeaderType::NET_PACKET_HEADER_TYPE_BASE)
        {
            return;
        }
        (sender);
        // todo: Create connection from this packet and return the encrypted data.

        
    }

    void OnUpdateFrame() override
    {

    }
};

class NetDisconnectionHandler : public NetTransportHandler
{
public:

protected:
    void OnInitialize() override
    {

    }

    void OnShutdown() override
    {

    }

    void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) override
    {
        (bytes);
        (byteLength);
        (sender);

        

    }

    void OnUpdateFrame() override
    {

    }
};





template<SizeT PacketSizeT>
class TPacketAllocator
{
public:
    using ValueT = TPacketData<PacketSizeT>;

private:


};

class PacketAllocator
{
public:
    PacketData* Allocate(const ByteT* bytes, SizeT byteLength);
    void Free(PacketData* packetData);

private:
};

class NetHeartbeatHandler : public NetTransportHandler
{
public:

protected:
    void OnInitialize() override
    {

    }

    void OnShutdown() override
    {

    }

    void OnReceivePacket(const ByteT* bytes, SizeT byteLength, const IPEndPointAny& sender) override
    {
        (bytes);
        (byteLength);
        (sender);
    }

    void OnUpdateFrame() override
    {

    }
};


class NetServer
{
public:
    void Start(NetTransportConfig&& config, const Crypto::RSAKey& serverKey)
    {
        mServerKey = serverKey;
        mTransport.Start(std::forward<NetTransportConfig&&>(config));
    }

    void Stop()
    {
        mTransport.Stop();
        mServerKey.Clear();
    }

private:
    NetTransport mTransport;
    Crypto::RSAKey mServerKey;
};

class NetClient
{
public:
    void Start(NetTransportConfig&& config, const Crypto::RSAKey& serverKey, const Crypto::RSAKey& clientKey, const Crypto::AESKey& dataKey)
    {
        mServerKey = serverKey;
        mClientKey = clientKey;
        mDataKey = dataKey;
        mTransport.Start(std::forward<NetTransportConfig&&>(config));
    }
    
    void Stop()
    {
        mClientKey.Clear();
    }

private:
    NetTransport mTransport;
    Crypto::RSAKey mServerKey;
    Crypto::RSAKey mClientKey;
    Crypto::AESKey mDataKey;
};

REGISTER_TEST(NetConnectionTest)
{
#if 0
    Crypto::RSAKey serverKey;
    Crypto::RSAKey clientKey;
    Crypto::AESKey dataKey;

    serverKey.GeneratePair(Crypto::RSA_KEY_2048);
    clientKey.GeneratePair(Crypto::RSA_KEY_2048);
    dataKey.Generate(Crypto::AES_KEY_256);

    NetTransportConfig config;
    config.SetAppId(NetConfig::NET_APP_ID);
    config.SetAppVersion(NetConfig::NET_APP_VERSION);
    config.SetPort(TEST_PORT);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT, LFNew<NetConnectionHandler>());
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT, LFNew<NetDisconnectionHandler>());
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT, LFNew<NetHeartbeatHandler>());

    NetServer server;
    server.Start(std::move(config), serverKey);

    NetClient client;
    client.Start(std::move(config), serverKey, clientKey, dataKey);
    
    // NetClient client;
    // client.Start();
    // 
    // server.Stop();
#endif


}

} // namespace lf
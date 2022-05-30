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

#include "Game/Test/Runtime/NetDriverTestUtils.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Utility.h"
#include "Runtime/Net/PacketSerializer.h"

namespace lf {

const SizeT TEST_PORT = 27015;
const char* TEST_IPV4_TARGET = "127.0.0.1";
const char* TEST_IPV6_TARGET = "::1";

template<SizeT NUM_CLIENTS>
struct ClientServerBasicConnectionStateMachine
{
    NetSecureServerDriver         mServer;
    NetSecureClientDriver         mClients[NUM_CLIENTS];

    ClientServerBasicConnectionStateMachine() {}
    ~ClientServerBasicConnectionStateMachine();

    bool Initialize();
    bool WaitConnections();
    bool WaitTimeouts();
    bool Idle(Float32 time);
    bool Shutdown();
};

template<SizeT NUM_CLIENTS>
ClientServerBasicConnectionStateMachine<NUM_CLIENTS>::~ClientServerBasicConnectionStateMachine()
{
    if (mServer.IsRunning())
    {
        mServer.Shutdown();
    }
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mClients); ++i)
    {
        if (mClients[i].IsRunning())
        {
            mClients[i].Shutdown();
        }
    }
}

template<SizeT NUM_CLIENTS>
bool ClientServerBasicConnectionStateMachine<NUM_CLIENTS>::Initialize()
{
    Crypto::RSAKey cert;
    cert.GeneratePair(Crypto::RSA_KEY_2048);
    IPEndPointAny ipAddress;
    IPCast(IPV4("127.0.0.1", 8080), ipAddress);

    NetServerDriverConfig config;
    config.mAppID = 0;
    config.mAppVersion = 0;
    config.mCertificate = &cert;
    config.mPort = 8080;

    bool serverInitialized;
    bool clientInitialized;
    TEST(serverInitialized = mServer.Initialize(config));
    if (!serverInitialized)
    {
        return false;
    }

    for (SizeT i = 0; i < LF_ARRAY_SIZE(mClients); ++i)
    {
        TEST(clientInitialized = mClients[i].Initialize(0, 0, ipAddress, cert));
        if (!clientInitialized)
        {
            return false;
        }
    }
    return true;
}

template<SizeT NUM_CLIENTS>
bool ClientServerBasicConnectionStateMachine<NUM_CLIENTS>::WaitConnections()
{
    ExecuteUpdate(10.0f, 60, [this] {
        SizeT numConnected = 0; 
        mServer.Update();
        for (SizeT i = 0; i < LF_ARRAY_SIZE(mClients); ++i)
        {
            mClients[i].Update();
            if (mClients[i].IsConnected())
            {
                ++numConnected;
            }
        }
        return numConnected != LF_ARRAY_SIZE(mClients); // Keep updating till all clients connected
    });
    return true;
}

template<SizeT NUM_CLIENTS>
bool ClientServerBasicConnectionStateMachine<NUM_CLIENTS>::WaitTimeouts()
{
    Float32 staggeringTimeout = 1.0f;
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mClients); ++i)
    {
        Float32 timeout = Max(mClients[i].GetTimeout(), mServer.GetTimeout()) + staggeringTimeout;
        mClients[i].SetHeartbeatDelta(timeout);
        staggeringTimeout += 1.0f;
    }


    ExecuteUpdate(100.0, 60, [this] {
        SizeT numDisconnected = 0;
        mServer.Update();
        for (SizeT i = 0; i < LF_ARRAY_SIZE(mClients); ++i)
        {
            mClients[i].Update();
            if (mClients[i].IsDisconnected())
            {
                ++numDisconnected;
            }
        }

        return numDisconnected != LF_ARRAY_SIZE(mClients);
    });
    return true;
}

template<SizeT NUM_CLIENTS>
bool ClientServerBasicConnectionStateMachine<NUM_CLIENTS>::Idle(Float32 time)
{
    ExecuteUpdate(time, 60, [this] {
        mServer.Update();
        for (SizeT i = 0; i < LF_ARRAY_SIZE(mClients); ++i)
        {
            mClients[i].Update();
        }
        return true;
    });
    return true;
}

template<SizeT NUM_CLIENTS>
bool ClientServerBasicConnectionStateMachine<NUM_CLIENTS>::Shutdown()
{
    TEST(mServer.GetConnectionCount() == 0);

    mServer.Shutdown();
    for (SizeT i = 0; i < LF_ARRAY_SIZE(mClients); ++i)
    {
        TEST(!mClients[i].IsConnected());
        TEST(mClients[i].IsDisconnected());
        mClients[i].Shutdown();
    }
    return true;
}

REGISTER_TEST(TestClientServerManyConnections, "Core.Net")
{
    NetTestInitializer init;
    ClientServerBasicConnectionStateMachine<24> tsm;

    gTestLog.Info(LogMessage("Initialize..."));
    TEST_CRITICAL(tsm.Initialize());
    gTestLog.Info(LogMessage("WaitConnections..."));
    TEST_CRITICAL(tsm.WaitConnections());
    gTestLog.Info(LogMessage("Idle..."));
    TEST_CRITICAL(tsm.Idle(5.0f));
    gTestLog.Info(LogMessage("WaitTimeouts..."));
    TEST_CRITICAL(tsm.WaitTimeouts());
    gTestLog.Info(LogMessage("Idle..."));
    TEST_CRITICAL(tsm.Idle(5.0f));
    gTestLog.Info(LogMessage("Shutdown..."));
    TEST_CRITICAL(tsm.Shutdown());
}

REGISTER_TEST(TestClientReachNoEndPoint, "Core.Net")
{
    NetTestInitializer init;

    Crypto::RSAKey cert;
    cert.GeneratePair(Crypto::RSA_KEY_2048);
    NetSecureClientDriver client;
    IPEndPointAny ipAddress;
    IPCast(IPV4("127.0.0.1", 8080), ipAddress);
    client.SetTimeout(3.0f);
    client.SetHeartbeatDelta(1.0f);

    TEST_CRITICAL(client.Initialize(0, 0, ipAddress, cert));
    ExecuteUpdate(10.0f, 60, [&client] {
        client.Update();
        return !client.IsConnected();
    });
    TEST(!client.IsConnected());
    client.Shutdown();
}

REGISTER_TEST(ClientSendDataTest, "Core.Net")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;

    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mServer = &server;
    tester.mClient = &client;
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    ExecuteUpdate(20.0f, 60, [&client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());

    bool messageReceived = false;
    const String message = "Example request message to be sent";
    NetDriver::Options options = (NetDriver::OPTION_ENCRYPT | NetDriver::OPTION_HMAC | NetDriver::OPTION_RELIABLE | NetDriver::OPTION_SIGNED);
    client.Send(NetDriver::MESSAGE_REQUEST, options, reinterpret_cast<const ByteT*>(message.CStr()), message.Size() + 1, 
        NetDriver::OnSendSuccess::Make([&messageReceived]() {
            gTestLog.Info(LogMessage("Successfully sent the message! Awaiting response..."));
        }),
        NetDriver::OnSendFailed::Make([&messageReceived]() {
            gTestLog.Info(LogMessage("Failed to send message!"));
        }));


    ExecuteUpdate(20.0f, 60, [&messageReceived, &tester] {
        tester.Update();
        return !messageReceived;
    });

    server.Shutdown();
    client.Shutdown();
}

// Test to make sure we can drop packets and still make a secure connection
REGISTER_TEST(StableSecureConnectionTest, "Core.Net")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;

    Crypto::RSAKey cert;
    cert.GeneratePair(Crypto::RSA_KEY_2048);
    IPEndPointAny ipAddress;
    IPCast(IPV4("127.0.0.1", 8080), ipAddress);

    NetSecureServerDriver server;
    NetSecureClientDriver client;
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    client.SetTimeout(10.0f);
    server.SetTimeout(10.0f);

    // todo: Ensure the ClientHello is Ack'ed
    // todo: Ensure the ServerHello is Ack'ed
    // todo: Add retransmission filter for ClientHello
    // todo: Add retransmission filter for ServerHello
    // todo: Add retransmission filter for Session Messages
    // todo: Add retransmission filter for Server Messages (on the client)

    StabilityTester tester;
    tester.mClient = &client;
    tester.mServer = &server;

    tester.DropServer();
    tester.DelayServer(2.500f);
    tester.DefaultServer();
    // tester.DropClient();
    tester.DefaultClient();
    tester.DelayClient(3.000f); // These should just drop
    tester.DefaultClient();
    tester.DefaultServer();
    tester.DefaultServer();
    tester.DefaultServer();
    tester.DelayServer(12.000f);
    tester.DefaultClient();
    tester.DefaultClient();
    tester.DefaultClient();
    tester.DefaultClient();
    tester.DefaultClient();
    tester.FilterPackets();

    ExecuteUpdate(30.0f, 60, [&tester] {
        tester.Update();
        return true;
    });

    server.Shutdown();
    client.Shutdown();

    LoggerMessage msg = LogMessage("ClientStats:");
    client.LogStats(msg);
    msg << "ServerStats:";
    server.LogStats(msg);
    gTestLog.Info(msg);


    // [  0.000]  Client       Send: Dropped...
    // [  3.000]  Client       Send: Delay...
    // [  6.000]  Client       Send: Received...
    // [  8.900]  Server Send [Ack]: Dropped...
    // [  9.000]  Client   Shutdown: No acks received in time
    // [  9.100]  Server       Send: Dropped...
    // [ 12.100]  Server       Send: Dropped...
    // [ 15.100]  Server       Send: Dropped...
    // [ 18.100]  Server      Close: No ack closing connection.
    
    // [  0.000]  Client       Send: Delay #3300ms...
    // [  3.000]  Client       Send: Received...
    // [  3.100]  Server        Ack: Received...
    // [  3.500]  Server       Send: Dropped...
    // [  6.500]  Server       Send: Delay...
    // [  9.500]  Server       Send: Received...
    // [  9.600]  Client        Ack: Dropped...
    // [ 12.500]  Server      Close: No ack received...      # We have a 3 second window to provide some sort of heartbeat to keep the connection alive.
}

// todo: Test that the client be DOSed with a malformed packet (a packet without proper signature)

} // namespace lf
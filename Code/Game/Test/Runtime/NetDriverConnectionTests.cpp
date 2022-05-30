// ********************************************************************
// Copyright (c) 2020 Nathan Hanlan
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
#include "Game/Test/Core/Net/NetTestUtils.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"
#include "Core/Utility/Utility.h"
#include "Core/String/StringCommon.h"
#include "Runtime/Net/PacketSerializer.h"

namespace lf {

// Test to make sure we can make a basic connection between client/server
REGISTER_TEST(ClientServerConnect_Test_000, "Core.Net.ConnectionTests")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;

    TEST(IPIsLocal(IPV6("::1", 2556)));

    StabilityTester tester;
    NetSecureServerDriver server;
    NetSecureClientDriver client;

    tester.mServer = &server;
    tester.mClient = &client;
    tester.FilterPackets();
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    ExecuteUpdate(5.0f, 60, [&server, &client] {
        server.Update();
        client.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);
    server.Shutdown();
    client.Shutdown();
}
// Test we can make a connection if the server drops 1 packet
REGISTER_TEST(ClientServerConnect_Test_001, "Core.Net.ConnectionTests")
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

    tester.DropServer(); // CLIENT_HELLO
    tester.FilterPackets();

    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);
    server.Shutdown();
    client.Shutdown();
}
// Test we can make a connection if the server drops 2 packets
REGISTER_TEST(ClientServerConnect_Test_002, "Core.Net.ConnectionTests")
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

    tester.DropServer(); // CLIENT_HELLO
    tester.DropServer(); // CLIENT_HELLO
    tester.FilterPackets();

    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);
    server.Shutdown();
    client.Shutdown();
}

// Test we can make a connection if the client drops the CLIENT_HELLO | ACK
REGISTER_TEST(ClientServerConnect_Test_003, "Core.Net.ConnectionTests")
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

    tester.DropClient(); // CLIENT_HELLO | ACK
    tester.FilterPackets();

    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);
    server.Shutdown();
    client.Shutdown();
}

// Test we can make a connection if the client drops the SERVER_HELLO
REGISTER_TEST(ClientServerConnect_Test_004, "Core.Net.ConnectionTests")
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

    tester.DropClient(); // CLIENT_HELLO | ACK
    tester.DropClient(); // SERVER_HELLO
    tester.FilterPackets();

    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);
    server.Shutdown();
    client.Shutdown();
}

// Test we can make a connection if the client drops the SERVER_HELLO 2 times
REGISTER_TEST(ClientServerConnect_Test_005, "Core.Net.ConnectionTests")
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

    tester.DropClient(); // CLIENT_HELLO | ACK
    tester.DropClient(); // SERVER_HELLO
    tester.DropClient(); // SERVER_HELLO
    tester.FilterPackets();

    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);
    server.Shutdown();
    client.Shutdown();
}

// Test we can't make a secure connection the client drops all the server's packets.
// However the server should've at least allocated a session
REGISTER_TEST(ClientServerConnect_Test_006, "Core.Net.ConnectionTests")
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

    tester.DropClient(); // CLIENT_HELLO | ACK
    tester.DropClient(); // SERVER_HELLO
    tester.DropClient(); // SERVER_HELLO
    tester.DropClient(); // SERVER_HELLO
    tester.FilterPackets();

    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(!client.IsConnected());
    TEST(server.GetConnectionsAccepted() == 1);
    server.Shutdown();
    client.Shutdown();
}

REGISTER_TEST(ClientServerConnect_Test_007, "Core.Net.ConnectionTests")
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

    tester.DropClient(); // CLIENT_HELLO | ACK
    tester.DelayClient(7.0f);
    tester.DelayClient(4.0f);
    tester.DelayClient(1.0f);
    tester.FilterPackets();

    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionsAccepted() == 1);
    server.Shutdown();
    client.Shutdown();
}

REGISTER_TEST(BasicServerTest, "Core.Net", TestFlags::TF_DISABLED)
{
    String tempDir = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), "\\Core\\Net\\"));
    TEST_CRITICAL(FileSystem::PathExists(tempDir) || FileSystem::PathCreate(tempDir));

    Crypto::RSAKey serverKey;
    TEST_CRITICAL(NetTestUtil::LoadPrivateKey(RSA_KEY_SERVER, serverKey));

    NetProtocol::Value ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    IPEndPointAny endPoint;

    String protocol = "ipv4";
    Int32 port = 25565;

    CmdLine::GetArgOption("test", "net_protocol", protocol);
    CmdLine::GetArgOption("test", "net_port", port);
    protocol = StrToLower(protocol);
    if (protocol == "ipv4")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    }
    else if (protocol == "ipv6")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV6_UDP;
    }
    else if (protocol == "any") 
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_UDP;
    }
    else 
    {
        gTestLog.Error(LogMessage("Invalid argument 'net_protocol'"));
        return;
    }

    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;

    NetSecureServerDriver server;
    StabilityTester tester;
    tester.mServer = &server;
    tester.FilterPackets();

    NetServerDriverConfig config;
    config.mAppID = 0;
    config.mAppVersion = 0;
    config.mCertificate = &serverKey;
    config.mPort = static_cast<UInt16>(port);
    config.mProtocol = ipProtocol;
    TEST(server.Initialize(config));

    ExecuteUpdate(999.0f, 60, [&server,&tester] {
        tester.Update();
        return true;
    });
    server.Shutdown();
}

REGISTER_TEST(BasicClientTest, "Core.Net", TestFlags::TF_DISABLED)
{
    String tempDir = FileSystem::PathResolve(FileSystem::PathJoin(TestFramework::GetTempDirectory(), "\\Core\\Net\\"));
    TEST_CRITICAL(FileSystem::PathExists(tempDir) || FileSystem::PathCreate(tempDir));

    Crypto::RSAKey serverKey;
    Crypto::RSAKey clientKey;
    TEST_CRITICAL(NetTestUtil::LoadPrivateKey(RSA_KEY_SERVER, serverKey));
    TEST(clientKey.LoadPublicKey(serverKey.GetPublicKey()));

    NetProtocol::Value ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    IPEndPointAny endPoint;

    String address = "127.0.0.1";
    String protocol = "ipv4";
    Int32 port = 25565;

    CmdLine::GetArgOption("test", "net_address", address);
    CmdLine::GetArgOption("test", "net_protocol", protocol);
    CmdLine::GetArgOption("test", "net_port", port);
    protocol = StrToLower(protocol);
    if (protocol == "ipv4")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
        TEST_CRITICAL(IPV4(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "ipv6")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV6_UDP;
        TEST_CRITICAL(IPV6(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "any")
    {
        TEST_CRITICAL("This option does not work for clients." != String());
    }
    else
    {
        gTestLog.Error(LogMessage("Invalid argument 'net_protocol'"));
        return;
    }

    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;

    NetClientDriverConfig config;
    config.mAppID = 0;
    config.mAppVersion = 0;
    config.mCertificate = &clientKey;
    config.mProtocol = ipProtocol;
    config.mEndPoint = endPoint;

    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mClient = &client;
    tester.FilterPackets();

    TEST(client.Initialize(config));
    

    ExecuteUpdate(20.0f, 60, [&client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });

    if (client.IsConnected())
    {
        ExecuteUpdate(999.0f, 60, [&client, &tester] {
            tester.Update();
            return true;
        });
    }

    client.Shutdown();
}

REGISTER_TEST(BasicUDPServerTest, "Core.Net", TestFlags::TF_DISABLED)
{
    const NetTestInitializer NET_INIT;

    NetProtocol::Value ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    IPEndPointAny endPoint;

    String address = "127.0.0.1";
    String protocol = "ipv4";
    Int32 port = 25565;

    CmdLine::GetArgOption("test", "net_address", address);
    CmdLine::GetArgOption("test", "net_protocol", protocol);
    CmdLine::GetArgOption("test", "net_port", port);
    protocol = StrToLower(protocol);
    if (protocol == "ipv4")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
        TEST_CRITICAL(IPV4(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "ipv6")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV6_UDP;
        TEST_CRITICAL(IPV6(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "any")
    {
        TEST_CRITICAL("This option does not work for clients." != String());
    }
    else
    {
        gTestLog.Error(LogMessage("Invalid argument 'net_protocol'"));
        return;
    }


    gTestLog.Info(LogMessage("Initializing listener..."));
    UDPSocket listener;
    TEST(listener.Create(ipProtocol));
    TEST(listener.Bind(static_cast<UInt16>(port)));

    
    for (SizeT i = 0; i < 100; ++i)
    {
        gTestLog.Info(LogMessage("Waiting on data..."));
        ByteT msg[1500];
        SizeT msgSize = sizeof(msg);
        IPEndPointAny target;
        if (!listener.ReceiveFrom(msg, msgSize, target))
        {
            break;
        }
        gTestLog.Info(LogMessage("Received ") << msgSize << " bytes...");

        if (listener.SendTo(msg, msgSize, target))
        {
            gTestLog.Info(LogMessage("Sent ") << msgSize << " bytes...");
        }

        // gTestLog.Info(LogMessage("Received ") << msgSize << " bytes from " << IPToString(target));
        // gTestLog.Info(LogMessage("Creating client..."));
        // UDPSocket client;
        // if (target.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV6)
        // {
        //     TEST(client.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP));
        // }
        // else if(target.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4)
        // {
        //     TEST(client.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP))
        // }
        // client.Bind(25565);
        // gTestLog.Info(LogMessage("Client connect..."));
        // if (!client.Connect(target))
        // {
        //     gTestLog.Info(LogMessage("Failed to connect 'client'"));
        //     client.Close();
        //     continue;
        // }
        // gTestLog.Info(LogMessage("Client send..."));
        // if (!client.Send(msg, msgSize))
        // {
        //     gTestLog.Info(LogMessage("Failed to send 'client'"));
        //     client.Close();
        //     continue;
        // }
        // 
        // client.Close();
    }
}

struct UDPTestState
{
    TVector<IPEndPointAny> mIncoming;
    SpinLock              mIncomingLock;
    ThreadFence           mIncomingEvent;
    UDPSocket             mSocket;
    volatile Atomic32     mRunning;
};

void UDPReceive(void* p)
{
    auto state = reinterpret_cast<UDPTestState*>(p);
    
    ByteT msg[1500];
    SizeT msgSize = sizeof(msg);
    IPEndPointAny target;
    gTestLog.Info(LogMessage("Waiting for connection..."));
    if (state->mSocket.ReceiveFrom(msg, msgSize, target))
    {
        gTestLog.Info(LogMessage("Received message from ") << IPToString(target));
        {
            ScopeLock lock(state->mIncomingLock);
            state->mIncoming.push_back(target);
        }
        state->mIncomingEvent.Signal();
    }

    if (state->mSocket.ReceiveFrom(msg, msgSize, target))
    {
        gTestLog.Info(LogMessage("Received final message from ") << IPToString(target));
    }
    AtomicStore(&state->mRunning, 0);
}

void UDPSend(UDPTestState* state)
{
    while (AtomicLoad(&state->mRunning) != 0)
    {
        state->mIncomingEvent.Wait(500);
        bool hasConnection = false;
        IPEndPointAny connection;
        {
            ScopeLock lock(state->mIncomingLock);
            if (!state->mIncoming.empty())
            {
                hasConnection = true;
                connection = state->mIncoming.back();
                state->mIncoming.erase(state->mIncoming.begin());
            }
        }
        if (hasConnection)
        {
            SleepCallingThread(1000);
            ByteT data[] = { 2, 3, 41, 1 };
            SizeT dataSize = sizeof(data);
            gTestLog.Info(LogMessage("Sending to connection...") << IPToString(connection));
            state->mSocket.SendTo(data, dataSize, connection);
            if (state->mSocket.IsAwaitingReceive())
            {
                state->mSocket.Shutdown();
            }
        }
    }
}

REGISTER_TEST(AdvancedUDPServerTest, "Core.Net", TestFlags::TF_DISABLED)
{
    const NetTestInitializer NET_INIT;

    NetProtocol::Value ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    IPEndPointAny endPoint;

    String address = "127.0.0.1";
    String protocol = "ipv4";
    Int32 port = 25565;

    CmdLine::GetArgOption("test", "net_address", address);
    CmdLine::GetArgOption("test", "net_protocol", protocol);
    CmdLine::GetArgOption("test", "net_port", port);
    protocol = StrToLower(protocol);
    if (protocol == "ipv4")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
        TEST_CRITICAL(IPV4(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "ipv6")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV6_UDP;
        TEST_CRITICAL(IPV6(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "any")
    {
        TEST_CRITICAL("This option does not work for clients." != String());
    }
    else
    {
        gTestLog.Error(LogMessage("Invalid argument 'net_protocol'"));
        return;
    }

    UDPTestState state;
    state.mIncomingEvent.Initialize();
    AtomicStore(&state.mRunning, 1);
    gTestLog.Info(LogMessage("Initializing listener..."));
    TEST(state.mSocket.Create(ipProtocol));
    TEST(state.mSocket.Bind(static_cast<UInt16>(port)));

    Thread thread;
    thread.Fork(UDPReceive, &state);
    UDPSend(&state);

    thread.Join();
    state.mIncomingEvent.Destroy();

}

REGISTER_TEST(BasicUDPClientTest, "Core.Net", TestFlags::TF_DISABLED)
{
    const NetTestInitializer NET_INIT;

    NetProtocol::Value ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    IPEndPointAny endPoint;

    String address = "127.0.0.1";
    String protocol = "ipv4";
    Int32 port = 25565;

    CmdLine::GetArgOption("test", "net_address", address);
    CmdLine::GetArgOption("test", "net_protocol", protocol);
    CmdLine::GetArgOption("test", "net_port", port);
    protocol = StrToLower(protocol);
    if (protocol == "ipv4")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
        TEST_CRITICAL(IPV4(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "ipv6")
    {
        ipProtocol = NetProtocol::NET_PROTOCOL_IPV6_UDP;
        TEST_CRITICAL(IPV6(endPoint, address.CStr(), static_cast<UInt16>(port)));
    }
    else if (protocol == "any")
    {
        TEST_CRITICAL("This option does not work for clients." != String());
    }
    else
    {
        gTestLog.Error(LogMessage("Invalid argument 'net_protocol'"));
        return;
    }

    UDPSocket socket;
    TEST(socket.Create(ipProtocol));

    gTestLog.Info(LogMessage("Sending data..."));
    String msgStr = "Hello from client!";
    SizeT msgLength = msgStr.Size();
    if (!socket.SendTo(reinterpret_cast<const ByteT*>(msgStr.CStr()), msgLength, endPoint))
    {
        gTestLog.Error(LogMessage("Failed to send data..."));
        socket.Close();
        return;
    }

    ByteT buf[1500];
    SizeT bufSize = sizeof(buf);
    IPEndPointAny serverEndPoint;
    if (socket.ReceiveFrom(buf, bufSize, serverEndPoint))
    {
        gTestLog.Info(LogMessage("Received ") << bufSize << " bytes from " << IPToString(serverEndPoint));
    }
    socket.Close();

}

}
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
#include "Runtime/Net/Controllers/NullMessageController.h"

namespace lf {

// Test that a client can send a message to server.
REGISTER_TEST(ClientServerMessage_Test_000, "Core.Net.MessageTests")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;
    const String MESSAGE_DATA = "Message text sent in a request!";


    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mServer = &server;
    tester.mClient = &client;

    tester.FilterPackets();
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));
    
    // Make a connection:
    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);

    bool messageReceived = false;
    TEST(client.Send(
        NetDriver::MESSAGE_REQUEST,
        GetStandardMessageOptions(),
        reinterpret_cast<const ByteT*>(MESSAGE_DATA.CStr()),
        MESSAGE_DATA.Size(),
        NetDriver::OnSendSuccess::Make([&messageReceived] { messageReceived = true; }),
        NetDriver::OnSendFailed()));


    ExecuteUpdate(20.0f, 60, [&messageReceived, &tester] {
        tester.Update();
        return !messageReceived;
    });
    TEST(messageReceived);
    server.Shutdown();
    client.Shutdown();
}

// Test that a client can send a message to server except server drops 1 packet
REGISTER_TEST(ClientServerMessage_Test_001, "Core.Net.MessageTests")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;
    const String MESSAGE_DATA = "Message text sent in a request!";


    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mServer = &server;
    tester.mClient = &client;
    
    tester.DropServer(NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.FilterPackets();
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    // Make a connection:
    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);

    bool messageReceived = false;
    client.Send(
        NetDriver::MESSAGE_REQUEST,
        GetStandardMessageOptions(),
        reinterpret_cast<const ByteT*>(MESSAGE_DATA.CStr()),
        MESSAGE_DATA.Size(),
        NetDriver::OnSendSuccess::Make([&messageReceived] { messageReceived = true; }),
        NetDriver::OnSendFailed());

    ExecuteUpdate(20.0f, 60, [&messageReceived, &tester] {
        tester.Update();
        return !messageReceived;
    });
    TEST(messageReceived);
    server.Shutdown();
    client.Shutdown();
}

// Test that a client can send a message to server except server drops 2 packets
REGISTER_TEST(ClientServerMessage_Test_002, "Core.Net.MessageTests")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;
    const String MESSAGE_DATA = "Message text sent in a request!";

    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mServer = &server;
    tester.mClient = &client;

    tester.DropServer(NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.DropServer(NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.FilterPackets();
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    // Make a connection:
    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);

    bool messageReceived = false;
    client.Send(
        NetDriver::MESSAGE_REQUEST,
        GetStandardMessageOptions(),
        reinterpret_cast<const ByteT*>(MESSAGE_DATA.CStr()),
        MESSAGE_DATA.Size(),
        NetDriver::OnSendSuccess::Make([&messageReceived] { messageReceived = true; }),
        NetDriver::OnSendFailed());

    ExecuteUpdate(20.0f, 60, [&messageReceived, &tester] {
        tester.Update();
        return !messageReceived;
    });
    TEST(messageReceived);
    server.Shutdown();
    client.Shutdown();
}

// Test that a client detects it failed to send a message because all packets were dropped.
REGISTER_TEST(ClientServerMessage_Test_003, "Core.Net.MessageTests")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;
    const String MESSAGE_DATA = "Message text sent in a request!";

    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mServer = &server;
    tester.mClient = &client;

    tester.DropServer(NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.DropServer(NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.DropServer(NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.FilterPackets();
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    // Make a connection:
    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);

    bool messageReceived = false;
    client.Send(
        NetDriver::MESSAGE_REQUEST,
        GetStandardMessageOptions(),
        reinterpret_cast<const ByteT*>(MESSAGE_DATA.CStr()),
        MESSAGE_DATA.Size(),
        NetDriver::OnSendSuccess::Make([&messageReceived] { messageReceived = true; }),
        NetDriver::OnSendFailed());

    ExecuteUpdate(20.0f, 60, [&messageReceived, &tester] {
        tester.Update();
        return !messageReceived;
    });
    TEST(!messageReceived);
    server.Shutdown();
    client.Shutdown();
}

// Test a server can resist duplicate packets
REGISTER_TEST(ClientServerMessage_Test_004, "Core.Net.MessageTests")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;
    const String MESSAGE_DATA = "Message text sent in a request!";

    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mServer = &server;
    tester.mClient = &client;

    tester.DelayServer(5.0f, NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.DropServer(NetPacketType::NET_PACKET_TYPE_REQUEST);
    tester.FilterPackets();
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    auto requestController = MakeConvertiblePtr<NullMessageController>();
    server.SetMessageController(NetDriver::MESSAGE_REQUEST, requestController);

    // Make a connection:
    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);

    bool messageReceived = false;
    client.Send(
        NetDriver::MESSAGE_REQUEST,
        GetStandardMessageOptions(),
        reinterpret_cast<const ByteT*>(MESSAGE_DATA.CStr()),
        MESSAGE_DATA.Size(),
        NetDriver::OnSendSuccess::Make([&messageReceived] { messageReceived = true; }),
        NetDriver::OnSendFailed());

    ExecuteUpdate(20.0f, 60, [&messageReceived, &tester] {
        tester.Update();
        return !messageReceived;
    });
    TEST(messageReceived);
    TEST(server.GetDroppedDuplicatePackets() == 1);
    server.Shutdown();
    client.Shutdown();
}

// Test we can handle message's as a Server
REGISTER_TEST(ClientServerMessage_Test_005, "Core.Net.MessageTests")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;
    const String MESSAGE_DATA = "Message text sent in a request!";

    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester tester;
    tester.mServer = &server;
    tester.mClient = &client;

    tester.FilterPackets();
    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    // Make a connection:
    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);

    NetConnectionAtomicPtr connection = server.FindConnection(client.GetSessionID());

    bool messageReceived = false;
    server.Send(
        NetDriver::MESSAGE_REQUEST,
        GetStandardMessageOptions(),
        reinterpret_cast<const ByteT*>(MESSAGE_DATA.CStr()),
        MESSAGE_DATA.Size(),
        connection,
        NetDriver::OnSendSuccess::Make([&messageReceived] { messageReceived = true; }),
        NetDriver::OnSendFailed());

    ExecuteUpdate(20.0f, 60, [&messageReceived, &tester] {
        tester.Update();
        return !messageReceived;
    });
    TEST(messageReceived);
    server.Shutdown();
    client.Shutdown();
}

} // namespace lf
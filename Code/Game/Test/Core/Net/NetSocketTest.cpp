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
#include "Core/Test/Test.h"

#include "Core/Net/UDPSocket.h"

#include "Core/Platform/Atomic.h"
#include "Core/Platform/Thread.h"
#include "Core/Platform/ThreadFence.h"

#include "Game/Test/Core/Net/NetTestUtils.h"

namespace lf {

static UInt16      TEST_PORT = 27015;
static const char* TEST_IPV4_TARGET = "127.0.0.1";
static const char* TEST_IPV6_TARGET = "::1";

// Test to make sure we can send data to a 'server' and then receive on the socket we sent. (I think this is NAT punchthrough)
REGISTER_TEST(UDPSocketIPV4SendReceive, "Core.Net")
{
    NetTestInitializer netInit;
    TEST_CRITICAL(netInit.mSuccess);

    ByteT dummyData[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    SizeT dummySize = sizeof(dummyData);
    IPEndPointAny target;
    TEST_CRITICAL(IPV4(target, TEST_IPV4_TARGET, TEST_PORT));

    struct Context
    {
        UDPSocket mClient;
        UDPSocket mServer;

        ThreadFence mServerSetupFence;
        ThreadFence mClientSetupFence;

        volatile Atomic32 mServerThreadStatus;
        volatile Atomic32 mClientThreadStatus;

        ThreadFence mServerThreadFence;
        ThreadFence mClientThreadFence;

    };

    Context context;
    context.mServerSetupFence.Initialize();
    context.mServerSetupFence.Set(true);
    context.mClientSetupFence.Initialize();
    context.mClientSetupFence.Set(true);
    context.mServerThreadFence.Initialize();
    context.mServerThreadFence.Set(true);
    context.mClientThreadFence.Initialize();
    context.mClientThreadFence.Set(true);

    AtomicStore(&context.mServerThreadStatus, 1);
    AtomicStore(&context.mClientThreadStatus, 1);

    UDPSocket* client = &context.mClient;
    TEST_CRITICAL(client->Create(NetProtocol::NET_PROTOCOL_IPV4_UDP));
    UDPSocket* server = &context.mServer;
    TEST_CRITICAL(server->Create(NetProtocol::NET_PROTOCOL_IPV4_UDP));
    TEST_CRITICAL(server->Bind(TEST_PORT));

    Thread serverThread;
    serverThread.Fork([](void* param)
    {
        ByteT bytes[4096];
        SizeT bytesRecevied = sizeof(bytes);
        IPEndPointAny endPoint;

        Context* context = reinterpret_cast<Context*>(param);
        context->mServerSetupFence.Set(false);
        if (!context->mServer.ReceiveFrom(bytes, bytesRecevied, endPoint))
        {
            AtomicStore(&context->mServerThreadStatus, 0);
            return;
        }

        if (context->mClientSetupFence.Wait(2500) != ThreadFence::WS_SUCCESS)
        {
            AtomicStore(&context->mServerThreadStatus, 0);
            return;
        }
        SleepCallingThread(100);

        UDPSocket connection;
        connection.Create(NetProtocol::NET_PROTOCOL_IPV4_UDP);
        if (!connection.SendTo(bytes, bytesRecevied, endPoint))
        {
            AtomicStore(&context->mServerThreadStatus, 0);
            return;
        }

        context->mServerThreadFence.Set(false);

    }, &context);

    TEST_CRITICAL(context.mServerSetupFence.Wait(2500) == ThreadFence::WS_SUCCESS);
    SleepCallingThread(100);

    TEST(client->SendTo(dummyData, dummySize, target));

    Thread clientThread;
    clientThread.Fork([](void* param)
    {
        ByteT bytes[4096];
        SizeT bytesRecevied = sizeof(bytes);
        IPEndPointAny endPoint;

        Context* context = reinterpret_cast<Context*>(param);
        context->mClientSetupFence.Set(false);
        if (!context->mClient.ReceiveFrom(bytes, bytesRecevied, endPoint))
        {
            AtomicStore(&context->mClientThreadStatus, 0);
            return;
        }

        context->mClientThreadFence.Set(false);

    }, &context);

    TEST(context.mServerThreadFence.Wait(2500) == ThreadFence::WS_SUCCESS);
    TEST(context.mClientThreadFence.Wait(2500) == ThreadFence::WS_SUCCESS);

    serverThread.Join();
    clientThread.Join();
    TEST(AtomicLoad(&context.mClientThreadStatus) == 1);
    TEST(AtomicLoad(&context.mServerThreadStatus) == 1);
}

REGISTER_TEST(UDPSocketIPV6SendReceive, "Core.Net")
{
    NetTestInitializer netInit;
    TEST_CRITICAL(netInit.mSuccess);

    ByteT dummyData[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    SizeT dummySize = sizeof(dummyData);
    IPEndPointAny target;
    TEST_CRITICAL(IPV6(target, TEST_IPV6_TARGET, TEST_PORT));

    struct Context
    {
        UDPSocket mClient;
        UDPSocket mServer;

        ThreadFence mServerSetupFence;
        ThreadFence mClientSetupFence;

        volatile Atomic32 mServerThreadStatus;
        volatile Atomic32 mClientThreadStatus;

        ThreadFence mServerThreadFence;
        ThreadFence mClientThreadFence;

    };

    Context context;
    context.mServerSetupFence.Initialize();
    context.mServerSetupFence.Set(true);
    context.mClientSetupFence.Initialize();
    context.mClientSetupFence.Set(true);
    context.mServerThreadFence.Initialize();
    context.mServerThreadFence.Set(true);
    context.mClientThreadFence.Initialize();
    context.mClientThreadFence.Set(true);

    AtomicStore(&context.mServerThreadStatus, 1);
    AtomicStore(&context.mClientThreadStatus, 1);

    UDPSocket* client = &context.mClient;
    TEST_CRITICAL(client->Create(NetProtocol::NET_PROTOCOL_IPV6_UDP));
    UDPSocket* server = &context.mServer;
    TEST_CRITICAL(server->Create(NetProtocol::NET_PROTOCOL_IPV6_UDP));
    TEST_CRITICAL(server->Bind(TEST_PORT));

    Thread serverThread;
    serverThread.Fork([](void* param)
    {
        ByteT bytes[4096];
        SizeT bytesRecevied = sizeof(bytes);
        IPEndPointAny endPoint;

        Context* context = reinterpret_cast<Context*>(param);
        context->mServerSetupFence.Set(false);
        if (!context->mServer.ReceiveFrom(bytes, bytesRecevied, endPoint))
        {
            AtomicStore(&context->mServerThreadStatus, 0);
            return;
        }

        if (context->mClientSetupFence.Wait(2500) != ThreadFence::WS_SUCCESS)
        {
            AtomicStore(&context->mServerThreadStatus, 0);
            return;
        }
        SleepCallingThread(100);

        UDPSocket connection;
        connection.Create(NetProtocol::NET_PROTOCOL_IPV6_UDP);
        if (!connection.SendTo(bytes, bytesRecevied, endPoint))
        {
            AtomicStore(&context->mServerThreadStatus, 0);
            return;
        }

        context->mServerThreadFence.Set(false);

    }, &context);

    TEST_CRITICAL(context.mServerSetupFence.Wait(2500) == ThreadFence::WS_SUCCESS);
    SleepCallingThread(100);

    TEST(client->SendTo(dummyData, dummySize, target));

    Thread clientThread;
    clientThread.Fork([](void* param)
    {
        ByteT bytes[4096];
        SizeT bytesRecevied = sizeof(bytes);
        IPEndPointAny endPoint;

        Context* context = reinterpret_cast<Context*>(param);
        context->mClientSetupFence.Set(false);
        if (!context->mClient.ReceiveFrom(bytes, bytesRecevied, endPoint))
        {
            AtomicStore(&context->mClientThreadStatus, 0);
            return;
        }

        context->mClientThreadFence.Set(false);

    }, &context);

    TEST(context.mServerThreadFence.Wait(2500) == ThreadFence::WS_SUCCESS);
    TEST(context.mClientThreadFence.Wait(2500) == ThreadFence::WS_SUCCESS);

    serverThread.Join();
    clientThread.Join();
    TEST(AtomicLoad(&context.mClientThreadStatus) == 1);
    TEST(AtomicLoad(&context.mServerThreadStatus) == 1);
}

} // namespace lf
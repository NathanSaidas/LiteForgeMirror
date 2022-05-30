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
#include "Core/Test/Test.h"
#include "Core/Math/Random.h"
#include "Runtime/Net/FileTransfer/FileTransferMessageController.h"
#include "Runtime/Net/FileTransfer/MemoryResourceLocator.h"
#include "Game/Test/Runtime/NetDriverTestUtils.h"

namespace lf {

static TVector<ByteT> RandomBytes(Int32& seed, SizeT numBytes)
{
    TVector<ByteT> bytes;
    bytes.resize(numBytes);
    for (SizeT i = 0; i < numBytes; ++i)
    {
        bytes[i] = static_cast<ByteT>(Random::Mod(seed, 255));
    }
    return bytes;
}

REGISTER_TEST(FileTransferMessageController_Test_000, "Core.Net")
{
    const NetTestInitializer NET_INIT;
    const SimpleConnectionConfig CONFIG;

    Int32 seed = 32932;

    TStrongPointer<MemoryResourceLocator> serverDB(LFNew<MemoryResourceLocator>());
    serverDB->WriteResource("testA", RandomBytes(seed, 4000), DateTime("06/04/2020"));
    serverDB->WriteResource("testB", RandomBytes(seed, 4000), DateTime("06/04/2020"));
    serverDB->WriteResource("testC", RandomBytes(seed, 4000), DateTime("06/04/2020"));


    NetSecureServerDriver server;
    NetSecureClientDriver client;
    StabilityTester       tester;
    tester.mServer = &server;
    tester.mClient = &client;
    tester.FilterPackets();

    TEST(CONFIG.Initialize(server));
    TEST(CONFIG.Initialize(client));

    auto messageController = MakeConvertiblePtr<FileTransferMessageController>();
    client.SetMessageController(NetDriver::MESSAGE_FILE_TRANSFER, messageController);
    messageController = MakeConvertiblePtr<FileTransferMessageController>();
    messageController->SetResourceLocator(serverDB);
    server.SetMessageController(NetDriver::MESSAGE_FILE_TRANSFER, messageController);

    // Make a connection:
    ExecuteUpdate(20.0f, 60, [&server, &client, &tester] {
        tester.Update();
        return !client.IsConnected();
    });
    TEST(client.IsConnected());
    TEST(server.GetConnectionCount() == 1);
    TEST(server.FindConnection(client.GetSessionID()) != NULL_PTR);

    ExecuteUpdate(60.0f, 60, [&tester] {
        tester.Update();
        return true;
    });

    server.Shutdown();
    client.Shutdown();
}

}
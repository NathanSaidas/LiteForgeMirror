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
#include "Engine/App/Application.h"
#include "Core/Concurrent/TaskScheduler.h"
#include "Core/IO/EngineConfig.h"
#include "Core/Platform/FileSystem.h"
#include "Core/Utility/CmdLine.h"
#include "Core/Utility/Log.h"

#include "Core/Net/Controllers/NetClientController.h"
#include "Core/Net/Controllers/NetConnectionController.h"
#include "Core/Net/Controllers/NetServerController.h"
#include "Core/Net/NetFramework.h"
#include "Core/Net/NetTransport.h"
#include "Core/Net/NetTransportConfig.h"
#include "Core/Net/TransportHandlers/ClientConnectionHandler.h"
#include "Core/Net/TransportHandlers/ServerConnectionHandler.h"

namespace lf {

static const UInt16 BASIC_PORT = 27015;
static const char* LOCAL_IPV4 = "127.0.0.1";
static const char* LOCAL_IPV6 = "::1";


// Declare our 'Client'
struct BasicClient
{
    NetTransport        mTransport;
    TaskScheduler       mTaskScheduler;
    NetClientController mClientController;
};

// Declare out 'Server'
struct BasicServer
{
    NetTransport        mTransport;
    TaskScheduler       mTaskScheduler;
    NetServerController mServerController;
    NetConnectionController mConnectionController;
};

// Heartbeat message: 
// Connected Header

class BasicNetApp : public Application
{
    DECLARE_CLASS(BasicNetApp, Application);
public:

    void OnStart() final
    {
        mIsClient = CmdLine::HasArgOption("net", "client");
        if (mIsClient)
        {
            gSysLog.Info(LogMessage("Running app as 'Client'"));
        }
        else
        {
            gSysLog.Info(LogMessage("Running app as 'Server'"));
        }
        gSysLog.Info(LogMessage("Temp Directory=") << GetTempDirectory());
        if (!LoadGenerateKey())
        {
            return;
        }

        if (!IsNetInitialized())
        {
            mIsNetOwner = true;
            if (!NetInitialize())
            {
                gSysLog.Error(LogMessage("Failed to initialize the network subsytstem."));
                return;
            }
        }
        

        if (mIsClient)
        {
            RunClient();
        }
        else
        {
            RunServer();
        }

        if (mIsNetOwner)
        {
            NetShutdown();
        }
    }

    String GetTempDirectory() const;
    bool LoadGenerateKey();
    void RunClient();
    void RunServer();

private:
    void LoadClient();
    void ShutdownClient();
    void LoadServer();
    void ShutdownServer();

    bool mIsNetOwner;
    bool mIsClient;
    Crypto::RSAKey mServerKey;

    BasicClient mClient;
    BasicServer mServer;
};
DEFINE_CLASS(BasicNetApp) { NO_REFLECTION; }


String BasicNetApp::GetTempDirectory() const
{

    String tempDirectory;
    if (GetConfig())
    {
        tempDirectory = FileSystem::PathJoin(GetConfig()->GetTempDirectory(), "BasicNetApp");
    }
    else
    {
        tempDirectory = FileSystem::PathJoin(FileSystem::PathGetParent(FileSystem::GetWorkingPath()), "Temp\\BasicNetApp");
    }

    if (!FileSystem::PathExists(tempDirectory) && !FileSystem::PathCreate(tempDirectory))
    {
        return String();
    }
    return tempDirectory;
}

bool BasicNetApp::LoadGenerateKey()
{
    String filename = FileSystem::PathJoin(GetTempDirectory(), "ServerKey.key");

    String keyString;
    File file;
    if (!file.Open(filename, FF_READ | FF_SHARE_READ, FILE_OPEN_EXISTING))
    {
        if (!file.Open(filename, FF_WRITE | FF_READ, FILE_OPEN_NEW))
        {
            gSysLog.Error(LogMessage("Failed to load/generate Server Key. ") << filename);
            return false;
        }
        gSysLog.Info(LogMessage("Generating Server Key..."));
        if (!mServerKey.GeneratePair(Crypto::RSA_KEY_2048))
        {
            return false;
        }
        gSysLog.Info(LogMessage("Server Key generated."));
        keyString = mServerKey.GetPrivateKey();
        file.Write(keyString.CStr(), keyString.Size());
        file.Close();
        return true;
    }

    gSysLog.Info(LogMessage("Loading server key..."));
    keyString.Resize(static_cast<SizeT>(file.GetSize()));
    file.Read(const_cast<char*>(keyString.CStr()), keyString.Size());
    file.Close();

    if (!mServerKey.LoadPrivateKey(keyString))
    {
        return false;
    }
    gSysLog.Info(LogMessage("Server key loaded."));
    return true;
}

void BasicNetApp::RunClient()
{
    LoadClient();
    gSysLog.Info(LogMessage("Waiting for server to respond..."));

    while (true)
    {
        if (!mClient.mTransport.IsRunning())
        {
            break;
        }

        if (mClient.mClientController.IsConnected())
        {
            gSysLog.Info(LogMessage("Client Connection Detected!"));
            break;
        }
        SleepCallingThread(0);
    }
    ShutdownClient();
    gSysLog.Info(LogMessage("Client complete."));
    SleepCallingThread(10000);
}

void BasicNetApp::RunServer()
{
    LoadServer();
    gSysLog.Info(LogMessage("Waiting for client to connect..."));

    while (true)
    {
        if (!mServer.mTransport.IsRunning())
        {
            break;
        }

        if (mServer.mConnectionController.FindConnection(4))
        {
            gSysLog.Info(LogMessage("Remote Connection Detected!"));
            break;
        }
        SleepCallingThread(0);
    }
    ShutdownServer();
    gSysLog.Info(LogMessage("Server complete."));
    SleepCallingThread(10000);
}

void BasicNetApp::LoadClient()
{
    TaskTypes::TaskSchedulerOptions options;
    options.mDispatcherSize = 20;
    options.mNumWorkerThreads = 2;

    mClient.mTaskScheduler.Initialize(options, true);
    Assert(mClient.mTaskScheduler.IsRunning());

    Crypto::RSAKey key = mServerKey;
    Assert(mClient.mClientController.Initialize(std::move(key)));

    IPEndPointAny localIP;
    Assert(IPCast(IPV6(LOCAL_IPV6, BASIC_PORT), localIP));

    NetTransportConfig config;
    config.SetAppId(NetConfig::NET_APP_ID);
    config.SetAppVersion(NetConfig::NET_APP_VERSION);
    config.SetPort(BASIC_PORT);
    config.SetEndPoint(localIP);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT,
        LFNew<ClientConnectionHandler>(
            MMT_GENERAL,
            &mClient.mTaskScheduler,
            &mClient.mClientController,
            nullptr,
            nullptr) // todo:
    );

    ServerConnectionHandler::PacketType packet;
    SizeT size = sizeof(packet.mBytes);
    const bool encoded = ConnectPacket::EncodePacket
    (
        packet.mBytes,
        size,
        mClient.mClientController.GetKey(),
        mClient.mClientController.GetServerKey(),
        mClient.mClientController.GetSharedKey(),
        mClient.mClientController.GetHMACKey(),
        mClient.mClientController.GetChallenge()
    );

    Assert(encoded);
    mClient.mTransport.Start(std::move(config), packet.mBytes, size);
}
void BasicNetApp::ShutdownClient()
{
    mClient.mTaskScheduler.Shutdown();
    mClient.mTransport.Stop();
    mClient.mClientController.Reset();
}
void BasicNetApp::LoadServer()
{
    TaskTypes::TaskSchedulerOptions options;
    options.mDispatcherSize = 20;
    options.mNumWorkerThreads = 2;
    mServer.mTaskScheduler.Initialize(options, true);
    Assert(mServer.mTaskScheduler.IsRunning());

    Crypto::RSAKey key = mServerKey;
    Assert(mServer.mServerController.Initialize(std::move(key)));

    NetTransportConfig config;
    config.SetAppId(NetConfig::NET_APP_ID);
    config.SetAppVersion(NetConfig::NET_APP_VERSION);
    config.SetPort(BASIC_PORT);
    config.SetTransportHandler(NetPacketType::NET_PACKET_TYPE_CONNECT,
        LFNew<ServerConnectionHandler>(
            MMT_GENERAL,
            &mServer.mTaskScheduler,
            &mServer.mConnectionController,
            &mServer.mServerController,
            nullptr,
            nullptr) // todo:
    );

    mServer.mTransport.Start(std::move(config));
}
void BasicNetApp::ShutdownServer()
{
    mServer.mTaskScheduler.Shutdown();
    mServer.mTransport.Stop();
    mServer.mConnectionController.Reset();
    mServer.mServerController.Reset();
}

}
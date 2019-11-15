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
#include "Core/Utility/Time.h"
#include "Core/Utility/ByteOrder.h"

#include "Core/Net/NetFramework.h"

#include "Core/Net/NetServerDriver.h"
#include "Core/Net/NetClientDriver.h"
#include "Core/Net/UDPSocket.h"

namespace lf {

static const UInt16 BASIC_PORT = 27015;
static const char* LOCAL_IPV4 = "127.0.0.1";
static const char* LOCAL_IPV6 = "::1";

// Heartbeat message: 
// Connected Header

struct ServerArgs
{
    // Number of clients to wait for
    Int32   mWaitClients;
    // The time for the server to keep its connection open.
    Float32 mWaitTime;
    // The amount of time a client gets before they are kicked.
    Float32 mClientLifetime;
};

struct ClientArgs
{
    Float32 mWaitTime;

    String mIPV4;

    String mIPV6;
};

// The goal of this application is to help exercise networking between client/server.
// 
// 1. Server
//     a) Run a server, await a client connection, await client disconnection, shutdown
//     b) Run a server, await client connection, shutdown
//     c) Run a server, await N client connections, await all client disconnects, shutdown
//     d) Run a server, await N client connections, shutdown
//     e) Run a server, await client connection, drop connection shutdown
//     f) Run a server, await client connection 3 connections, drop the 2nd one. shutdown
// 2. 
//     a) Run a client, connect to server, wait T seconds, disconnect.
//     b) Run a client, connect to server, wait T seconds (be disconnected)
//     c) Run a client, connect 
class BasicNetApp : public Application
{
    DECLARE_CLASS(BasicNetApp, Application);
public:

    enum Execution
    {
        BasicClient,
        BasicServer,
        Client,
        Server
    };

    void OnStart() final
    {
        gSysLog.SetLogLevel(LogLevel::LOG_DEBUG);
        gSysLog.Debug(LogMessage("Hello World"));

        String executionStr;
        Execution exec = BasicServer;
        if (!CmdLine::GetArgOption("net", "execution", executionStr))
        {
            gSysLog.Info(LogMessage("Missing net 'execute' argument, defaulting to BasicServer"));
        }

        executionStr = StrToLower(executionStr);
        if (executionStr == "basicclient") { exec = BasicClient; }
        else if (executionStr == "basicserver") { exec = BasicServer; }
        else if (executionStr == "client") { exec = Client; }
        else if (executionStr == "server") { exec = Server; }



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
        

        switch (exec)
        {
        case BasicClient: RunBasicClient(); break;
        case BasicServer: RunBasicServer(); break;
        case Client: RunClient(); break;
        case Server: RunServer(); break;
        default:
            gSysLog.Error(LogMessage("Unknown execution mode!"));
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
    void RunBasicClient();
    void RunBasicServer();

private:

    bool mIsNetOwner;
    Crypto::RSAKey mServerKey;
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
    Int32 port = BASIC_PORT;
    UInt16 appID = NetConfig::NET_APP_ID;
    UInt16 appVersion = NetConfig::NET_APP_VERSION;
    CmdLine::GetArgOption("net", "port", port);

    ClientArgs args;
    args.mWaitTime = 10.0f;

    CmdLine::GetArgOption("net", "client_WaitTime", args.mWaitTime);

    gSysLog.Info(LogMessage("Running client with config."));
    gSysLog.Info(LogMessage("port=") << port);
    gSysLog.Info(LogMessage("WaitTime=") << args.mWaitTime);

    IPEndPointAny endPoint;
    String ip;
    if (CmdLine::GetArgOption("net", "client_IPV4", ip))
    {
        if (!IPV4(endPoint, ip.CStr(), static_cast<UInt16>(port)))
        {
            gSysLog.Error(LogMessage("Failed to parse IPV4 address"));
            return;
        }
    }
    else if (CmdLine::GetArgOption("net", "client_IPV6", ip))
    {
        if (!IPV6(endPoint, ip.CStr(), static_cast<UInt16>(port)))
        {
            gSysLog.Error(LogMessage("Failed to parse IPV6 address"));
            return;
        }
    }
    else
    {
        CriticalAssert(IPV6(endPoint, LOCAL_IPV6, static_cast<UInt16>(port)));
    }

    NetClientDriver driver;
    if (!driver.Initialize(mServerKey, endPoint, appID, appVersion))
    {
        gSysLog.Error(LogMessage("Failed to initialize the NetClientDriver"));
        return;
    }

    // Wait for connection
    Int64 frequency = GetClockFrequency();
    Int64 connectionBegin = GetClockTime();
    while (!driver.IsConnected())
    {
        if ((GetClockTime() - connectionBegin) / static_cast<Float64>(frequency) > 2.0)
        {
            driver.Shutdown();
            return;
        }
    }

    Int64 begin = GetClockTime();
    Float64 time = 0.0;
    Float64 targetFrameRate = 1000.0 * (1.0 / 60.0);
    Int64 lastHeartBeat = GetClockTime();
    do
    {
        if (!driver.IsConnected())
        {
            gSysLog.Info(LogMessage("Client has been disconnected"));
            break;
        }

        bool force = ((GetClockTime() - lastHeartBeat) / static_cast<Float64>(frequency)) > 0.100;

        if (driver.EmitHeartbeat(force))
        {
            lastHeartBeat = GetClockTime();
        }

        Int64 frameBegin = GetClockTime();
        driver.Update();
        Float64 frameTime = 1000.0 * (GetClockTime() - frameBegin) / static_cast<Float64>(frequency);
        if (frameTime < targetFrameRate)
        {
            SizeT sleepTime = static_cast<SizeT>(targetFrameRate - frameTime);
            SleepCallingThread(sleepTime);
        }

        time = (GetClockTime() - begin) / static_cast<Float64>(frequency);
    } while (time < args.mWaitTime);

    driver.Shutdown();
}

void BasicNetApp::RunServer()
{
    // 1. Server
    //     a) Run a server, await a client connection, await client disconnection, shutdown
    //     b) Run a server, await client connection, shutdown
    //     c) Run a server, await N client connections, await all client disconnects, shutdown
    //     d) Run a server, await N client connections, shutdown
    //     e) Run a server, await client connection, drop connection shutdown
    //     f) Run a server, await client connection 3 connections, drop the 2nd one. shutdown

    Int32 port = BASIC_PORT;
    UInt16 appID = NetConfig::NET_APP_ID;
    UInt16 appVersion = NetConfig::NET_APP_VERSION;
    CmdLine::GetArgOption("net", "port", port);

    ServerArgs args;
    args.mWaitClients = 1;
    args.mWaitTime = 60.0f;
    args.mClientLifetime = 5.0f;
    CmdLine::GetArgOption("net", "server_WaitClients", args.mWaitClients);
    CmdLine::GetArgOption("net", "server_WaitTime", args.mWaitTime);
    CmdLine::GetArgOption("net", "server_ClientLifetime", args.mClientLifetime);


    gSysLog.Info(LogMessage("Running server with config."));
    gSysLog.Info(LogMessage("port=") << port);
    gSysLog.Info(LogMessage("WaitClients=") << args.mWaitClients);
    gSysLog.Info(LogMessage("WaitTime=") << args.mWaitTime);
    gSysLog.Info(LogMessage("ClientLifetime=") << args.mClientLifetime);


    NetServerDriver driver;
    if (!driver.Initialize(mServerKey, static_cast<UInt16>(port), appID, appVersion))
    {
        gSysLog.Error(LogMessage("Failed to initialize the NetServerDriver."));
        return;
    }

    Int64 frequency = GetClockFrequency();
    Int64 begin = GetClockTime();
    Float64 time = 0.0;
    Float64 targetFrameRate = 1000.0 * (1.0 / 60.0);
    do
    {
        Int64 frameBegin = GetClockTime();
        driver.Update();
        Float64 frameTime = 1000.0 * (GetClockTime() - frameBegin) / static_cast<Float64>(frequency);
        if (frameTime < targetFrameRate)
        {
            SizeT sleepTime = static_cast<SizeT>(targetFrameRate - frameTime);
            SleepCallingThread(sleepTime);
        }

        time = (GetClockTime() - begin) / static_cast<Float64>(frequency);
    } while (time < args.mWaitTime);

    driver.Shutdown();
}

void BasicNetApp::RunBasicClient()
{
    Int32 port = BASIC_PORT;
    CmdLine::GetArgOption("net", "port", port);

    ClientArgs args;
    args.mWaitTime = 10.0f;

    CmdLine::GetArgOption("net", "client_WaitTime", args.mWaitTime);

    gSysLog.Info(LogMessage("Running client with config."));
    gSysLog.Info(LogMessage("port=") << port);
    gSysLog.Info(LogMessage("WaitTime=") << args.mWaitTime);

    IPEndPointAny endPoint;
    String ip;
    NetProtocol::Value protocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    if (CmdLine::GetArgOption("net", "client_IPV4", ip))
    {
        if (!IPV4(endPoint, ip.CStr(), static_cast<UInt16>(port)))
        {
            gSysLog.Error(LogMessage("Failed to parse IPV4 address"));
            return;
        }
        protocol = NetProtocol::NET_PROTOCOL_IPV4_UDP;
    }
    else if (CmdLine::GetArgOption("net", "client_IPV6", ip))
    {
        if (!IPV6(endPoint, ip.CStr(), static_cast<UInt16>(port)))
        {
            gSysLog.Error(LogMessage("Failed to parse IPV6 address"));
            return;
        }
        protocol = NetProtocol::NET_PROTOCOL_IPV6_UDP;
    }
    else
    {
        CriticalAssert(IPV6(endPoint, LOCAL_IPV6, static_cast<UInt16>(port)));
        protocol = NetProtocol::NET_PROTOCOL_IPV6_UDP;
    }

    // if (source == IPV4 && dest == IPV6) { error("Source must be IPV6"); }
    // if (source == IPV6 && dest == IPV4)
    // ::ffff::00C0.00A8.0001.0009


    UDPSocket socket;
    if (!socket.Create(protocol))
    {
        gSysLog.Error(LogMessage("Failed to create UDP socket."));
        return;
    }

    String message = "Hello Server";
    SizeT numBytes = message.Size();
    const ByteT* bytes = reinterpret_cast<const ByteT*>(message.CStr());
    
    gSysLog.Info(LogMessage("Sending payload..."));
    if (!socket.SendTo(bytes, numBytes, endPoint))
    {
        gSysLog.Error(LogMessage("Failed to send some data!"));
    }
    SleepCallingThread(16);


    struct Context
    {
        UDPSocket* mSocket;
        ThreadFence mFence;
        volatile Atomic32 mRunning;
    };
    Context context;
    context.mSocket = &socket;
    CriticalAssert(context.mFence.Initialize());
    CriticalAssert(context.mFence.Set(true));
    AtomicStore(&context.mRunning, 1);

    // Await Reply:
    Thread thread;
    thread.Fork([](void* data) {
        Context* context = reinterpret_cast<Context*>(data);
        ByteT bytes[256] = { 0 };
        SizeT numBytes = 256;
        IPEndPointAny endPoint;

        while (AtomicLoad(&context->mRunning) != 0)
        {
            if (context->mSocket->ReceiveFrom(bytes, numBytes, endPoint))
            {
                gSysLog.Info(LogMessage("Received some data...") << ((endPoint.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4) ? " AddressFamily=IPV4" : "AddressFamily=IPV6"));
                break;
            }
        }
        context->mFence.Set(false);
    }, &context);

    gSysLog.Info(LogMessage("Waiting for reply..."));
    context.mFence.Wait(static_cast<SizeT>(args.mWaitTime * 1000.0));
    AtomicStore(&context.mRunning, 0);
    if (socket.IsAwaitingReceive())
    {
        gSysLog.Info(LogMessage("Did not receive reply :("));
        socket.Shutdown();
    }
    else
    {
        socket.Close();
    }
    thread.Join();
    gSysLog.Info(LogMessage("All done..."));

    
}
void BasicNetApp::RunBasicServer()
{
    Int32 port = BASIC_PORT;
    CmdLine::GetArgOption("net", "port", port);

    ServerArgs args;
    args.mWaitTime = 60.0f;
    CmdLine::GetArgOption("net", "server_WaitTime", args.mWaitTime);
    NetProtocol::Value protocol = NetProtocol::NET_PROTOCOL_UDP;
    if (CmdLine::HasArgOption("net", "server_IPV4")) { protocol = NetProtocol::NET_PROTOCOL_IPV4_UDP; }
    else if (CmdLine::HasArgOption("net", "server_IPV6")) { protocol = NetProtocol::NET_PROTOCOL_IPV6_UDP; }

    UDPSocket socket;
    if (!socket.Create(protocol))
    {
        gSysLog.Error(LogMessage("Failed to create UDP socket"));
        return;
    }

    switch (protocol)
    {
    case NetProtocol::NET_PROTOCOL_UDP:
        gSysLog.Info(LogMessage("Running server as UDP IP agnostic"));
        break;
    case NetProtocol::NET_PROTOCOL_IPV4_UDP:
        gSysLog.Info(LogMessage("Running server as UDP IPV4"));
        break;
    case NetProtocol::NET_PROTOCOL_IPV6_UDP:
        gSysLog.Info(LogMessage("Running server as UDP IPV6"));
        break;
    }

    struct Context
    {
        UDPSocket* mSocket;
        ThreadFence mFence;
        volatile Atomic32 mRunning;
    };
    Context context;
    context.mSocket = &socket;
    CriticalAssert(context.mFence.Initialize());
    CriticalAssert(context.mFence.Set(true));
    AtomicStore(&context.mRunning, 1);

    socket.Bind(static_cast<UInt16>(port));

    Thread thread;
    thread.Fork([](void* data) {
        Context* context = reinterpret_cast<Context*>(data);
        ByteT bytes[256] = { 0 };
        SizeT numBytes = 256;
        IPEndPointAny endPoint;
        while (context->mRunning)
        {
            if (context->mSocket->ReceiveFrom(bytes, numBytes, endPoint))
            {
                String originalIPAddress = IPToString(endPoint);
                String ipAddress = IPToString(endPoint);
                String portStr = ipAddress.SubString(ipAddress.FindLast(':') + 1);
                ipAddress = ipAddress.SubString(0, ipAddress.FindLast(':'));
                if (Invalid(ipAddress.Find(':')))
                {
                    gSysLog.Info(LogMessage("Converting IPV6 to IPV4"));
                    CriticalAssert(IPV4(endPoint, ipAddress.CStr(), SwapBytes(static_cast<UInt16>(ToInt32(portStr)))));
                }
                else if (ipAddress.Find("::ffff:") == 0)
                {
                    // ipv4 convert

                    ipAddress = ipAddress.SubString(7);
                    gSysLog.Info(LogMessage("Converting IPV6 to IPV4"));
                    CriticalAssert(IPV4(endPoint, ipAddress.CStr(), static_cast<UInt16>(ToInt32(portStr))));
                }

                gSysLog.Info(LogMessage("Sending echo to ") << originalIPAddress << " | " << IPToString(endPoint));
                UDPSocket socket;
                socket.Create(endPoint.mAddressFamily == NetAddressFamily::NET_ADDRESS_FAMILY_IPV4 ? NetProtocol::NET_PROTOCOL_IPV4_UDP : NetProtocol::NET_PROTOCOL_IPV6_UDP);
                String message = IPToString(endPoint);
                const ByteT* messageBytes = reinterpret_cast<const ByteT*>(message.CStr());
                SizeT messageLength = message.Size();
                socket.SendTo(messageBytes, messageLength, endPoint);
                socket.Close();
            }
        }

        context->mFence.Set(false);
    }, &context);

    context.mFence.Wait(static_cast<SizeT>(args.mWaitTime * 1000.0));
    AtomicStore(&context.mRunning, 0);

    if (socket.IsAwaitingReceive())
    {
        socket.Shutdown();
    }
    else
    {
        socket.Close();
    }
    thread.Join();
    gSysLog.Info(LogMessage("All done!"));

}

}
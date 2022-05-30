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

#include "NetTestUtils.h"
#include "Core/Utility/Time.h"

namespace lf
{
    
#if 0
class LocalClientServerImpl : public LocalClientServer
{
public:
    LocalClientServerImpl(const char* ip, UInt16 port) 
    : mIP(ip)
    , mPort(port)
    , mServerKey()
    , mConnectTimeout(5.0)
    , mTimeout(30.0)
    , mOnUpdate()
    , mClientOnConnect()
    , mClientOnDisconnect()
    , mServerOnClientConnect()
    , mServerOnClientDisconnect()
    , mServer()
    , mClient()
    {

    }

    void OnUpdate(const UpdateCallback& callback) override;
    void ClientOnConnect(const ClientOnConnectCallback& callback) override;
    void ClientOnDisconnect(const ClientOnDisconnectCallback& callback) override;
    void ServerOnClientConnected(const ServerOnClientConnectedCallback& callback) override;
    void ServerOnClientDisconnected(const ServerOnClientDisconnectedCallback& callback) override;

    void Run() override;

    bool Start();
    void Update();
    void Shutdown();
private:
    String mIP;
    UInt16 mPort;
    Crypto::RSAKey mServerKey;
    Float64 mConnectTimeout;
    Float64 mTimeout;

    UpdateCallback                     mOnUpdate;
    ClientOnConnectCallback            mClientOnConnect;
    ClientOnDisconnectCallback         mClientOnDisconnect;
    ServerOnClientConnectedCallback    mServerOnClientConnect;
    ServerOnClientDisconnectedCallback mServerOnClientDisconnect;

    NetServerDriver                    mServer;
    NetClientDriver                    mClient;
};

LocalClientServerPtr LocalClientServer::Create(const char* ip, UInt16 port)
{
    return LocalClientServerPtr(LFNew<LocalClientServerImpl>(MMT_GENERAL, ip, port));
}

void LocalClientServerImpl::OnUpdate(const UpdateCallback& callback)
{
    mOnUpdate = callback;
}
void LocalClientServerImpl::ClientOnConnect(const ClientOnConnectCallback& callback)
{
    mClientOnConnect = callback;
}
void LocalClientServerImpl::ClientOnDisconnect(const ClientOnDisconnectCallback& callback)
{
    mClientOnDisconnect = callback;
}
void LocalClientServerImpl::ServerOnClientConnected(const ServerOnClientConnectedCallback& callback)
{
    mServerOnClientConnect = callback;
}
void LocalClientServerImpl::ServerOnClientDisconnected(const ServerOnClientDisconnectedCallback& callback)
{
    mServerOnClientDisconnect = callback;
}

void LocalClientServerImpl::Run()
{
    if (Start())
    {
        Update();
    }
    Shutdown();
}

bool LocalClientServerImpl::Start()
{
    if (mServerKey.GetKeySize() == Crypto::RSA_KEY_Unknown)
    {
        if (!mServerKey.GeneratePair(Crypto::RSA_KEY_2048))
        {
            return false;
        }
    }
    
    if (!mServer.Initialize(mServerKey, mPort, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION))
    {
        return false;
    }

    IPEndPointAny endPoint;
    IPCast(IPV4(mIP.CStr(), mPort), endPoint);
    if (!mClient.Initialize(mServerKey, endPoint, NetConfig::NET_APP_ID, NetConfig::NET_APP_VERSION))
    {
        return false;
    }

    return true;
}

void LocalClientServerImpl::Update()
{
    Int64 frequency = GetClockFrequency();
    Int64 beginTick = GetClockTime();

    while (true)
    {
        Float64 duration = (GetClockTime() - beginTick) / static_cast<Float64>(frequency);
        if (duration > mConnectTimeout)
        {
            break;
        }

        if (mClient.IsConnected())
        {
            break;
        }
        mServer.Update();
    }

    if (!mClient.IsConnected())
    {
        return;
    }

    if (mClientOnConnect)
    {
        mClientOnConnect.Invoke();
    }

    if (mServerOnClientConnect)
    {
        mServerOnClientConnect.Invoke();
    }

    SizeT frame = 0;
    while (true)
    {
        Float64 duration = (GetClockTime() - beginTick) / static_cast<Float64>(frequency);
        if (duration > mTimeout)
        {
            break;
        }

        if (!mClient.IsConnected())
        {
            if (mClientOnDisconnect)
            {
                mClientOnDisconnect.Invoke();
            }

            if (mServerOnClientDisconnect)
            {
                mServerOnClientDisconnect.Invoke();
            }

            break;
        }

        mClient.EmitHeartbeat();
        mClient.Update();
        mServer.Update();

        if (mOnUpdate)
        {
            mOnUpdate.Invoke(&mServer, &mClient);
        }

        Thread::Sleep(32);
        ++frame;
    }
}

void LocalClientServerImpl::Shutdown()
{
    mClient.Shutdown();
    mServer.Shutdown();
}
#endif
}
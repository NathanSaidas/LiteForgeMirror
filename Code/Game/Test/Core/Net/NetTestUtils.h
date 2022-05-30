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
#pragma once

#include "Core/Test/Test.h"
#include "Core/Memory/SmartPointer.h"
#include "Core/Net/NetFramework.h"
#include "Core/Utility/SmartCallback.h"

namespace lf {

namespace Crypto {
class AESKey;
class RSAKey;
}

struct NetTestInitializer
{
    NetTestInitializer() 
    : mRelease(!IsNetInitialized())
    , mSuccess(mRelease ? NetInitialize() : true)
    {

    }
    ~NetTestInitializer()
    {
        if (mRelease) { TEST(NetShutdown()); }
    }
    bool mRelease;
    bool mSuccess;
};

class LocalClientServer;
using LocalClientServerPtr = TStrongPointer<LocalClientServer>;

class NetDriver;

#if 0
class LocalClientServer
{
public:
    using UpdateCallback = TCallback<void, NetDriver*, NetDriver*>;
    using ClientOnConnectCallback = TCallback<void>;
    using ClientOnDisconnectCallback = TCallback<void>;
    using ServerOnClientConnectedCallback = TCallback<void>;
    using ServerOnClientDisconnectedCallback = TCallback<void>;

    static LocalClientServerPtr Create(const char* ip, UInt16 port);

    virtual ~LocalClientServer() {}

    virtual void OnUpdate(const UpdateCallback& callback) = 0;
    template<typename T> void OnUpdate(const T& lambda) { OnUpdate(UpdateCallback::Make(lambda)); }

    virtual void ClientOnConnect(const ClientOnConnectCallback& callback) = 0;
    template<typename T> void ClientOnConnect(const T& lambda) { ClientOnConnect(ClientOnConnectCallback::Make(lambda)); }

    virtual void ClientOnDisconnect(const ClientOnDisconnectCallback& callback) = 0;
    template<typename T> void ClientOnDisconnect(const T& lambda) { ClientOnDisconnect(ClientOnDisconnectCallback::Make(lambda)); }

    virtual void ServerOnClientConnected(const ServerOnClientConnectedCallback& callback) = 0;
    template<typename T> void ServerOnClientConnected(const T& lambda) { ServerOnClientConnected(ServerOnClientConnectedCallback::Make(lambda)); }

    virtual void ServerOnClientDisconnected(const ServerOnClientDisconnectedCallback& callback) = 0;
    template<typename T> void ServerOnClientDisconnected(const T& lambda) { ServerOnClientDisconnected(ServerOnClientDisconnectedCallback::Make(lambda)); }

    virtual void Run() = 0;
};
#endif


namespace NetTestUtil
{
bool LoadPrivateKey(const char* filename, Crypto::RSAKey& key);
bool LoadPublicKey(const char* filename, Crypto::RSAKey& key);
bool LoadAsPublicKey(const char* filename, Crypto::RSAKey& key);
bool LoadSharedKey(const char* filename, Crypto::AESKey& key);
}

extern const char* RSA_KEY_SERVER;
extern const char* RSA_KEY_CLIENT;
extern const char* AES_KEY_SHARED;
extern const char* RSA_KEY_UNIQUE;

}
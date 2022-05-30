#pragma once
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
#include "Core/Common/Types.h"
#include "Core/Crypto/AES.h"
#include "Core/Crypto/ECDH.h"
#include "Core/Crypto/HMAC.h"
#include "Core/Crypto/SHA256.h"
#include "Core/Crypto/RSA.h"

#include "Core/Memory/SmartPointer.h"
#include "Core/Net/NetTypes.h"
#include "Core/Net/NetFramework.h"
#include "Core/Utility/Time.h"

#include "Runtime/Net/Server/NetSecureServerDriver.h"
#include "Runtime/Net/Client/NetSecureClientDriver.h"

#include "Game/Test/Core/Net/NetTestUtils.h"


namespace lf {

// **********************************
// Below is boiler-plate code to write a network unit test, 
// it initializes client/server and updates them to connect
// then shuts down.
// **********************************
// 
// const NetTestInitializer NET_INIT;
// const SimpleConnectionConfig CONFIG;
//
// NetSecureServerDriver serverDriver;
// NetSecureClientDriver clientDriver;
// StabilityTester tester;
// tester.mServer = &serverDriver;
// tester.mClient = &clientDriver;
// tester.FilterPackets();
//
// TEST(CONFIG.Initialize(serverDriver));
// TEST(CONFIG.Initialize(clientDriver));
//
// ExecuteUpdate(5.0f, 60, [&tester] {
//     tester.Update();
//     return true;
// });
// TEST(clientDriver.IsConnected());
// TEST(serverDriver.FindConnection(clientDriver.GetSessionID()) != NULL_PTR);
// serverDriver.Shutdown();
// clientDriver.Shutdown();
// **********************************

// **********************************
// Runs an update loop for a set amount of time at a capped frame rate
// 
// @param executionTime - The amount of time in seconds to execute the update loop
// @param frameRate     - The maximum frame rate the update loop can execute at.
// @param callback      - [bool Callback()] A callback executed each frame. Return false to break out of the update loop
// **********************************
template<typename Pred>
void ExecuteUpdate(Float32 executionTime, SizeT frameRate, Pred callback);

void LogPacketDetails(const char* message, const ByteT* bytes, SizeT size, const IPEndPointAny& endPoint);
NetDriver::Options GetStandardMessageOptions();

class PacketAction
{
public:
    virtual ~PacketAction() {}
    virtual bool Filter(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint) = 0;
    virtual void Update(NetSecureServerDriver* server) = 0;
    virtual void Update(NetSecureClientDriver* client) = 0;

    bool AcceptsType(const ByteT* bytes, SizeT numBytes) const;
    void SetFilterType(NetPacketType::Value packetType) { mPacketFilterType = packetType; }
protected:
private:
    NetPacketType::Value mPacketFilterType;
};
DECLARE_PTR(PacketAction);

class StabilityTester
{
public:
    StabilityTester();

    NetSecureClientDriver* mClient;
    NetSecureServerDriver* mServer;

    void DropServer(NetPacketType::Value packetType = NetPacketType::INVALID_ENUM);
    void DelayServer(Float32 seconds, NetPacketType::Value packetType = NetPacketType::INVALID_ENUM);
    void DefaultServer(NetPacketType::Value packetType = NetPacketType::INVALID_ENUM);
    void DropClient(NetPacketType::Value packetType = NetPacketType::INVALID_ENUM);
    void DelayClient(Float32 seconds, NetPacketType::Value packetType = NetPacketType::INVALID_ENUM);
    void DefaultClient(NetPacketType::Value packetType = NetPacketType::INVALID_ENUM);

    void FilterPackets();
    void Update();

private:
    TVector<PacketActionPtr> mServerActions;
    SizeT mServerActionIndex;
    TVector<PacketActionPtr> mClientActions;
    SizeT mClientActionIndex;

    bool OnFilterServerPacket(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint)
    {
        if (mServerActionIndex < mServerActions.size() && mServerActions[mServerActionIndex]->AcceptsType(bytes, numBytes))
        {
            return mServerActions[mServerActionIndex++]->Filter(bytes, numBytes, endPoint);
        }
        LogPacketDetails("[ReceiveServer]: ", bytes, numBytes, endPoint);
        return false;
    }

    bool OnFilterClientPacket(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint)
    {
        if (mClientActionIndex < mClientActions.size() && mClientActions[mClientActionIndex]->AcceptsType(bytes, numBytes))
        {
            return mClientActions[mClientActionIndex++]->Filter(bytes, numBytes, endPoint);
        }
        LogPacketDetails("[ReceiveClient]: ", bytes, numBytes, endPoint);
        return false;
    }
};

struct SimpleConnectionConfig
{
    SimpleConnectionConfig()
    {
        TEST(mServerCertification.GeneratePair(Crypto::RSA_KEY_2048));
        IPCast(IPV4("127.0.0.1", 8080), mIP);
    }

    bool Initialize(NetSecureServerDriver& server) const
    {
        NetServerDriverConfig config;
        config.mAppID = 0;
        config.mAppVersion = 0;
        config.mCertificate = &mServerCertification;
        config.mPort = 8080;

        return server.Initialize(config);
    }
    bool Initialize(NetSecureClientDriver& client) const
    {
        return client.Initialize(0, 0, mIP, mServerCertification);
    }

    Crypto::RSAKey mServerCertification;
    IPEndPointAny  mIP;
};



// ** INLINE

template<typename Pred>
void ExecuteUpdate<Pred>(Float32 executionTime, SizeT frameRate, Pred callback)
{
    Float32 fps = static_cast<Float32>(frameRate);
    SizeT targetMs = static_cast<SizeT>((1.0f / fps) * 1000.0f);

    Timer t;
    t.Start();
    while (t.PeekDelta() < executionTime)
    {
        Timer frame;
        frame.Start();
        if (!callback())
        {
            break;
        }
        frame.Stop();

        SizeT ms = static_cast<SizeT>(frame.GetDelta() * 1000.0);
        if (ms < targetMs)
        {
            SleepCallingThread(targetMs - ms);
        }
    }
}

}

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
#include "NetDriverTestUtils.h"
#include "Core/Utility/Log.h"
#include "Runtime/Net/PacketSerializer.h"

namespace lf {

static const char* PACKET_TYPE_NAMES[] =
{
    "CONNECT",
    "DISCONNECT",
    "HEARTBEAT",
    "MESSAGE",
    "REQUEST",
    "RESPONSE",
    "CLIENT_HELLO",
    "SERVER_HELLO",
};
LF_STATIC_ASSERT(LF_ARRAY_SIZE(PACKET_TYPE_NAMES) == NetPacketType::MAX_VALUE);

void LogPacketDetails(const char* message, const ByteT* bytes, SizeT size, const IPEndPointAny& endPoint)
{
    String ipAddress = IPToString(endPoint);

    PacketSerializer ps;
    if (ps.SetBuffer(bytes, size))
    {
        const char* type = ps.GetType() < LF_ARRAY_SIZE(PACKET_TYPE_NAMES) ? PACKET_TYPE_NAMES[ps.GetType()] : "INVALID_TYPE";
        const char* ack = ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_ACK) ? "true" : "false";
        const char* sign = ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_SIGNED) ? "true" : "false";
        const char* hmac = ps.HasFlag(NetPacketFlag::NET_PACKET_FLAG_HMAC) ? "true" : "false";

        gTestLog.Info(LogMessage(message) << "Address=" << ipAddress << ", UID=" << ps.GetPacketUID() << ", Type=" << type << ", Ack=" << ack << ", Signed=" << sign << ", HMAC=" << hmac);
    }
    else
    {
        gTestLog.Info(LogMessage(message) << "Address=" << ipAddress << " BAD_PACKET ");
    }
}

NetDriver::Options GetStandardMessageOptions()
{
    return NetDriver::OPTION_RELIABLE | NetDriver::OPTION_ENCRYPT | NetDriver::OPTION_SIGNED | NetDriver::OPTION_HMAC;
}

bool PacketAction::AcceptsType(const ByteT* bytes, SizeT numBytes) const
{
    if (ValidEnum(mPacketFilterType))
    {
        PacketSerializer ps;
        if (ps.SetBuffer(bytes, numBytes) && ps.GetType() == mPacketFilterType)
        {
            return true;
        }
        return false;
    }
    return true;
}

class DropPacketAction : public PacketAction
{
public:
    bool Filter(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint) override
    {
        LogPacketDetails("Force dropping packet...", bytes, numBytes, endPoint);
        return true;
    }
    void Update(NetSecureServerDriver*) override { };
    void Update(NetSecureClientDriver*) override { };
};

class DelayPacketAction : public PacketAction
{
public:
    bool Filter(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint) override
    {
        LogPacketDetails("Force delaying packet...", bytes, numBytes, endPoint);

        mData.resize(numBytes);
        memcpy(mData.data(), bytes, numBytes);
        mEndPoint = endPoint;
        mDelayTimer.Start();
        return true;
    }

    void Update(NetSecureServerDriver* server) override
    {
        if (mDelayTimer.IsRunning() && mDelayTimer.PeekDelta() > mDelayAmount)
        {
            mDelayTimer = Timer();
            LogPacketDetails("Sending delayed packet...", mData.data(), mData.size(), mEndPoint);
            server->ProcessPacketData(mData.data(), mData.size(), mEndPoint);
            mData.clear();
            mEndPoint = IPEndPointAny();
        }
    }
    void Update(NetSecureClientDriver* client) override
    {
        if (mDelayTimer.IsRunning() && mDelayTimer.PeekDelta() > mDelayAmount)
        {
            mDelayTimer = Timer();
            client->ProcessPacketData(mData.data(), mData.size(), mEndPoint);
            mData.clear();
            mEndPoint = IPEndPointAny();
        }
    }

    Float32 mDelayAmount;
    Timer   mDelayTimer;
    TVector<ByteT> mData;
    IPEndPointAny mEndPoint;
};

class DefaultPacketAction : public PacketAction
{
    bool Filter(const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint) override
    {
        LogPacketDetails("Processing packet as default...", bytes, numBytes, endPoint);
        return false;
    }
    void Update(NetSecureServerDriver*) override { };
    void Update(NetSecureClientDriver*) override { };
};

StabilityTester::StabilityTester()
: mClient(nullptr)
, mServer(nullptr)
, mServerActions()
, mServerActionIndex(0)
, mClientActions()
, mClientActionIndex(0)
{}

void StabilityTester::DropServer(NetPacketType::Value packetType)
{
    TStrongPointer<DropPacketAction> action(LFNew<DropPacketAction>());
    action->SetFilterType(packetType);
    mServerActions.push_back(action);
}
void StabilityTester::DelayServer(Float32 seconds, NetPacketType::Value packetType)
{
    TStrongPointer<DelayPacketAction> action(LFNew<DelayPacketAction>());
    action->SetFilterType(packetType);
    action->mDelayAmount = seconds;
    mServerActions.push_back(action);
}
void StabilityTester::DefaultServer(NetPacketType::Value packetType)
{
    TStrongPointer<DefaultPacketAction> action(LFNew<DefaultPacketAction>());
    action->SetFilterType(packetType);
    mServerActions.push_back(action);
}

void StabilityTester::DropClient(NetPacketType::Value packetType)
{
    TStrongPointer<DropPacketAction> action(LFNew<DropPacketAction>());
    action->SetFilterType(packetType);
    mClientActions.push_back(action);
}
void StabilityTester::DelayClient(Float32 seconds, NetPacketType::Value packetType)
{
    TStrongPointer<DelayPacketAction> action(LFNew<DelayPacketAction>());
    action->SetFilterType(packetType);
    action->mDelayAmount = seconds;
    mClientActions.push_back(action);
}
void StabilityTester::DefaultClient(NetPacketType::Value packetType)
{
    TStrongPointer<DefaultPacketAction> action(LFNew<DefaultPacketAction>());
    action->SetFilterType(packetType);
    mClientActions.push_back(action);
}

void StabilityTester::FilterPackets()
{
    if (mServer)
    {
        mServer->SetPacketFilter(NetSecureServerDriver::PacketFilter::Make([this]
        (const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint) -> bool {
            return OnFilterServerPacket(bytes, numBytes, endPoint);
        })
        );
    }

    if (mClient)
    {
        mClient->SetPacketFilter(NetSecureClientDriver::PacketFilter::Make([this]
        (const ByteT* bytes, SizeT numBytes, const IPEndPointAny& endPoint) -> bool {
            return OnFilterClientPacket(bytes, numBytes, endPoint);
        })
        );
    }
}

void StabilityTester::Update()
{
    if (mServer)
    {
        mServer->Update();
    }

    if (mClient)
    {
        mClient->Update();
    }
    if (mServer)
    {
        for (PacketAction* action : mServerActions) { action->Update(mServer); }
    }
    if (mClient)
    {
        for (PacketAction* action : mClientActions) { action->Update(mClient); }
    }
}

} // namespace lf